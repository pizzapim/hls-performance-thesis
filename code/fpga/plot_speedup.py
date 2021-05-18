import argparse
import matplotlib as mpl
mpl.use('TkAgg')
import matplotlib.gridspec as gridspec
import matplotlib.pyplot as plt
import numpy as np
import json
import glob


def main(corpora, sizes, lengths, fpgadir, cpudir, count, kernel, save):
    fpgaresults = parse_fpga_results(corpora, sizes, lengths, fpgadir)
    cpuresults = parse_cpu_results(corpora, sizes, lengths, cpudir)
    plot_speedup(fpgaresults, cpuresults, corpora, sizes, lengths, count, kernel, save)
    plot_speedup_cycles(fpgaresults, cpuresults, corpora, sizes, lengths, count, kernel, save)


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
            result.append((time/1000,))

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


def plot_speedup(fpgaresults, cpuresults, corpora, sizes, lengths, count, kernel, save):
    def calculate_speedup(corpus, size, length):
        result = []
        for (fpgaresult, cpuresult) in zip(fpgaresults[corpus][size][length], cpuresults[corpus][size][length]):
            fpgatime = fpgaresult[0]
            cputime = cpuresult[0] + cpuresult[1]
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

        means = np.array([[np.mean(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        stds = np.array([[np.std(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]} characters", yerr=stds[i])

        if axi in [0, 1]:
            ax.set_ylabel("Speedup")
        ax.set_xlabel("Corpus size (MB)")
        ax.set_title(f"\"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        if axi == 0:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0., title="Pattern length")

    plt.suptitle('Speedup of patterns matched per second for FPGA compared to CPU')

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig("speedup_fpga.png", format="png", dpi=100)
    else:
        plt.show()

        
def plot_speedup_cycles(fpgaresults, cpuresults, corpora, sizes, lengths, count, kernel, save):
    fpga_clockspeed = 0.3
    cpu_clockspeed = 3.4
    def calculate_speedup(corpus, size, length):
        result = []
        for (fpgaresult, cpuresult) in zip(fpgaresults[corpus][size][length], cpuresults[corpus][size][length]):
            fpgatime = fpgaresult[0] * fpga_clockspeed
            cputime = (cpuresult[0] + cpuresult[1]) * cpu_clockspeed
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

        means = np.array([[np.mean(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        stds = np.array([[np.std(calculate_speedup(corpus, size, length)) for size in sizes] for length in lengths])
        for i in range(len(lengths)):
            ax.bar(xs - (width*len(lengths))/2 + i * width + width/2, means[i], width, label=f"{lengths[i]} characters", yerr=stds[i])

        if axi in [0, 1]:
            ax.set_ylabel("Speedup")
        ax.set_xlabel("Corpus size (MB)")
        ax.set_title(f"\"{corpus}\" corpus");
        ax.set_xticks(xs)
        ax.set_xticklabels(labels)
        if axi == 0:
            ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0., title="Pattern length")

    plt.suptitle('Speedup of patterns matched per clock cycle for FPGA compared to CPU')

    if save:
        figure = plt.gcf()
        figure.set_size_inches(9, 7)
        plt.savefig("speedup_cycles_fpga.png", format="png", dpi=100)
    else:
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-f", "--fpgadir", help="directory containing FPGA results", required=True)
    parser.add_argument("-d", "--cpudir", help="directory containing CPU results", required=True)
    parser.add_argument("-t", "--corpora", help="text corpora (without file size)", nargs="+", default=[], required=True)
    parser.add_argument("-s", "--sizes", help="file sizes", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-k", "--kernel", help="kernel", required=True)
    parser.add_argument("-o", "--save", help="save as SVG", action="store_true", required=False)
    args = parser.parse_args()

    main(args.corpora, args.sizes, args.lengths, args.fpgadir, args.cpudir, args.count, args.kernel, args.save)
