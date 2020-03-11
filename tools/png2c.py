#!/usr/bin/env python3

from PIL import Image
import math
import argparse
import os.path
from array import array
from collections import namedtuple


def c2p(pix, width, height, depth):
    data = array('H')
    padding = array('B', [0 for _ in range(16 - (width & 15))])

    for offset in range(0, width * height, width):
        row = pix[offset:offset + width]
        if width & 15:
            row.extend(padding)
        for p in range(depth):
            bits = [(byte >> p) & 1 for byte in row]
            for i in range(0, width, 16):
                word = 0
                for j in range(16):
                    word = word * 2 + bits[i + j]
                data.append(word)

    return data


def bitword(word):
    return '0b' + ''.join([str(int(bool(word & (1 << k))))
                           for k in range(15, -1, -1)])


Bitmap = namedtuple('Bitmap', 'bpl width height depth')


def save_sprite(name, bm):
    print('__datachip sprdat_t %s_spr_data0[] = {' % name)
    print('  SPRHDR(0, 0, 0, %d),' % bm.height)
    for y in range(bm.height):
        print('  {%s, %s},' % (bitword(bm.bpl[y*2+0]), bitword(bm.bpl[y*2+1])))
    print('  SPREND()')
    print('};')
    print('')
    print('sprite_t %s_spr = {' % name)
    print('  .height = %d,' % bm.height)
    print('  .data = %s_spr_data0' % name)
    print('};')


def save_bitmap(name, bm, mask):
    bytesPerRow = ((bm.width + 15) & ~15) // 8
    wordsPerRow = bytesPerRow // 2
    for d in range(bm.depth):
        i = d * wordsPerRow
        print('__datachip uint16_t %s_bpl%d[] = {' % (name, d))
        for y in range(bm.height):
            words = ['0x%04x' % bm.bpl[i + x]
                     for x in range(wordsPerRow)]
            print('  %s,' % ','.join(words))
            i += wordsPerRow * bm.depth
        print('};')
    if mask:
        print('')
        print('__datachip uint16_t %s_mask[] = {' % name)
        i = 0
        for y in range(bm.height):
            maskRow = [0 for _ in range(wordsPerRow)]
            for d in range(bm.depth):
                for x in range(wordsPerRow):
                    maskRow[x] |= bm.bpl[i + x]
                i += wordsPerRow
            print('  %s,' % ','.join(['0x%04x' % w for w in maskRow]))
        print('};')
    print('')
    print('bitmap_t %s_bm = {' % name)
    print('  .width = %d,' % bm.width)
    print('  .height = %d,' % bm.height)
    print('  .depth = %s,' % bm.depth)
    if mask:
        print('  .flags = BM_HASMASK,')
    print('  .bytesPerRow = %d,' % bytesPerRow)
    if mask:
        print('  .mask = %s_mask,' % name)
    print('  .planes = {')
    for d in range(bm.depth):
        print('    %s_bpl%d,' % (name, d))
    print('  }')
    print('};')


def save_palette(name, pal):
    print('palette_t %s_pal = {' % name)
    print('  .count = %d,' % len(pal))
    print('  .colors = {')
    for r, g, b in pal:
        print('    0x%x%x%x,' % (r >> 4, g >> 4, b >> 4))
    print('  }')
    print('};')


def convert(path, name, is_sprite, do_palette, do_mask):
    im = Image.open(path)

    if im.mode not in ['1', 'L', 'P']:
        raise SystemExit('Only 8-bit images supported.')

    pix = array('B', im.getdata())
    pal = im.getpalette()

    width, height = im.size
    colors = im.getextrema()[1] + 1
    depth = int(math.ceil(math.log(colors, 2)))

    if depth > 5:
        raise SystemExit('Images with more than 32-colors are not supported!')

    if is_sprite:
        if width != 16:
            raise SystemExit('Sprite must have width of exactly 16 pixels!')
        if depth != 2:
            raise SystemExit('Sprite must have 4 colors!')

    if pal is None:
        grays = [int(i * 255.0 / colors) for i in range(colors)]
        cmap = [(g, g, g) for g in grays]
    else:
        cmap = [pal[i * 3:(i + 1) * 3] for i in range(colors)]
    bpls = c2p(pix, width, height, depth)

    bm = Bitmap(bpls, width, height, depth)
    if is_sprite:
        save_sprite(name, bm)
    else:
        save_bitmap(name, bm, do_mask)
    if do_palette:
        print('')
        save_palette(name, cmap)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Converts an image to bitmap.')
    parser.add_argument(
        '-s', '--sprite', action='store_true',
        help='Generate sprite instead of bitmap.')
    parser.add_argument(
        '-m', '--mask', action='store_true',
        help='Generate blitter mask for the image.')
    parser.add_argument(
        '-p', '--palette', action='store_true',
        help='Export palette as well.')
    parser.add_argument(
        '-n', '--name', metavar='NAME', type=str,
        help='Base name of C objects.')
    parser.add_argument('path', metavar='PATH', type=str,
                        help='Input image filename.')
    args = parser.parse_args()

    if not args.name:
        path = os.path.splitext(os.path.basename(args.path))[0]
        name = path.replace('-', '_')
    else:
        name = args.name

    convert(args.path, name, args.sprite, args.palette, args.mask)
