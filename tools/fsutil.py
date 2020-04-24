#!/usr/bin/env python3

import argparse
import os
import stat
from array import array
from fnmatch import fnmatch
from struct import pack, unpack
from io import BytesIO

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
#  [WORD] #namelen : size of file names in bytes (see STRTAB below)
#  [WORD] #files   : number of file entries
#  repeated #files times:
#   [WORD] #name   : offset of file name determined from start of string table
#   [WORD] #start  : sector position where the file begins (**)
#   [LONG] #length : size of the file
#  [STRTAB] file names corresponding to file entries
#           stored consecutively as strings with trailing '\0'
#
# (**) negative if file is executable
#
# sector (n+2)..(n+m+1): executable file in AmigaHunk format
#  the file is assumed to be first in the directory
#
# sector (m+n+2)..1759: data
#

SECTOR = 512
FLOPPY = SECTOR * 80 * 11 * 2


def align(size, alignment=None):
    if alignment is None:
        alignment = SECTOR
    return (size + alignment - 1) // alignment * alignment


def sectors(size):
    return align(size, SECTOR) // SECTOR


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


class FileEntry(object):
    __slots__ = ('name', 'data', 'exe')

    def __init__(self, name, data, exe):
        self.name = name
        self.data = data
        self.exe = exe

    def __str__(self):
        s = '%-32s %6d' % (self.name, len(self))
        if self.exe:
            s += ' (executable)'
        return s

    def __len__(self):
        return len(self.data)


def collect(paths):
    entries = []

    for path in paths:
        if not os.path.exists(path):
            raise SystemExit('create: %s does not exist' % path)
        if not os.path.isfile(path):
            raise SystemExit('create: %s is not a regular file' % path)

        name = os.path.basename(path)

        if any(e.name == name for e in entries):
            raise SystemExit(
                'create: %s has already been added to archive' % path)

        with open(path, 'rb') as fh:
            data = fh.read()

        entry = FileEntry(name, data, bool(
            os.stat(path).st_mode & stat.S_IEXEC))
        entries.append(entry)

    return entries


def load(archive):
    with open(archive, 'rb') as fh:
        fh.seek(2 * SECTOR)

        names_len, dirent_count = unpack('>HH', fh.read(4))
        dirent = [list(unpack('>HhI', fh.read(8)))
                  for i in range(dirent_count)]
        strtab = fh.read(names_len)

        entries = []
        for nameoff, offset, size in dirent:
            exe = bool(offset < 0)
            if exe:
                offset = -offset
            fh.seek(offset)
            namesz = strtab[nameoff:].index(b'\0')
            name = strtab[nameoff:nameoff + namesz].decode()
            entries.append(FileEntry(name, fh.read(size), exe))

        return entries


def save(archive, entries, bootcode=None):
    # Collect boot code if there's any...
    if bootcode is not None:
        if os.path.isfile(bootcode):
            with open(bootcode, 'rb') as fh:
                bootcode = fh.read()
            if len(bootcode) > 2 * SECTOR:
                raise SystemExit('Boot code is larger than 1024 bytes!')
        else:
            raise SystemExit('Boot code file does not exists!')
        if not len(entries) or not entries[0].exe:
            raise SystemExit('First file must be AmigaHunk executable!')
    else:
        bootcode = ''

    dir_len = 4 + 8 * len(entries)
    names_len = 0
    names_off = []
    files_len = 0
    files_off = []

    for entry in entries:
        # Determine file names position
        names_off.append(names_len)
        names_len += len(entry.name) + 1
        # Determine file position
        files_off.append(files_len)
        files_len += align(len(entry.data))

    # Calculate starting position of files in the file system image
    files_pos = align(dir_len + names_len) + 2 * SECTOR

    with open(archive, 'wb') as fh:
        boot = BytesIO(bootcode)
        # Overwrite boot block header
        exe_start = sectors(files_pos)
        exe_length = sectors(len(entries[0])) if bootcode else 0
        boot.write(pack('>4s4xHH', b'DOS\0', exe_length * 2, exe_start * 2))
        # Move to the end and pad it so it takes 2 sectors
        boot.seek(0, os.SEEK_END)
        write_pad(boot, 2 * SECTOR)
        # Calculate checksum and fill missing field in boot block
        val = checksum(boot.getvalue())
        boot.seek(4, os.SEEK_SET)
        boot.write(pack('>I', val))
        # Write fixed boot block to file system image
        fh.write(boot.getvalue())

        # Write directory header
        fh.write(pack('>HH', names_len, len(entries)))
        # Write directory entries
        for entry, name_off, file_off in zip(entries, names_off, files_off):
            file_off += files_pos
            start = sectors(file_off)
            if entry.exe:
                start = -start
            fh.write(pack('>HhI', name_off, start, len(entry)))
        # Write file names
        for entry in entries:
            fh.write(entry.name.encode() + b'\0')
        # Finish off directory by aligning it to sector boundary
        write_pad(fh)

        # Write file entries
        for entry in entries:
            fh.write(entry.data)
            write_pad(fh)

        # Complete floppy disk image
        write_pad(fh, FLOPPY)


def extract(archive, patterns, force):
    for pattern in patterns:
        for entry in archive:
            if fnmatch(entry.name, pattern):
                if os.path.exists(entry.name) and not force:
                    print(f'extract: skipping {entry.name} '
                          '- file is present on disk')
                else:
                    print(f'extracting {entry.name}')
                    with open(entry.name, 'wb') as fh:
                        fh.write(entry.data)
                    os.chmod(entry.name, [0o644, 0o755][entry.exe])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Tool for handling Read-Only FileSystem.')
    parser.add_argument(
        'action', metavar='ACTION', type=str,
        choices=['create', 'extract', 'list'],
        help='Action to perform of filesystem image.')
    parser.add_argument(
        '-f', '--force', action='store_true',
        help='If output file exist, the tool will overwrite it.')
    parser.add_argument(
        '-b', '--bootcode', metavar='BOOTCODE', type=str,
        help='Boot code to be embedded into floppy disk representation.')
    parser.add_argument(
        'image', metavar='IMAGE', type=str,
        help='File system image file.')
    parser.add_argument(
        'files', metavar='FILES', type=str, nargs='*',
        help='Files to add to / extract from filesystem image.')
    args = parser.parse_args()

    if args.action == 'create':
        archive = collect(args.files)
        for entry in archive:
            print(entry)
        save(args.image, archive, args.bootcode)
    elif args.action == 'list':
        archive = load(args.image)
        for entry in archive:
            print(entry)
    elif args.action == 'extract':
        archive = load(args.image)
        extract(archive, args.files, args.force)
