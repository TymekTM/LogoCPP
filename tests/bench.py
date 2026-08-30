#!/usr/bin/env python3
"""Benchmark harness for LogoCPP performance work.

Times the three canonical inputs (recursive fractal tree, depth 8/12/15) in
benchmark mode (-b: parsing and execution only, no file I/O). Repeats each
measurement and keeps the best run to minimize scheduler noise.

Usage:
    python tests/bench.py                      # print current numbers
    python tests/bench.py --save baseline.json # store as reference
    python tests/bench.py --compare baseline.json
"""

import argparse
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
DEFAULT_EXE = os.path.join(ROOT, "..", "x64", "Release", "LogoCPP.exe")

# (name, input file, iterations per timed run, canvas size)
BENCHMARKS = [
    ("tree_n8", "performance_test.logo", 20000, 500),
    ("tree_n12", "performance_test_heavy.logo", 500, 500),
    ("tree_n15", "performance_test_super_heavy.logo", 50, 500),
]

REPEATS = 5


def measure(exe, logo_file, iterations, size):
    result = subprocess.run(
        [exe, "-i", os.path.join(ROOT, logo_file), "-o", "bench_out.txt",
         "-s", str(size), "-b", str(iterations)],
        capture_output=True, text=True, cwd=ROOT,
    )
    match = re.search(r"Average per iteration: ([\d.]+) ms", result.stdout)
    if not match:
        raise RuntimeError(f"benchmark failed for {logo_file}: {result.stdout} {result.stderr}")
    return float(match.group(1))


def main():
    parser = argparse.ArgumentParser(description="LogoCPP benchmark harness")
    parser.add_argument("--exe", default=DEFAULT_EXE)
    parser.add_argument("--save", metavar="JSON", help="write results to file")
    parser.add_argument("--compare", metavar="JSON", help="compare against stored results")
    args = parser.parse_args()

    exe = os.path.abspath(args.exe)
    if not os.path.exists(exe):
        print(f"ERROR: interpreter not found: {exe}")
        return 2

    results = {}
    for name, logo_file, iterations, size in BENCHMARKS:
        best = min(measure(exe, logo_file, iterations, size) for _ in range(REPEATS))
        results[name] = best

    print(f"{'benchmark':<12} {'ms/iter':>12}")
    for name, _f, _i, _s in BENCHMARKS:
        print(f"{name:<12} {results[name]:>12.6f}")

    if args.save:
        with open(args.save, "w") as f:
            json.dump(results, f, indent=2)
        print(f"\nsaved to {args.save}")

    if args.compare:
        with open(args.compare) as f:
            base = json.load(f)
        print(f"\n{'benchmark':<12} {'base ms':>12} {'now ms':>12} {'speedup':>9}")
        total_base, total_now = 0.0, 0.0
        for name, _f, _i, _s in BENCHMARKS:
            if name not in base:
                continue
            b, n = base[name], results[name]
            total_base += b
            total_now += n
            print(f"{name:<12} {b:>12.6f} {n:>12.6f} {b / n:>8.2f}x")
        if total_base:
            print(f"{'GEOMEAN':<12} {'':>12} {'':>12} "
                  f"{(total_base / total_now) ** (1.0 / max(len(base), 1)):>8.2f}x")

    return 0


if __name__ == "__main__":
    sys.exit(main())
