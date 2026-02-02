# LogoCPP

A simple Logo interpreter with a turtle that draws a path into a text file. The program reads a `.logo` file, executes instructions, and writes the result as a character grid (by default `*` and spaces).  
Semester project written in 4 nights — lightweight, pragmatic, and focused on getting things done.

## ✨ Quick start

1. Open `LogoCPP.slnx` in Visual Studio (install the **Desktop development with C++** workload).
2. Build the `LogoCPP` project (Release or Debug).
3. Run the program with input/output parameters.

## ✅ Requirements

- Windows
- Visual Studio 2019/2022 with C++

## 🔧 Running (CLI)

Syntax:

- `-i <input_file>` — `.logo` input file
- `-o <output_file>` — output file (text grid)
- `-s <size>` — canvas/board size (e.g., `100`)
- `-t` — trim output to the minimal bounding box (faster output)
- `-b <N>` — benchmark mode: run N iterations and report time (no I/O)

Example:

- `LogoCPP -i example_input.logo -o output.txt -s 100`

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

## ⚡ Performance vs Python

This is a C++ implementation, so it will **typically** be noticeably faster than an equivalent interpreter written in Python — especially for large inputs or many iterations. The parser also works in a streaming way (no heavy AST), which reduces overhead.

I’m not listing “magic” numbers because performance depends heavily on hardware and the specific program. Instead, you can:

1. Run benchmark mode (`-b N`), which measures **only** parsing and execution (no I/O).
2. Compare the timing with a Python version of the same program.

Benchmark example:

- `LogoCPP -i tests/performance_test.logo -o output.txt -s 200 -b 100`

## 📂 Useful files

- `example_input.logo` — simple input example
- `tests/performance_test*.logo` — heavier programs for performance tests
- `x64/Release/output.txt` — example output

## 📄 License

MIT — see `LICENSE` for details.
