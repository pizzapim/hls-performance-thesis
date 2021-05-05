import argparse
import matplotlib as mpl
mpl.use('TkAgg')
from matplotlib import ticker
import matplotlib.pyplot as plt
import numpy as np


def main(corpora, sizes, lengths, dir, count):
    results = parse_results(corpora, sizes, lengths, dir)
    plot_results(results, corpora, sizes, lengths, count)


def parse_results(corpora, sizes, lengths, dir):
    results = dict()

    for corpus in corpora:
        results[corpus] = dict()
        for size in sizes:
            results[corpus][size] = dict()
            for length in lengths:
                results[corpus][size][length] = parse_result(corpus, size, length, dir)

    return results


def parse_result(corpus, size, length, dir):
    def parse_line(line):
        [range_time, index_time, total_matches] = line.split(" ")
        return (float.fromhex(range_time), float.fromhex(index_time), int(total_matches))

    filename = f"{dir}/{corpus}.{size}MB.cpu{length}.result"

    with open(filename, "r") as f:
        data = list(map(parse_line, f.read().splitlines()))

    return data


def plot_results(results, corpora, sizes, lengths, count):
    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))

    _, axes = plt.subplots(nrows=1, ncols=len(corpora), figsize=(3,3))
    for axi in range(len(corpora)):
        ax = axes[axi]
        corpus = corpora[axi]

        means = np.array([[count / np.mean([result[0] + result[1] for result in results[corpus][size][length]]) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            bar = ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]}")
            # ax.bar_label(bar, padding=3)

        ax.set_ylabel("Throughput (patterns matched/s)")
        ax.set_title(f"CPU throughput of the \"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        ax.legend(title="Pattern length")
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y, _: str(int(y/1000)) + 'K'))

    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-d", "--dir", help="directory containing results", required=True)
    parser.add_argument("-t", "--corpora", help="text corpora (without file size)", nargs="+", default=[], required=True)
    parser.add_argument("-s", "--sizes", help="file sizes", type=int, nargs="+", default=[], required=True)
    args = parser.parse_args()

    main(args.corpora, args.sizes, args.lengths, args.dir, args.count)
