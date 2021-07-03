import argparse
import matplotlib as mpl
mpl.use('TkAgg')
import matplotlib.pyplot as plt
import numpy as np
import json
import glob
from scipy.integrate import simps


kernel_name_map = {
    "cpu": "reference CPU application",
    "unopt": "reference FPGA kernel",
    "memory": "memory optimized kernel",
    "ndrange": "NDRange optimized kernel",
    "final": "fully optimized kernel"
}
cmap = plt.get_cmap('viridis')
colors = [cmap(i) for i in np.linspace(0, 1, 3)]


def main(corpora, sizes, lengths, optdir, unoptdir, cpudir, save, count, kernel):
    plt.style.use('seaborn')

    optresults = parse_fpga_results(corpora, sizes, lengths, optdir)
    unoptresults = parse_fpga_results(corpora, sizes, lengths, unoptdir)
    cpuresults = parse_cpu_results(corpora, sizes, lengths, cpudir)
    plot_throughput(optresults, unoptresults, cpuresults, corpora, lengths, save, count, kernel)


def parse_fpga_results(corpora, sizes, lengths, dir):
    results = dict()

    for corpus in corpora:
        results[corpus] = dict()
        for size in sizes:
            results[corpus][size] = dict()
            for length in lengths:
                results[corpus][size][length] = parse_fpga_result(corpus, size, length, dir)

    return results


def parse_fpga_result(corpus, size, length, dir):
    resultdir = f"{dir}/{corpus}.{size}MB.len{length}"
    result = []
    for filepath in glob.iglob(f"{resultdir}/*"):
        with open(filepath, "r") as f:
            data = json.load(f)
            time = data['Kernel Execution'][0]['time']
            energy = get_energy_usage(data)
            result.append((time/1000, energy))

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


def parse_cpu_results(corpora, sizes, lengths, dir):
    results = dict()

    for corpus in corpora:
        results[corpus] = dict()
        for size in sizes:
            results[corpus][size] = dict()
            for length in lengths:
                results[corpus][size][length] = parse_cpu_result(corpus, size, length, dir)

    return results


def parse_cpu_result(corpus, size, length, dir):
    def parse_line(line):
        [range_time, index_time, total_matches] = line.split(" ")
        return (float.fromhex(range_time), float.fromhex(index_time), int(total_matches))

    filename = f"{dir}/{corpus}.{size}MB.cpu{length}.result"

    with open(filename, "r") as f:
        data = list(map(parse_line, f.read().splitlines()))

    return data


def plot_throughput(optresults, unoptresults, cpuresults, corpora, lengths, save, count, kernel):
    width = .2
    keys = np.array([[(corpus, length) for length in lengths] for corpus in corpora]).reshape(len(corpora) * len(lengths), 2)
    labels = [f"({corpus}, {length})" for (corpus, length) in keys]
    xs = np.arange(len(labels))
    size = 20
    _, ax = plt.subplots()

    for i, (results, name) in enumerate([(cpuresults, "cpu"), (unoptresults, "unopt"), (optresults, kernel)]):
        means = [np.mean(count / np.array(results[corpus][size][int(length)])[:, 0]) for (corpus, length) in keys]
        stds = [np.std(count / np.array(results[corpus][size][int(length)])[:, 0]) for (corpus, length) in keys]
        ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means, width, label=kernel_name_map[name].capitalize(), yerr=stds, color=colors[i])

    ax.set_ylabel("Throughput (patterns matched/s)")
    ax.set_xlabel("Corpus and pattern length")
    ax.set_xticks(xs)
    ax.tick_params(axis="x", rotation=25)
    ax.set_xticklabels(labels)
    ax.legend()
    ax.set_title(f"Throughput comparison between reference applications and {kernel_name_map[kernel]}")

    plt.subplots_adjust(bottom=0.16)

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 5)
        plt.savefig(f"throughput_comparison_{kernel}.png", format="png", dpi=100)
    else:
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-f", "--optdir", help="directory containing optimized FPGA results", required=True)
    parser.add_argument("-u", "--unoptdir", help="directory containing unoptimized FPGA results", required=True)
    parser.add_argument("-d", "--cpudir", help="directory containing CPU results", required=True)
    parser.add_argument("-t", "--corpora", help="text corpora (without file size)", nargs="+", default=[], required=True)
    parser.add_argument("-s", "--sizes", help="file sizes", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-k", "--kernel", help="kernel", required=True)
    parser.add_argument("-o", "--save", help="save as PNG", action="store_true", required=False)
    args = parser.parse_args()

    main(args.corpora, args.sizes, args.lengths, args.optdir, args.unoptdir, args.cpudir, args.save, args.count, args.kernel)
