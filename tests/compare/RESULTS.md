# Cross-implementation comparison: recursive fractal tree

The same program (`tests/performance_test{,_heavy,_super_heavy}.logo`, a binary
recursive tree of depth 8/12/15) implemented in LogoCPP and in reference
implementations, timed on one machine. LogoCPP numbers come from
`tests/bench.py` (best of 5 runs of `-b` mode, which compiles once and times
execution only).

## Results (2026-08-30, Ryzen desktop, Release x64 + LTCG, no PGO)

| Implementation | n=8 | n=12 | n=15 |
|---|---:|---:|---:|
| Python stdlib turtle (Tk, tracer off) | 2,461 us | 37,774 us | 375,455 us |
| Pure CPython char-grid port | 559 us | 5,610 us | 35,403 us |
| Original hand-written code (legacy-codebase branch) | 5,765 us | 90,492 us | 738,764 us |
| LogoCPP tokenizer interpreter (pre-optimization) | 970 us | 13,700 us | 105,800 us |
| Straightforward hand-written C++ (same Turtle/Canvas, /O2) | 14.4 us | 118.8 us | 372.6 us |
| LogoCPP bytecode executor (pre-JIT) | 8.43 us | 88.5 us | 613.6 us |
| **LogoCPP JIT** | **5.02 us** | **47.8 us** | **284.5 us** |

The tokenizer-interpreter row is the pre-optimization baseline recorded in
`CASE_STUDY.md` on the same machine; the bytecode row is `tests/baseline.json`
(recorded at commit 1286b5e, the state right before the JIT landed).

## vs the original hand-written code (`legacy-codebase`)

The `legacy-codebase` branch preserves the untouched original project. Timed
through `tests/compare/legacy_driver.cpp` (pure `TurtleInstructions` calls,
same inputs and 500x500 canvas, best of 3):

| | n=8 | n=12 | n=15 |
|---|---:|---:|---:|
| Original (legacy-codebase) | 5,765 us | 90,492 us | 738,764 us |
| LogoCPP JIT | 5.02 us | 47.8 us | 284.5 us |
| **Speedup** | **~1,200x** | **~1,880x** | **~2,550x** |

What the original does per function call (all removed on the way here):
re-tokenizes the entire function body from a string, copies the whole
`std::map<string, double>` variable map twice per call (locals + saved),
parses arithmetic expressions from strings with `std::stod`, and draws by
sampling every step with two `round()` calls and double `sin`/`cos` per
move. Canvas is `char**` rows rebuilt per call (measured fixed overhead:
~104 us of the totals above, under 2% even at n=8).

Reproduce (from the repo root):

```
git worktree add .tmpp/legacy legacy-codebase
msbuild .tmpp\legacy\LogoCPP.slnx -p:Configuration=Release -p:Platform=x64 -p:PlatformToolset=v143
cl /nologo /O2 /MD /EHsc /std:c++20 /I .tmpp\legacy\LogoCore tests\compare\legacy_driver.cpp /Fe:.tmpp\legacy\legacy_driver.exe /link .tmpp\legacy\x64\Release\LogoCore.lib
.tmpp\legacy\legacy_driver.exe tests\performance_test_super_heavy.logo 3
```

(The legacy projects declare toolset v145; overriding to v143/VS2022 builds
them unmodified.)

## LogoCPP JIT vs the field (n=15)

| Against | Speedup |
|---|---:|
| Python stdlib turtle (Tk) | **~1320x** |
| Original hand-written code (legacy-codebase) | **~2550x** |
| LogoCPP tokenizer interpreter (pre-optimization) | ~372x |
| Pure CPython char-grid port | ~124x |
| LogoCPP bytecode executor | 2.2x |
| Hand-written C++ recursion | **1.3x** (JIT wins) |

The last row is the interesting one: the JIT-generated machine code beats
straightforward hand-written C++ that calls the very same `Turtle`/`Canvas`.
Two compiler tricks a human typically would not bother with pay for it:

1. **Call-site early-exit guard** - a recursive call guarded by `if (n > 0)`
   in the callee is turned into `compare + jcc` *at the call site*, so leaf
   calls (half of all calls in a binary tree) never execute at all: no frame,
   no argument evaluation.
2. **Fully inlined turtle moves** - Forward/Backward compile to straight-line
   code reading the fixed-point trig tables through registers pinned for the
   whole program, with sub-pixel moves stored directly to the grid.

A hand-tuned C++ version with the same tricks would likely match the JIT;
the claim is "beats the code a competent human writes in five minutes", not
"beats all possible C++".

## Reproducing

```
# LogoCPP (JIT path; -b compiles once, then times N executions)
python tests/bench.py

# Python mirrors
python tests/compare/tree_py_turtle.py 15
python tests/compare/tree_py_grid.py 15

# Hand-written C++ ceiling (needs a Release build of LogoCore first)
tests\compare\build_handwritten.bat
tests\compare\handwritten.exe 15 100
```

Methodology notes: C++ timings average 100-20,000 iterations after a warmup
run; Python grid timings are best-of-3 after warmup; Tk turtle is a single
run (Tk dominates). All implementations draw the same figure with the same
rounding (`int(x + 0.5)` on distance, fixed-point x8192 trig, Bresenham
lines) except Tk turtle, which is float-native.
