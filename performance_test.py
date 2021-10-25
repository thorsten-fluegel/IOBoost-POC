#!/bin/env python3
import argparse
import os.path
import subprocess
import sys


# this function makes sure that the file content has to be read from disk the next time it is accessed
def empty_standby_list():
    try:
        subprocess.run(['rammap', '-Et'])
    except FileNotFoundError:
        print('error: rammap not found, please download it from '
              'https://docs.microsoft.com/en-us/sysinternals/downloads/rammap', file=sys.stderr)
        exit(3)
    except OSError as e:
        if e.winerror == 740:
            print('error: this script requires elevation for emptying the standby list', file=sys.stderr)
            exit(4)


def run_tests(exe_path, folders, strategies, count, out_dir):
    if not os.path.isdir(out_dir):
        os.mkdir(out_dir)
    for folder in folders:
        print(f'processing {folder}')
        safe_folder = folder.translate(str.maketrans(' \\/:*?\"<>|', '__________'))
        # prepare the filesystem, which won't be affected by emptying the standby list
        subprocess.run([exe_path, '-f', folder], capture_output=True)
        # run the actual tests
        for strategy in strategies:
            print(f'strategy: {strategy}')
            for c in range(1, count+1):
                print('.', end='', flush=True)
                empty_standby_list()
                with open(f'{out_dir}/{safe_folder}_{strategy}_{c}', 'bw') as f:
                    print([exe_path, f'-{strategy}', folder])
                    subprocess.run([exe_path, f'-{strategy}', folder], stdout=f, stderr=subprocess.STDOUT)
            print('')


def main():
    parser = argparse.ArgumentParser(description='Run performance tests.')
    parser.add_argument('folders', metavar='folders', type=str, nargs='+',
                        help='the folders that will be processed')
    parser.add_argument('-count', metavar='N', type=int, default=1,
                        help='how many times each folder will be processed with each iteration strategy')
    parser.add_argument('-out', metavar='out', type=str, default='out',
                        help='the folder where the performance results will be written')
    parser.add_argument('-a', dest='strategies', action='append_const', const='a',
                        help='process entire files in alphabetic order')
    parser.add_argument('-a64', dest='strategies', action='append_const', const='a64',
                        help='process first 64kB of files in alphabetic order')
    parser.add_argument('-r', dest='strategies', action='append_const', const='r',
                        help='process entire files in random order')
    parser.add_argument('-r64', dest='strategies', action='append_const', const='r64',
                        help='process first 64kB of files in random order')
    parser.add_argument('-f', dest='strategies', action='append_const', const='f',
                        help='process entire files in filesystem order')
    parser.add_argument('-f64', dest='strategies', action='append_const', const='f64',
                        help='process first 64kB of files in filesystem order')
    parser.add_argument('-c', dest='strategies', action='append_const', const='c',
                        help='process entire files in cluster order')
    parser.add_argument('-c64', dest='strategies', action='append_const', const='c64',
                        help='process first 64kB of files in cluster order')
    args = parser.parse_args()

    if not args.strategies:
        print('error: please provide at least one iteration strategy', file=sys.stderr)
        exit(1)

    paths = ['FileIterator.exe', 'x64/Release/FileIterator.exe']
    for path in paths:
        if os.path.isfile(path):
            exe_path = path
    if not exe_path:
        print('error: file iterator executable not found', file=sys.stderr)
        exit(2)

    run_tests(exe_path, args.folders, args.strategies, args.count, args.out)


if __name__ == '__main__':
    main()
