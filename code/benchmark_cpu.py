import argparse
import subprocess
import os


def main(repeats, count, maxmatches, lengths, dir, filenames):
    for filename in filenames:
        for length in lengths:
            benchmark(repeats, count, maxmatches, length, dir, filename)


def benchmark(repeats, count, maxmatches, length, dir, filename):
    testfilename = f"{dir}/{filename}.cpu{length}.test"
    fmfilename = f"{dir}/{filename}.fm"
    textfilename = f"{dir}/{filename}"
    resultfilename = f"{dir}/{filename}.cpu{length}.result"

    gentestargs = ["./generate_test_data", textfilename, fmfilename, testfilename, str(count), str(length), str(maxmatches)]
    benchmarkargs = ["./benchmark", fmfilename, testfilename]
    print(" ".join(gentestargs))
    print(" ".join(benchmarkargs))

    # Remove result file if it already exists.
    try:
        os.remove(resultfilename)
    except OSError:
        pass

    for n in range(repeats):
        print(f"{n+1}/{repeats}")

        # Create test file.
        gentestproc = subprocess.Popen(gentestargs, universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = gentestproc.communicate()
        if stderr:
            print(f">{stderr.strip()}")
        ret = gentestproc.poll()
        if ret != 0:
            print(f"Error creating test data: {stdout.strip()}")
            exit(1)

        # Create result file.
        with open(resultfilename, "a") as resultfile:
            benchmarkproc = subprocess.Popen(benchmarkargs, stdout=resultfile, universal_newlines=True, stderr=subprocess.PIPE)
            _, stderr = benchmarkproc.communicate()
            if stderr:
                print(f">{stderr.strip()}")
            ret = benchmarkproc.poll()
            if ret != 0:
                print("Error benchmarking test data")
                exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--repeats", help="number of times to repeat each experiment", type=int, required=True)
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-m", "--maxmatches", help="maximum number of matches per pattern", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-d", "--dir", help="directory containing FM-indices and original texts (with the same name)", required=True)
    parser.add_argument("-f", "--files", help="FM-index files to benchmark", nargs="+", default=[], required=True)
    args = parser.parse_args()

    main(args.repeats, args.count, args.maxmatches, args.lengths, args.dir, args.files)
