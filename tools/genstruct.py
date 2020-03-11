#!/usr/bin/env python3

import argparse


def gen_bitmap(name, desc):
    width, height, depth = map(int, desc.split('x'))
    bytesPerRow = ((width + 15) & ~15) // 8
    bplSize = bytesPerRow * height // 2
    for d in range(depth):
        print('__bsschip uint16_t %s_bpl%d[%d];' % (name, d, bplSize))
    print('')
    print('bitmap_t %s_bm = {' % name)
    print('  .width = %d,' % width)
    print('  .height = %d,' % height)
    print('  .depth = %s,' % depth)
    print('  .bytesPerRow = %d,' % bytesPerRow)
    print('  .planes = {')
    for d in range(depth):
        print('    %s_bpl%d,' % (name, d))
    print('  }')
    print('};')
    print('')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Generates a structure based on given description.')
    parser.add_argument(
            '-b', '--bitmap', metavar='BITMAP', type=str,
            help='bitmap_t structure description: WIDTHxHEIGHTxDEPTH.')
    parser.add_argument(
            'name', metavar='NAME', type=str,
            help='Base name of generated symbols.')
    args = parser.parse_args()

    if args.bitmap:
        gen_bitmap(args.name, args.bitmap)
