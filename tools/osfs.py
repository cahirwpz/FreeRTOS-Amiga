#!/usr/bin/env python3

import argparse
import os
import stat
from array import array
from struct import pack, unpack
from io import BytesIO

FLOPPY = 80 * 11 * 2

#
# On disk format description:
#
# sector 0..1: bootblock
#  [LONG] 'DOS\0'
#  [LONG] checksum
#  [LONG] size of executable file aligned to sector size (takes m sectors)
#  ...    boot code
#
# sector 2..(n+1): directory entries (take n sectors)
#  [WORD] dirsize : total size of directory entries in bytes
#  for each directory entry (2-byte aligned):
#   [BYTE] #reclen : total size of this record
#   [BYTE] #type   : type of file (1: executable, 0: regular)
#   [WORD] #start  : sector where the file begins (0..1759)
#   [LONG] #length : size of the file in bytes (up to 1MiB)
#   [STRING] #name : name of the file (NUL terminated)
#
# sector (n+2)..(n+m+1): executable file in AmigaHunk format
#  the file is assumed to be first in the directory
#
# sector (m+n+2)..1759: data
#


def align(size, n):
    return (size + n - 1) // n * n


def write_pad(fh, alignment=None):
    pos = fh.tell()
    pad = align(pos, alignment) - pos
    fh.write(b'\0' * pad)


def checksum(data):
    arr = array('I', data)
    arr.byteswap()
    chksum = sum(arr)
    chksum = (chksum >> 32) + (chksum & 0xffffffff)
    return (~chksum & 0xffffffff)


FS_SBOFF = 1024
FS_SBSZ = 32
FS_BLKSIZE = 512
FS_MAGIC = 0x43697269
FS_BPB = 8 * FS_BLKSIZE              # bits per block


class Superblock(object):
    __layout__ = '>IIIIIIII'
    __slots__ = ('magic', 'nblocks', 'nfreeblk', 'ninodes', 'nfreeino',
                 'inodestart', 'blkbmstart', 'datastart')

    def __init__(self, nblocks, nfreeblk, ninodes, nfreeino, inodestart,
                 blkbmstart, datastart):
        self.magic = FS_MAGIC
        self.nblocks = nblocks
        self.nfreeblk = nfreeblk
        self.ninodes = ninodes
        self.nfreeino = nfreeino
        self.inodestart = inodestart
        self.blkbmstart = blkbmstart
        self.datastart = datastart

    @classmethod
    def read(cls, data):
        sb = unpack(cls.__layout__, data)
        assert sb[0] == FS_MAGIC
        return cls(*sb[1:])

    def __bytes__(self):
        return pack(self.__layout__, self.magic, self.nblocks,
                    self.nfreeblk, self.ninodes, self.nfreeino,
                    self.inodestart, self.blkbmstart, self.datastart)


FS_INOBMOFF = FS_SBOFF + FS_BLKSIZE     # where i-node bitmap starts
FS_NADDR = 13       # Number of block addresses in an i-node
FS_INODESZ = 64     # I-node size in bytes

# File permissions
FS_IEXEC = 0x01     # executable
FS_IWRITE = 0x02    # writable
FS_IREAD = 0x04     # readable

# File types
FS_IFREG = 0x10     # regular file
FS_IFDIR = 0x20     # directory file
FS_IFCHR = 0x30     # character device
FS_IFBLK = 0x40     # block device
FS_IFLNK = 0x50     # symbolic link
FS_IFMT = 0x70      # mask of file type


class Inode(object):
    __slots__ = ('number', 'mode', 'nlink', 'nblock', 'size', 'blocks')
    __layout__ = '>HHII13I'

    def __init__(self, number=0):
        self.number = number
        self.mode = 0
        self.nlink = 0
        self.nblock = 0
        self.size = 0
        self.blocks = [0 for i in range(FS_NADDR)]

    def __bytes__(self):
        return pack(self.__layout__, self.mode, self.nlink, self.nblock,
                    self.size, *self.blocks)

    def __str__(self):
        return 'Inode[%d]' % self.number


# Maximum file name length
FS_MAXNAMLEN = 14
FS_DIRENTSZ = 16


class DirEntry(object):
    __slots__ = ('inum', 'name')
    __layout__ = '>H14s'

    def __init__(self, inum=0, name=b''):
        self.inum = inum
        self.name = name

    @classmethod
    def read(cls, data):
        return DirEntry(*unpack(self.__layout__, data))

    def __bytes__(self):
        return pack(self.__layout__, self.inum, self.name)

    def __str__(self):
        return 'DirEntry<inum=%d, name=%r>' % (self.inum, self.name)


