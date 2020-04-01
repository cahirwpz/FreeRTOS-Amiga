#!/usr/bin/env python3

import os.path
import argparse
import wave


def convert(path, name):
    if wav.getnchannels() > 1:
        raise SystemExit('Only mono sound files supported!')
    if wav.getframerate() > 22050:
        raise SystemExit('Sample rate must not be greater that 22050Hz!')
    if wav.getcomptype() != 'NONE':
        raise SystemExit('Compressed sound files not supported!')
    if wav.getsampwidth() not in [1, 2]:
        raise SystemExit('Only 8-bit or 16-bit samples are supported!')

    n = wav.getnframes()
    data = wav.readframes(n)
    samples = [data[i] - 128 for i in range(0, n, wav.getsampwidth())]
    if n & 1:
        samples.append(samples[-1])
        n += 1

    print('static alignas(uint16_t) __datachip int8_t _%s_data[] = {' % name,
          end='')
    for i in range(0, n, 1):
        if i % 16 == 0:
            print('\n ', end='')
        print(' %d,' % samples[i], end='')
        if i == n - 1:
            print()
    print('};')
    print()
    print('Sound_t %s = {' % name)
    print('  .length = %d,' % n)
    print('  .rate = %d,' % wav.getframerate())
    print('  .samples = _%s_data' % name)
    print('};')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Convert WAV sound file to C representation.')
    parser.add_argument('--name', metavar='NAME', type=str,
                        help='Base name of C object.')
    parser.add_argument('path', metavar='PATH', type=str,
                        help='PC Screen Font file.')
    args = parser.parse_args()

    if not os.path.isfile(args.path):
        raise SystemExit('Input file does not exists!')

    with wave.open(args.path, 'rb') as wav:
        convert(wav, args.name)
