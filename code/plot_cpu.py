import argparse
import matplotlib as mpl
mpl.use('TkAgg')
import matplotlib.gridspec as gridspec
from matplotlib import ticker
import matplotlib.pyplot as plt
import numpy as np


cmap = plt.get_cmap('viridis')
colors = [cmap(i) for i in np.linspace(0, 1, 6)]


def main(corpora, sizes, lengths, dir, count, save):
    plt.style.use('seaborn')

    results = parse_results(corpora, sizes, lengths, dir)
    plot_throughput(results, corpora, sizes, lengths, count, save)
    plot_energy(results, corpora, sizes, lengths, save)
    print_match_count_table(results, corpora, sizes, lengths, count)


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


def plot_throughput(results, corpora, sizes, lengths, count, save):
    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))
    gs = gridspec.GridSpec(2, 4)
    gs.update(wspace=0.5, hspace=0.5)

    axes = [plt.subplot(gs[0, 1:3], ), plt.subplot(gs[1, :2]), plt.subplot(gs[1, 2:])]
    for axi, ax in enumerate(axes):
        corpus = corpora[axi]
        ax.sharey(axes[0])

        means = np.array([[int(np.mean([count / result[0] for result in results[corpus][size][length]])) for size in sizes] for length in lengths])
        stds = np.array([[np.std([count / result[0] for result in results[corpus][size][length]]) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]} characters", yerr=stds[i], color=colors[i])

        if axi in [0, 1]:
            ax.set_ylabel("Throughput (patterns matched/s)")
        ax.set_xlabel("Corpus size (MB)")
        ax.set_title(f"\"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        if axi == 0:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0., title="Pattern length")
        ax.yaxis.set_major_formatter(ticker.FuncFormatter(lambda y, _: str(int(y/1000)) + ("K" if y != 0 else "")))

    plt.suptitle("Average throughput for the reference CPU application")

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig("throughput_cpu.png", format="png", dpi=100)
    else:
        plt.show()


def plot_energy(results, corpora, sizes, lengths, save):
    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))
    gs = gridspec.GridSpec(2, 4)
    gs.update(wspace=0.5, hspace=0.5)


    axes = [plt.subplot(gs[0, 1:3], ), plt.subplot(gs[1, :2]), plt.subplot(gs[1, 2:])]
    for axi, ax in enumerate(axes):
        corpus = corpora[axi]

        means = np.array([[np.mean([result[1]/1000000 for result in results[corpus][size][length]]) for size in sizes] for length in lengths])
        stds = np.array([[np.std([result[1]/1000000 for result in results[corpus][size][length]]) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]} characters", yerr=stds[i], color=colors[i+3])

        if axi in [0, 1]:
            ax.set_ylabel("Energy consumption (J)")
        ax.set_xlabel("Corpus size (MB)")
        ax.set_title(f"\"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        if axi == 0:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0., title="Pattern length")

    plt.suptitle("Average energy consumption for the reference CPU application")

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig("energy_cpu.png", format="png", dpi=100)
    else:
        plt.show()

    
def print_match_count_table(results, corpora, sizes, lengths, count):
    for length in lengths:
        print(f"{length}", end="")
        for corpus in corpora:
            for size in sizes:
                mean = np.mean([result[2] / count for result in results[corpus][size][length]])
                print(f" & {round(mean)}", end="")
        print(" \\\\")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-d", "--dir", help="directory containing results", required=True)
    parser.add_argument("-t", "--corpora", help="text corpora (without file size)", nargs="+", default=[], required=True)
    parser.add_argument("-s", "--sizes", help="file sizes", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-o", "--save", help="save as SVG", action="store_true", required=False)
    args = parser.parse_args()

    main(args.corpora, args.sizes, args.lengths, args.dir, args.count, args.save)
