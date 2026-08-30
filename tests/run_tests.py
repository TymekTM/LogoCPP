#!/usr/bin/env python3
"""Regression test suite for LogoCPP.

Runs every program in tests/cases through the interpreter and compares the
rendered character grid byte-for-byte against the golden files in
tests/expected. This is the correctness gate for performance work: every
optimization must keep all tests passing.

Usage:
    python tests/run_tests.py                 # run all tests
    python tests/run_tests.py --update        # regenerate golden files
    python tests/run_tests.py --exe <path>    # use a specific build
"""

import argparse
import os
import subprocess
import sys
import tempfile

# Canvas size per test case (must stay in sync with golden files)
CASE_SIZES = {
    "square": 50,
    "star": 50,
    "spiral": 40,
    "nested_squares": 50,
    "koch_snowflake": 60,
    "tree": 50,
    "pl_commands": 50,
    "pen_dots": 50,
    "vars_arith": 50,
    "condition_ops": 50,
    "multi_function": 50,
    "diagonals": 50,
}

ROOT = os.path.dirname(os.path.abspath(__file__))
CASES_DIR = os.path.join(ROOT, "cases")
EXPECTED_DIR = os.path.join(ROOT, "expected")
DEFAULT_EXE = os.path.join(ROOT, "..", "x64", "Release", "LogoCPP.exe")


def run_case(exe, name, out_path):
    size = CASE_SIZES[name]
    input_file = os.path.join(CASES_DIR, name + ".logo")
    result = subprocess.run(
        [exe, "-i", input_file, "-o", out_path, "-s", str(size)],
        capture_output=True,
        text=True,
    )
    return result


def main():
    parser = argparse.ArgumentParser(description="LogoCPP regression tests")
    parser.add_argument("--update", action="store_true",
                        help="regenerate golden files from current build")
    parser.add_argument("--exe", default=DEFAULT_EXE,
                        help="path to LogoCPP.exe")
    args = parser.parse_args()

    exe = os.path.abspath(args.exe)
    if not os.path.exists(exe):
        print(f"ERROR: interpreter not found: {exe}")
        return 2

    names = sorted(f[:-5] for f in os.listdir(CASES_DIR) if f.endswith(".logo"))
    missing = [n for n in names if n not in CASE_SIZES]
    if missing:
        print(f"ERROR: no canvas size configured for: {missing}")
        return 2

    os.makedirs(EXPECTED_DIR, exist_ok=True)
    passed, failed = 0, 0

    for name in names:
        golden_path = os.path.join(EXPECTED_DIR, name + ".txt")
        with tempfile.NamedTemporaryFile(suffix=".txt", delete=False) as tmp:
            out_path = tmp.name
        try:
            result = run_case(exe, name, out_path)
            if result.returncode != 0:
                print(f"FAIL {name}: exit code {result.returncode}")
                failed += 1
                continue
            with open(out_path, "rb") as f:
                actual = f.read()
            if args.update:
                with open(golden_path, "wb") as f:
                    f.write(actual)
                print(f"UPDATED {name} ({len(actual)} bytes)")
                continue
            if not os.path.exists(golden_path):
                print(f"FAIL {name}: no golden file (run with --update first)")
                failed += 1
                continue
            with open(golden_path, "rb") as f:
                expected = f.read()
            if actual == expected:
                print(f"PASS {name}")
                passed += 1
            else:
                # Locate first differing line for a helpful message
                exp_lines = expected.splitlines(keepends=True)
                act_lines = actual.splitlines(keepends=True)
                for i, (e, a) in enumerate(zip(exp_lines, act_lines)):
                    if e != a:
                        print(f"FAIL {name}: first diff at line {i + 1}")
                        print(f"  expected: {e!r}")
                        print(f"  actual:   {a!r}")
                        break
                else:
                    print(f"FAIL {name}: length differs "
                          f"(expected {len(expected)}, got {len(actual)})")
                failed += 1
        finally:
            os.unlink(out_path)

    if args.update:
        print(f"\n{len(names)} golden files written.")
        return 0

    print(f"\n{passed} passed, {failed} failed, {len(names)} total")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
