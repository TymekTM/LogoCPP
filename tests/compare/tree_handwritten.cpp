// Straightforward hand-written C++ version of tests/performance_test_super_
// heavy.logo, calling the same LogoCore Turtle/Canvas as the LogoCPP JIT.
// This is the "what if a human just wrote the recursion in C++" reference:
// it shares Forward/Backward/Left/Right and the char grid, so the only thing
// being compared is the caller - hand-written recursion vs JIT machine code.
#include "Turtle.h"
#include "Canvas.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>

static void krzaczek(Turtle& t, double x, int n) {
    if (n > 0) {
        t.Forward(static_cast<int>(x + 0.5));
        t.Left(45);
        krzaczek(t, x * 0.75, n - 1);
        t.Right(90);
        krzaczek(t, x * 0.75, n - 1);
        t.Left(45);
        t.Backward(static_cast<int>(x + 0.5));
    }
}

int main(int argc, char** argv) {
    int depth = argc > 1 ? atoi(argv[1]) : 15;
    int iters = argc > 2 ? atoi(argv[2]) : 20;
    Canvas canvas(100, 100);
    Turtle turtle(canvas, '*');

    krzaczek(turtle, 50.0, depth);   // warmup

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; ++i) {
        canvas.reset(100, 100, true);
        turtle.posX = 50; turtle.posY = 50; turtle.angle = 0; turtle.penDown = true;
        krzaczek(turtle, 50.0, depth);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double us = std::chrono::duration<double, std::micro>(end - start).count() / iters;
    printf("handwritten depth=%d: %.3f us/iter\n", depth, us);
    return 0;
}
