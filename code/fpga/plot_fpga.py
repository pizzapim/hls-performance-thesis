import argparse
import matplotlib as mpl
mpl.use('TkAgg')
import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt
import numpy as np
import json
import glob
from scipy.integrate import simps


kernel_name_map = {
    "unopt": "reference FPGA kernel",
    "memory": "memory optimized kernel",
    "ndrange": "NDRange optimized kernel",
    "final": "memory and NDRAnge optimized kernel"
}
cmap = plt.get_cmap('viridis')
colors = [cmap(i) for i in np.linspace(0, 1, 6)]


def main(corpora, sizes, lengths, dir, count, save, kernel):
    plt.style.use('seaborn')

    results = parse_results(corpora, sizes, lengths, dir)
    plot_throughput(results, corpora, sizes, lengths, count, save, kernel)
    plot_energy(results, corpora, sizes, lengths, save, kernel)
    print_memory_transfers(results, corpora, sizes, lengths)


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
            transfer_time = sum(map(lambda x: x['time'], data['Data Transfer: Host to Global Memory']))
            energy = get_energy_usage(data)
            result.append((time/1000, energy, transfer_time))

    return result


def get_energy_usage(data):
    xs = list(map(lambda x: x["timestamp"], data["power"]))
    ys = list(map(lambda y: y["power"], data["power"]))

    # Get start and end timestamp of kernel execution.
    start = float(data["timeline"]["START"])
    end = float(data["timeline"]["END"])

    # Find nearest power data points.
    nearest_start = min(range(len(xs)), key=lambda i: abs(xs[i] - start))
    nearest_end = min(range(len(xs)), key=lambda i: abs(xs[i] - end))

    # Find power data points within kernel execution
    kernel_xs = np.array(xs)[nearest_start:nearest_end+1] / 1000
    kernel_ys = np.array(ys)[nearest_start:nearest_end+1]

    # Use Simpson's Rule to integrate and find energy usage.
    return simps(y=kernel_ys, x=kernel_xs)


def plot_throughput(results, corpora, sizes, lengths, count, save, kernel):
    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))
    gs = gridspec.GridSpec(2, 4)
    gs.update(wspace=1.0, hspace=0.5)

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

    plt.suptitle(f"Average throughput for the {kernel_name_map[kernel]}")

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig(f"throughput_{kernel}.png", format="png", dpi=100)
    else:
        plt.show()


def plot_energy(results, corpora, sizes, lengths, save, kernel):
    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))
    gs = gridspec.GridSpec(2, 4)
    gs.update(wspace=1.0, hspace=0.5)

    axes = [plt.subplot(gs[0, 1:3], ), plt.subplot(gs[1, :2]), plt.subplot(gs[1, 2:])]
    for axi, ax in enumerate(axes):
        corpus = corpora[axi]

        means = np.array([[int(np.mean([result[1] for result in results[corpus][size][length]])) for size in sizes] for length in lengths])
        stds = np.array([[np.std([result[1] for result in results[corpus][size][length]]) for size in sizes] for length in lengths])
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

    plt.suptitle(f"Average energy consumption for the {kernel_name_map[kernel]}")

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig(f"energy_{kernel}.png", format="png", dpi=100)
    else:
        plt.show()


def print_memory_transfers(results, corpora, sizes, lengths):
    for length in lengths:
        print(f"{length}", end="")
        for corpus in corpora:
            for size in sizes:
                mean = np.mean([result[2] / 1000 for result in results[corpus][size][length]])
                print(" & \\SI{{{:.2f}}}{{s}}".format(mean), end="")
        print(" \\\\")


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

    main(args.corpora, args.sizes, args.lengths, args.dir, args.count, args.save, args.kernel)
