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
    "final": "fully optimized kernel"
}
cmap = plt.get_cmap('cividis')
colors = [cmap(i) for i in np.linspace(0, 1, 6)]


def main(corpora, sizes, lengths, fpgadir, cpudir, save, count, kernel):
    plt.style.use('seaborn')

    fpgaresults = parse_fpga_results(corpora, sizes, lengths, fpgadir)
    cpuresults = parse_cpu_results(corpora, sizes, lengths, cpudir)
    plot_speedup(fpgaresults, cpuresults, corpora, sizes, lengths, save, count, kernel)
    plot_speedup_cycles(fpgaresults, cpuresults, corpora, sizes, lengths, save, kernel)
    print_energy_table(fpgaresults, cpuresults, corpora, sizes, lengths)


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


def plot_speedup(fpgaresults, cpuresults, corpora, sizes, lengths, save, count, kernel):
    def calculate_speedup(corpus, size, length):
        result = []
        for (fpgaresult, cpuresult) in zip(fpgaresults[corpus][size][length], cpuresults[corpus][size][length]):
            fpgatime = count / fpgaresult[0]
            cputime = count / cpuresult[0]
            result.append(cputime / fpgatime)
        return result

    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))
    gs = gridspec.GridSpec(2, 4)
    gs.update(wspace=1.0, hspace=0.5)

    axes = [plt.subplot(gs[0, 1:3], ), plt.subplot(gs[1, :2]), plt.subplot(gs[1, 2:])]
    for axi, ax in enumerate(axes):
        corpus = corpora[axi]
        ax.sharey(axes[0])

        means = np.array([[np.mean(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        stds = np.array([[np.std(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]} characters", yerr=stds[i], color=colors[i])

        if axi in [0, 1]:
            ax.set_ylabel("Throughput CPU / FPGA")
        ax.set_xlabel("Corpus size (MB)")
        ax.set_title(f"\"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        if axi == 0:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0., title="Pattern length")

    plt.suptitle(f"Throughput comparison of the {kernel_name_map[kernel]} and reference CPU application")

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig(f"speedup_{kernel}.png", format="png", dpi=100)
    else:
        plt.show()

        
def plot_speedup_cycles(fpgaresults, cpuresults, corpora, sizes, lengths, save, kernel):
    fpga_clockspeed = 0.3
    cpu_clockspeed = 3.4
    def calculate_speedup(corpus, size, length):
        result = []
        for (fpgaresult, cpuresult) in zip(fpgaresults[corpus][size][length], cpuresults[corpus][size][length]):
            fpgacycles = fpgaresult[0] * fpga_clockspeed
            cpucycles = cpuresult[0] * cpu_clockspeed
            result.append(cpucycles / fpgacycles)
        return result

    width = .1
    labels = [f"{size}MB" for size in sizes]
    xs = np.arange(len(labels))
    gs = gridspec.GridSpec(2, 4)
    gs.update(wspace=1.0, hspace=0.5)

    axes = [plt.subplot(gs[0, 1:3], ), plt.subplot(gs[1, :2]), plt.subplot(gs[1, 2:])]
    for axi, ax in enumerate(axes):
        corpus = corpora[axi]
        ax.sharey(axes[0])

        means = np.array([[np.mean(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        stds = np.array([[np.std(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]} characters", yerr=stds[i], color=colors[i+3])

        if axi in [0, 1]:
            ax.set_ylabel("Cycles CPU / FPGA")
        ax.set_xlabel("Corpus size (MB)")
        ax.set_title(f"\"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        if axi == 0:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0., title="Pattern length")

    plt.suptitle(f"Clock cycle comparison of the {kernel_name_map[kernel]} and the reference CPU application")

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig(f"speedup_cycles_{kernel}.png", format="png", dpi=100)
    else:
        plt.show()


def print_energy_table(fpgaresults, cpuresults, corpora, sizes, lengths):
    for length in lengths:
        print(f"{length}", end="")
        for corpus in corpora:
            for size in sizes:
                cpumean = np.mean([result[1] for result in cpuresults[corpus][size][length]])
                fpgamean = np.mean([result[1] for result in fpgaresults[corpus][size][length]])
                comp = fpgamean / (cpumean / 1000000)
                print(" & ${:.1f}\\times$".format(comp), end="")
        print(" \\\\")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-f", "--fpgadir", help="directory containing FPGA results", required=True)
    parser.add_argument("-d", "--cpudir", help="directory containing CPU results", required=True)
    parser.add_argument("-t", "--corpora", help="text corpora (without file size)", nargs="+", default=[], required=True)
    parser.add_argument("-s", "--sizes", help="file sizes", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-k", "--kernel", help="kernel", required=True)
    parser.add_argument("-o", "--save", help="save as PNG", action="store_true", required=False)
    args = parser.parse_args()

    main(args.corpora, args.sizes, args.lengths, args.fpgadir, args.cpudir, args.save, args.count, args.kernel)