class Filesystem(object):
    @classmethod
    def mkfs(cls, fo, ntotal, ninodes):
        # substract 3 for boot block and super block
        nleft = ntotal - 3

        # calculate number of i-nodes
        ninodes = align(ninodes, FS_BLKSIZE // FS_INODESZ)
        inodeblks = ninodes * FS_INODESZ // FS_BLKSIZE
        inobmblks = align(ninodes, FS_BPB) // FS_BPB

        if ninodes * FS_INODESZ > FS_BLKSIZE * ntotal / 8:
            raise SystemExit(
                    'i-nodes cannot take more than 1/8 of available space')

        # substract space for i-node bitmap and table
        nleft -= inobmblks + inodeblks

        # calculate number of blocks
        nblocks = (nleft * FS_BPB - (FS_BPB - 1)) // (FS_BPB + 1)
        datbmblks = nleft - nblocks

        # we have all the data to determine super block value
        blkbmstart = 3 + inobmblks
        inodestart = blkbmstart + datbmblks
        datastart = inodestart + inodeblks

        sb = Superblock(nblocks, nblocks, ninodes, ninodes, blkbmstart,
                        inodestart, datastart)

        fo.truncate(nblocks * FS_BLKSIZE)

        fs = cls(fo, sb)
        fs.sbwrite()

        return fs

    @classmethod
    def mount(cls, fo):
        fo.seek(FS_SBOFF)
        sb = Superblock.read(fo.read(FS_SBSZ))
        return Filesystem(fo, sb)

    def __init__(self, fo, sb):
        self.sb = sb
        self.__fo = fo

    def sbwrite(self):
        self.write(FS_SBOFF, self.sb)

    def __bitalloc(self, off, n, start):
        for i in range(n):
            j = i & 7
            if j == 0:
                b8 = self.read(off + i // 8, 1)[0]
            if b8 & (1 << j):
                continue
            b8 |= (1 << j)
            self.write(off + i, bytes([b8]))
            return i + start

        return None

    def ialloc(self):
        inum = self.__bitalloc(FS_INOBMOFF, self.sb.ninodes, 1)

        if not inum and self.sb.ninodes > 0:
            raise RuntimeError('no free i-node found!')

        return Inode(inum)

    def balloc(self):
        bnum = self.__bitalloc(self.sb.blkbmstart, self.sb.nblocks,
                               self.sb.datastart)

        if not bnum and self.sb.nblocks > 0:
            raise RuntimeError('no free block found!')

        return bnum

    def iwrite(self, inode):
        off = self.sb.inodestart * FS_BLKSIZE
        self.write(off + (inode.number - 1) * FS_INODESZ, inode)

    def read(self, off, size):
        self.__fo.seek(off)
        return self.__fo.read(size)

    def write(self, off, *args):
        self.__fo.seek(off)
        for data in args:
            self.__fo.write(bytes(data))


def mkfs(image, nblocks, ninodes):
    with open(image, 'w+b') as fo:
        fs = Filesystem.mkfs(fo, nblocks, ninodes)

        ino = fs.ialloc()
        ino.mode = FS_IREAD | FS_IWRITE | FS_IEXEC | FS_IFDIR
        ino.nlink = 2
        ino.nblock = 1
        ino.size = 2 * FS_DIRENTSZ
        ino.blocks[0] = fs.balloc()
        fs.iwrite(ino)

        off = ino.blocks[0] * FS_BLKSIZE
        de1 = DirEntry(1, b'.')
        de2 = DirEntry(1, b'..')
        fs.write(off, de1, de2)


def boot(image, bootcode):
    if not os.path.isfile(bootcode):
        raise SystemExit('Boot code file does not exists!')

    with open(bootcode, 'rb') as fh:
        bootcode = fh.read()

    if len(bootcode) > 2 * FS_BLKSIZE:
        raise SystemExit('Boot code is larger than 1024 bytes!')

    boot = BytesIO(bootcode)
    # Move to the end and pad it so it takes 2 sectors
    boot.seek(0, os.SEEK_END)
    write_pad(boot, 2 * FS_BLKSIZE)
    # Calculate checksum and fill missing field in boot block
    val = checksum(boot.getvalue())
    boot.seek(4, os.SEEK_SET)
    boot.write(pack('>I', val))
    # Write fixed boot block to file system image
    with open(image, 'r+b') as fo:
        fs = Filesystem.mount(fo)
        fs.write(0, boot.getvalue())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Tool for handling OverSimplified FileSystem.')
    parser.add_argument(
        'image', metavar='IMAGE', type=str,
        help='File system image file.')
    subparsers = parser.add_subparsers(
        dest='command', help='Action to perform for filesystem image.')

    parser_mkfs = subparsers.add_parser(
        'mkfs', help='Create new filesystem.')
    parser_mkfs.add_argument(
        '-s', '--sectors', type=int,
        help='Number of 512B sectors.', default=FLOPPY)
    parser_mkfs.add_argument(
        '-i', '--inodes', type=int,
        help='Number of i-nodes.', default=128)

    parser_boot = subparsers.add_parser(
        'boot', help='Install boot loader.')
    parser_boot.add_argument(
        'bootcode', type=str, help='Boot code to be embedded in boot sector.')

    parser_add = subparsers.add_parser(
        'add', help='Add files and directories.')
    parser_add.add_argument(
        'files', metavar='FILES', type=str, nargs='*',
        help='Files to add to / extract from filesystem image.')

    parser_extract = subparsers.add_parser(
        'extract', help='Extract files to local filesystem.')
    parser_extract.add_argument(
        '-f', '--force', action='store_true',
        help='If output file exist, the tool will overwrite it.')

    parser_ls = subparsers.add_parser(
        'ls', help='List directory contents.')

    args = parser.parse_args()

    if args.command == 'mkfs':
        mkfs(args.image, args.sectors, args.inodes)
    elif args.command == 'boot':
        boot(args.image, args.bootcode)
