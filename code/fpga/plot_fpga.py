import argparse
import matplotlib as mpl
mpl.use('TkAgg')
import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt
import numpy as np
import json
import glob


def main(corpora, sizes, lengths, dir, count, kernel, save):
    results = parse_results(corpora, sizes, lengths, dir)
    plot_throughput(results, corpora, sizes, lengths, count, kernel, save)


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
    resultdir = f"{dir}/{corpus}.{size}MB.len{length}"
    result = []
    for filepath in glob.iglob(f"{resultdir}/*"):
        with open(filepath, "r") as f:
            data = json.load(f)
            time = data['Kernel Execution'][0]['time']
            result.append((time/1000,))

    return result


def plot_throughput(results, corpora, sizes, lengths, count, kernel, save):
    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))
    gs = gridspec.GridSpec(2, 4)
    gs.update(wspace=1.0, hspace=0.5)

    axes = [plt.subplot(gs[0, 1:3], ), plt.subplot(gs[1, :2]), plt.subplot(gs[1, 2:])]
    for axi, ax in enumerate(axes):
        corpus = corpora[axi]

        means = np.array([[int(np.mean([count / result[0] for result in results[corpus][size][length]])) for size in sizes] for length in lengths])
        stds = np.array([[np.std([count / result[0] for result in results[corpus][size][length]]) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            bar = ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]} characters", yerr=stds[i])

        if axi in [0, 1]:
            ax.set_ylabel("Throughput (patterns matched/s)")
        ax.set_xlabel("Corpus size (MB)")
        ax.set_title(f"FPGA throughput of the \"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        if axi == 0:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0., title="Pattern length")

    plt.suptitle(kernel)

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig("throughput_fpga.png", format="png", dpi=100)
    else:
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-d", "--dir", help="directory containing results", required=True)
    parser.add_argument("-t", "--corpora", help="text corpora (without file size)", nargs="+", default=[], required=True)
    parser.add_argument("-s", "--sizes", help="file sizes", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-k", "--kernel", help="kernel", required=True)
    parser.add_argument("-o", "--save", help="save as SVG", action="store_true", required=False)
    args = parser.parse_args()

    main(args.corpora, args.sizes, args.lengths, args.dir, args.count, args.kernel, args.save)
