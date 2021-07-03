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
cmap = plt.get_cmap('cividis')
colors = [cmap(i) for i in np.linspace(0, 1, 5)]


def main(corpora, sizes, lengths,save, count, unoptdir, memorydir, ndrangedir, finaldir, cpudir):
    plt.style.use('seaborn')

    unoptresults = parse_fpga_results(corpora, sizes, lengths, unoptdir)
    memoryresults = parse_fpga_results(corpora, sizes, lengths, memorydir)
    ndrangeresults = parse_fpga_results(corpora, sizes, lengths, ndrangedir)
    finalresults = parse_fpga_results(corpora, sizes, lengths, finaldir)
    cpuresults = parse_cpu_results(corpora, sizes, lengths, cpudir)
    # plot_throughput(cpuresults, unoptresults, memoryresults, ndrangeresults, finalresults, corpora, lengths, save, count)
    # plot_throughput(cpuresults, unoptresults, memoryresults, ndrangeresults, finalresults, corpora, lengths, save, count, log=True)
    # plot_cycles(cpuresults, unoptresults, memoryresults, ndrangeresults, finalresults, corpora, lengths, save, count)
    # print_energy_table(cpuresults, memoryresults, ndrangeresults, finalresults, corpora, sizes, lengths)
    plot_time_energy(cpuresults, finalresults, corpora, lengths, save)


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


def plot_throughput(cpuresults, unoptresults, memoryresults, ndrangeresults, finalresults, corpora, lengths, save, count, log=False):
    width = .15
    keys = np.array([[(corpus, length) for length in lengths] for corpus in corpora]).reshape(len(corpora) * len(lengths), 2)
    labels = [f"({corpus}, {length})" for (corpus, length) in keys]
    xs = np.arange(len(labels))
    size = 20
    _, ax = plt.subplots()

    for i, (results, name) in enumerate([(cpuresults, "cpu"), (unoptresults, "unopt"), (memoryresults, "memory"), (ndrangeresults, "ndrange"), (finalresults, "final")]):
        means = [np.mean(count / np.array(results[corpus][size][int(length)])[:, 0]) for (corpus, length) in keys]
        stds = [np.std(count / np.array(results[corpus][size][int(length)])[:, 0]) for (corpus, length) in keys]
        ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means, width, label=kernel_name_map[name].capitalize(), yerr=stds, color=colors[i])

    ax.set_ylabel("Throughput (patterns matched/s)")
    ax.set_xlabel("Corpus and pattern length")
    ax.set_xticks(xs)
    ax.tick_params(axis="x", rotation=25)
    ax.set_xticklabels(labels)
    ax.legend()
    ax.set_title(f"Throughput of the reference applications and the optimized kernels")
    if log:
        ax.set_yscale('log')

    plt.subplots_adjust(bottom=0.16)

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 5)
        plt.savefig(f"throughput_opts{'_log' if log else ''}.png", format="png", dpi=100)
    else:
        plt.show()


def plot_cycles(cpuresults, unoptresults, memoryresults, ndrangeresults, finalresults, corpora, lengths, save, count):
    size = 20
    fpga_clockspeed = 0.3
    cpu_clockspeed = 3.4
    def calculate_cycles(fpgaresults, corpus, length):
        result = []
        length = int(length)
        for (fpgaresult, cpuresult) in zip(fpgaresults[corpus][size][length], cpuresults[corpus][size][length]):
            fpgacycles = fpgaresult[0] * fpga_clockspeed
            cpucycles = cpuresult[0] * cpu_clockspeed
            result.append(cpucycles / fpgacycles)
        return result
    width = .15
    keys = np.array([[(corpus, length) for length in lengths] for corpus in corpora]).reshape(len(corpora) * len(lengths), 2)
    labels = [f"({corpus}, {length})" for (corpus, length) in keys]
    xs = np.arange(len(labels))
    _, ax = plt.subplots()

    for i, (results, name) in enumerate([(unoptresults, "unopt"), (memoryresults, "memory"), (ndrangeresults, "ndrange"), (finalresults, "final")]):
        means = [np.mean(calculate_cycles(results, corpus, length)) for (corpus, length) in keys]
        stds = [np.std(calculate_cycles(results, corpus, length)) for (corpus, length) in keys]
        ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means, width, label=kernel_name_map[name].capitalize(), yerr=stds, color=colors[i])

    ax.set_ylabel("Cycles CPU / FPGA")
    ax.set_xlabel("Corpus and pattern length")
    ax.set_xticks(xs)
    ax.tick_params(axis="x", rotation=25)
    ax.set_xticklabels(labels)
    ax.legend()
    ax.set_title(f"Clock cycle comparison of the FPGA kernels and the reference CPU application")

    plt.subplots_adjust(bottom=0.16)

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 5)
        plt.savefig("cycles_opts.png", format="png", dpi=100)
    else:
        plt.show()


