import argparse
import subprocess
import os
import shutil
import json
from pathlib import Path

# Thanks Tristan Laan for data extraction functions.

def _calculate_wattage(data: list) -> float:
    return (int(data[1]) / 1000) * (int(data[2]) / 1000) \
        + (int(data[3]) / 1000) * (int(data[4]) / 1000)


def _get_power_profile_data(directory: Path) -> list:
    file = next(directory.glob('power_profile_*.csv'))
    data = []

    with file.open() as f:
        for i, line in enumerate(f):
            if i < 2:
                continue
            csv_data = line.split(',')
            data.append({'timestamp': float(csv_data[0]),
                         'power': _calculate_wattage(csv_data)})

    return data


def _is_mode(line: str):
    if line == 'OpenCL API Calls':
        return True
    if line == 'Kernel Execution':
        return True
    if line == 'Compute Unit Utilization':
        return True
    if line == 'Data Transfer: Host to Global Memory':
        return True
    if line == 'Data Transfer: Kernels to Global Memory':
        return True


def _get_profile_summary_data(file: Path) -> dict:
    data = dict()
    mode = None
    skip_line = False
    with file.open() as f:
        for line in f:
            line = line.strip()
            if skip_line:
                skip_line = False
                continue

            if _is_mode(line):
                mode = line
                data[mode] = []
                skip_line = True
                continue

            if line == '':
                mode = None

            if not mode:
                continue

            csv_data = line.split(',')

            if mode == 'OpenCL API Calls':
                data[mode].append({
                        'name': csv_data[0],
                        'calls': int(csv_data[1]),
                        'time': float(csv_data[2])
                    })

            if mode == 'Kernel Execution':
                data[mode].append({
                        'kernel': csv_data[0],
                        'enqueues': int(csv_data[1]),
                        'time': float(csv_data[2])
                    })

            if mode == 'Compute Unit Utilization':
                data[mode].append({
                        'cu': csv_data[1],
                        'kernel': csv_data[2],
                        'time': float(csv_data[9])
                    })

            if mode == 'Data Transfer: Host to Global Memory':
                data[mode].append({
                        'type': csv_data[1],
                        'transfers': int(csv_data[2]),
                        'speed': float(csv_data[3]),
                        'utilization': float(csv_data[4]),
                        'size': float(csv_data[5]),
                        'time': float(csv_data[6])
                    })


            if mode == 'Data Transfer: Kernels to Global Memory':
                data[mode].append({
                        'interface': csv_data[3],
                        'type': csv_data[4],
                        'transfers': int(csv_data[5]),
                        'speed': float(csv_data[6]),
                        'utilization': float(csv_data[7]),
                        'size': float(csv_data[8])
                    })

    return data


def read_data(directory: Path) -> dict:
    data = dict()
    profile = directory / 'profile_summary.csv'
    try:
        data['power'] = _get_power_profile_data(directory)
    except StopIteration:
        pass
    if profile.exists():
        data.update(_get_profile_summary_data(profile))
    return data


def write_data(data: dict, file: Path):
    with file.open('w') as f:
        json.dump(data, f)


class cd:
    """Context manager for changing the current working directory"""
    def __init__(self, newPath):
        self.newPath = os.path.expanduser(newPath)

    def __enter__(self):
        self.savedPath = os.getcwd()
        os.chdir(self.newPath)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.savedPath)


def main(repeats, count, maxmatches, lengths, dir, filenames, kernel, target):
    for filename in filenames:
        for length in lengths:
            benchmark(repeats, count, maxmatches, length, dir, filename, kernel, target)


def benchmark(repeats, count, maxmatches, length, dir, filename, kernel, target):
    textfilename = f"../{dir}/{filename}"
    fmfilename = f"{textfilename}.fm"
    resultdir = f"../{kernel}_results/{filename}.len{length}"
    testfilename = f"{resultdir}/test.txt"

    if target != "hw":
        env = dict(os.environ, XCL_EMULATION_MODE=target)
    else:
        env = os.environ

    gentestargs = ["../../generate_test_data", textfilename, fmfilename, testfilename, str(count), str(length), str(maxmatches)]
    benchmarkargs = ["../benchmark", fmfilename, f"{kernel}.xclbin", testfilename]
    print(" ".join(gentestargs))
    print(" ".join(benchmarkargs))

    with cd(target):
        shutil.rmtree(resultdir, ignore_errors=True)
        Path(resultdir).mkdir(parents=True, exist_ok=True)

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

            # Execute kernel
            benchmarkproc = subprocess.Popen(benchmarkargs, stdout=subprocess.PIPE, universal_newlines=True, stderr=subprocess.PIPE, env=env)
            _, stderr = benchmarkproc.communicate()
            if stderr:
                print(f">{stderr.strip()}")
            ret = benchmarkproc.poll()
            if ret != 0:
                print("Error benchmarking test data")
                exit(1)

            # Move data to result directory.
            data = read_data(Path("."))
            write_data(data, Path(resultdir) / f"run{n}.json")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-n", "--repeats", help="number of times to repeat each experiment", type=int, required=True)
    parser.add_argument("-c", "--count", help="number of patterns", type=int, required=True)
    parser.add_argument("-m", "--maxmatches", help="maximum number of matches per pattern", type=int, required=True)
    parser.add_argument("-l", "--lengths", help="length of the patterns", type=int, nargs="+", default=[], required=True)
    parser.add_argument("-d", "--dir", help="directory containing FM-indices and original texts (with the same name)", required=True)
    parser.add_argument("-f", "--files", help="FM-index files to benchmark", nargs="+", default=[], required=True)
    parser.add_argument("-k", "--kernel", help="FPGA kernel", required=True)
    parser.add_argument("-t", "--target", help="compilation target", default="sw_emu", required=False)
    args = parser.parse_args()

    main(args.repeats, args.count, args.maxmatches, args.lengths, args.dir, args.files, args.kernel, args.target)
