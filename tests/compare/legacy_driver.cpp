// Times the ORIGINAL hand-written LogoCore (branch legacy-codebase) in a
// loop, mirroring tests/bench.py methodology (warmup + N timed iterations).
// Build it against a worktree of legacy-codebase, not against mainline
// LogoCore (the API differs: char** TurtleInstructions(std::string, ...)).
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "LogoCore.h"

static std::string readFile(const char* path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("usage: legacy_driver <file.logo> <iters>\n");
        return 1;
    }
    std::string code = readFile(argv[1]);
    int iters = atoi(argv[2]);

    TurtleInstructions(code, 500, 500);   // warmup

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; ++i)
        TurtleInstructions(code, 500, 500);
    auto end = std::chrono::high_resolution_clock::now();
    double us = std::chrono::duration<double, std::micro>(end - start).count() / iters;
    printf("legacy us/iter: %.3f\n", us);
    return 0;
}
