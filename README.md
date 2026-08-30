# LogoCPP

A simple Logo interpreter with a turtle that draws a path into a text file. The program reads a `.logo` file, executes instructions, and writes the result as a character grid (by default `*` and spaces).  
Semester project written in 4 nights - lightweight, pragmatic, and focused on getting things done.

## ✨ Quick start

1. Open `LogoCPP.slnx` in Visual Studio (install the **Desktop development with C++** workload).
2. Build the `LogoCPP` project (Release or Debug).
3. Run the program with input/output parameters.

## ✅ Requirements

- Windows
- Visual Studio 2019/2022 with C++

## 🔧 Running (CLI)

Syntax:

- `-i <input_file>` - `.logo` input file
- `-o <output_file>` - output file (text grid)
- `-s <size>` - canvas/board size (e.g., `100`)
- `-t` - trim output to the minimal bounding box (faster output)
- `-j` - run via the native JIT: compile Logo to x86-64 machine code
- `-b <N>` - benchmark mode: run N iterations and report time (no I/O)

Example:

- `LogoCPP -i example_input.logo -o output.txt -s 100`

The program has three execution paths: the streaming tokenizer interpreter
(default), the bytecode executor, and the native JIT (`-j`). Benchmark mode
(`-b`) always uses the fastest available path (JIT).

## 🧠 Input format & syntax

- Instructions end with a semicolon `;`
- Blocks use curly braces `{ ... }`
- Function call: `name(arg1, arg2);`
- Function definition: `def name(a, b) { ... };`
- Variables: `x = 10;`
- Conditions: `if (x >= 10) { ... };`
- Operators: `+`, `-`, `*`
- Comparisons: `==`, `<>`, `>=`, `<=`, `>`, `<`

### Command cheat sheet (PL/EN)

Turtle movement:
- `forward(n)` / `przod(n)`
- `backward(n)` / `tyl(n)`
- `left(n)` / `lewo(n)`
- `right(n)` / `prawo(n)`

Pen control:
- `penup()` / `pu()`
- `pendown()` / `pd()`

### Mini example

```
pd();
forward(10);
right(90);
forward(10);
right(90);
forward(10);
right(90);
forward(10);
```

## ⚡ Performance

The headline numbers (recursive fractal tree, depth 15, one desktop CPU,
2026-08 - full table and methodology in [tests/compare/RESULTS.md](tests/compare/RESULTS.md)):

| Implementation | n=15 | LogoCPP JIT is |
|---|---:|---:|
| Python stdlib turtle (Tk) | 375 ms | ~1320x faster |
| Pure CPython, same algorithm | 35.4 ms | ~124x faster |
| Straightforward hand-written C++ | 373 us | 1.3x faster |
| LogoCPP bytecode executor | 614 us | 2.2x faster |
| **LogoCPP JIT** | **285 us** | - |

The JIT beats even a hand-written C++ recursion that calls the same
`Turtle`/`Canvas`: it hoists the `if (n > 0)` recursion guard to call sites
(leaf calls never execute) and inlines turtle moves into straight-line code
with fixed-point trig tables pinned in registers.

Numbers are machine-specific. Reproduce them with:

- `python tests/bench.py` - LogoCPP, best of 5 runs
- `python tests/compare/tree_py_turtle.py 15` - Python stdlib turtle
- `python tests/compare/tree_py_grid.py 15` - pure CPython port
- `tests\compare\build_handwritten.bat && tests\compare\handwritten.exe 15` - C++ reference

The design and optimization journey (what worked, what regressed, and why)
is documented in [CASE_STUDY.md](CASE_STUDY.md).

## 📂 Useful files

- `example_input.logo` - simple input example
- `tests/` - regression suite (`run_tests.py`), benchmarks (`bench.py`), comparison scripts (`compare/`)
- `tests/performance_test*.logo` - heavier programs for performance tests
- `CASE_STUDY.md` - the 1x → 370x optimization write-up (PL)

## 📄 License

MIT - see `LICENSE` for details.
