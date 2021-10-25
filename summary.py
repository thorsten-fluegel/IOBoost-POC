#!/bin/env python3
import argparse
import json
import os
import re
import sys

import matplotlib.pyplot as plt
import numpy as np


def read_perf_result(path):
    with open(path, 'r') as f:
        for line in f.readlines():
            m = re.match(r'hashing .* took ([\d.]+)s', line)
            if m:
                return m.group(1)
    return None


def get_results(folder):
    results = {}
    for path in os.listdir(folder):
        m = re.match(r'(.+)_(.+)_(\d+)', path)
        if m:
            corpus = m.group(1)
            strategy = m.group(2)
            run = m.group(3)
            result = read_perf_result(os.path.join(folder, path))
            if result is not None:
                results.setdefault(corpus, {}).setdefault(strategy, {})[run] = result
    return results


def result_statistics(results):
    return {corpus: {strategy: {'mean': np.mean([float(v) for (k, v) in results[corpus][strategy].items()]),
                                'std': np.std([float(v) for (k, v) in results[corpus][strategy].items()], ddof=1),
                                'median': np.median([float(v) for (k, v) in results[corpus][strategy].items()]),
                                'amin': np.amin([float(v) for (k, v) in results[corpus][strategy].items()]),
                                'amax': np.amax([float(v) for (k, v) in results[corpus][strategy].items()])}
                     for strategy in results[corpus]}
            for corpus in results}


def scatterplot(results, filename):
    fig, plots = plt.subplots(len(results), 2, figsize=(19.20, 10.80))
    plots[0][0].set_title('First 64kB')
    plots[0][1].set_title('Full file')
    row = 0
    for corpus in results:
        plots[row][0].annotate(corpus, xy=(-0.15, 0.5), xycoords='axes fraction',
                               xytext=(0, 0), textcoords='offset points',
                               size='medium', ha='center', va='center', rotation=90)
        plots[row][0].set_xlabel('run')
        plots[row][1].set_xlabel('run')
        plots[row][0].set_ylabel('time [s]')
        plots[row][1].set_ylabel('time [s]')
        for strategy in results[corpus]:
            p = plots[row][0] if strategy.endswith('64') else plots[row][1]
            data = [(int(k), float(v)) for (k, v) in results[corpus][strategy].items()]
            p.scatter(*zip(*data), label=strategy)
        plots[row][0].legend()
        plots[row][1].legend()
        row += 1
    fig.savefig(filename)


def boxplot(results, filename):
    fig, plots = plt.subplots(len(results), 2, figsize=(19.20, 10.80))
    plots[0][0].set_title('First 64kB')
    plots[0][1].set_title('Full file')
    row = 0
    for corpus in results:
        plots[row][0].annotate(corpus, xy=(-0.15, 0.5), xycoords='axes fraction',
                               xytext=(0, 0), textcoords='offset points',
                               size='medium', ha='center', va='center', rotation=90)
        plots[row][0].set_ylabel('time [s]')
        plots[row][1].set_ylabel('time [s]')
        data64 = []
        data = []
        for strategy in results[corpus]:
            d = data64 if strategy.endswith('64') else data
            d.append((strategy, [float(v) for (k, v) in results[corpus][strategy].items()]))
        props = dict(facecolor='lightblue')
        plots[row][0].boxplot(list(zip(*data64))[1], labels=list(zip(*data64))[0], patch_artist=True, boxprops=props)
        plots[row][1].boxplot(list(zip(*data))[1], labels=list(zip(*data))[0], patch_artist=True, boxprops=props)
        row += 1
    fig.savefig(filename)


def print_factors(vals):
    v = sorted(vals, key=lambda e: e[1])
    print(f'fastest: {v[0][0]}, {v[0][1]:.2f}s')
    for i in range(1, len(v)):
        print(f'{v[i][0]}: {v[i][1]:.2f}s, {v[i][1] / v[0][1]:.2f}x slower')


def print_findings(stats):
    for corpus in stats:
        vals64 = []
        vals = []
        for strategy in stats[corpus]:
            v = vals64 if strategy.endswith('64') else vals
            v.append((strategy, stats[corpus][strategy]['mean']))
        print(f'{corpus}:')
        print_factors(vals64)
        print_factors(vals)
        print('')


def main():
    parser = argparse.ArgumentParser(description='Summarize performance tests.')
    parser.add_argument('folder', type=str, nargs='?', default='out',
                        help='the output folder containing performance results '
                             'that will be summarized')
    parser.add_argument('-out', metavar='name', type=str, nargs='?', default='summary',
                        help='the base file name for the summaries of the performance results '
                             'that will be written to the specified folder')
    args = parser.parse_args()

    if not os.path.isdir(args.folder):
        print('error: input folder not found', file=sys.stderr)
        exit(1)

    results = get_results(args.folder)
    scatterplot(results, os.path.join(args.folder, f'{args.out}_runs.svg'))
    boxplot(results, os.path.join(args.folder, f'{args.out}_box.svg'))
    stats = result_statistics(results)
    with open(os.path.join(args.folder, f'{args.out}_stats.json'), 'w') as f:
        json.dump(stats, fp=f, indent=4)
    print_findings(stats)


if __name__ == '__main__':
    main()