def print_energy_table(cpuresults, memoryresults, ndrangeresults, finalresults, corpora, sizes, lengths):
    size = 20
    for (results, kernel) in [(memoryresults, "memory"), (ndrangeresults, "NDRange"), (finalresults, "full")]:
        print(f"\\textit{{{kernel}}}", end="")
        for corpus in corpora:
            for length in lengths:
                cpumean = np.mean([result[1] for result in cpuresults[corpus][size][length]])
                fpgamean = np.mean([result[1] for result in results[corpus][size][length]])
                comp = fpgamean / (cpumean / 1000000)
                print(" & ${:.1f}\\times$".format(comp), end="")
        print(" \\\\")


def plot_time_energy(cpuresults, fpgaresults, corpora, lengths, save):
    cmap = plt.get_cmap('viridis')
    colors = [cmap(i) for i in np.linspace(0, 1, 3)]
    size = 20

    def calculate_time(fpgaresults, corpus, length):
        result = []
        length = int(length)
        for (fpgaresult, cpuresult) in zip(fpgaresults[corpus][size][length], cpuresults[corpus][size][length]):
            result.append(fpgaresult[0] / cpuresult[0])
        return result

    def calculate_energy(fpgaresults, corpus, length):
        result = []
        length = int(length)
        for (fpgaresult, cpuresult) in zip(fpgaresults[corpus][size][length], cpuresults[corpus][size][length]):
            result.append(fpgaresult[1] / (cpuresult[1]/1000000))
        return result

    width = .25
    keys = np.array([[(corpus, length) for length in lengths] for corpus in corpora]).reshape(len(corpora) * len(lengths), 2)
    labels = [f"({corpus}, {length})" for (corpus, length) in keys]
    xs = np.arange(len(labels))
    _, ax = plt.subplots()

    means = [np.mean(calculate_time(fpgaresults, corpus, length)) for (corpus, length) in keys]
    stds = [np.std(calculate_time(fpgaresults, corpus, length)) for (corpus, length) in keys]
    ax.bar(xs - width/2, means, width, label=r"time$_{FPGA}$ / time$_{CPU}$", yerr=stds, color=colors[1])

    means = [np.mean(calculate_energy(fpgaresults, corpus, length)) for (corpus, length) in keys]
    stds = [np.std(calculate_energy(fpgaresults, corpus, length)) for (corpus, length) in keys]
    ax.bar(xs + width/2, means, width, label=r"energy$_{FPGA}$ / energy$_{CPU}$", yerr=stds, color=colors[2])

    ax.set_ylabel("Fraction")
    ax.set_xlabel("Corpus and pattern length")
    ax.set_xticks(xs)
    ax.tick_params(axis="x", rotation=25)
    ax.set_xticklabels(labels)
    ax.legend()
    ax.set_title(f"Comparison of time and energy between the reference CPU application and fully optimized FPGA kernel")

    plt.subplots_adjust(bottom=0.16)

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 5)
        plt.savefig("final_time_energy.png", format="png", dpi=100)
    else:
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-t", "--corpora", help="text corpora (without file size)", nargs="+", default=[], required=True)
    parser.add_argument("-s", "--sizes", help="file sizes", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-o", "--save", help="save as PNG", action="store_true", required=False)
    parser.add_argument("-u", "--unoptdir", help="directory containing unoptimized FPGA results", required=True)
    parser.add_argument("-m", "--memorydir", help="directory containing memory-optimized FPGA results", required=True)
    parser.add_argument("-n", "--ndrangedir", help="directory containing NDRange-optimized FPGA results", required=True)
    parser.add_argument("-f", "--finaldir", help="directory containing final FPGA results", required=True)
    parser.add_argument("-p", "--cpudir", help="directory containing CPU results", required=True)
    args = parser.parse_args()

    main(args.corpora, args.sizes, args.lengths, args.save, args.count, args.unoptdir, args.memorydir, args.ndrangedir, args.finaldir, args.cpudir)
