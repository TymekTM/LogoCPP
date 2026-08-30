#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "../LogoCore/LogoCore.h"
#include "../LogoCore/FileHandler.h"

void printUsage() {
    std::cout << "Usage: LogoCPP -i <input_file> -o <output_path> -s <canvas_size>" << std::endl;
    std::cout << "  -i  input file in logo format" << std::endl;
    std::cout << "  -o  output file with path traced out" << std::endl;
    std::cout << "  -s  canvas/board size (ex. 100)" << std::endl;
    std::cout << "  -t  trim output to minimal bounding box" << std::endl;
    std::cout << "  -j  run via native JIT (compile Logo to machine code)" << std::endl;
    std::cout << "  -b  benchmark mode - run N iterations and print timing" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    std::string outputFile;
    int boardSize = 50;
    bool trimOutput = false;
    bool useJit = false;
    int benchmarkIterations = 0;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-i" && i + 1 < argc) inputFile = argv[++i];
        else if (arg == "-o" && i + 1 < argc) outputFile = argv[++i];
        else if (arg == "-s" && i + 1 < argc) {
            try { boardSize = std::stoi(argv[++i]); }
            catch (...) { std::cerr << "Error: Invalid canvas size" << std::endl; return 1; }
        }
        else if (arg == "-t") trimOutput = true;
        else if (arg == "-j") useJit = true;
        else if (arg == "-b" && i + 1 < argc) {
            try { benchmarkIterations = std::stoi(argv[++i]); }
            catch (...) { std::cerr << "Error: Invalid benchmark iterations" << std::endl; return 1; }
        }
    }

    if (inputFile.empty() || outputFile.empty()) { printUsage(); return 1; }

    std::string logoCode = FileHandler::ReadInputFile(inputFile);
    if (logoCode.empty()) {
        std::cerr << "Error: Input file is empty or cannot be accessed" << std::endl;
        return 1;
    }

    if (benchmarkIterations > 0) {
        for (int i = 0; i < 3; i++)
            TurtleInstructionsBenchmark(logoCode, boardSize, boardSize, '*', trimOutput);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < benchmarkIterations; i++)
            TurtleInstructionsBenchmark(logoCode, boardSize, boardSize, '*', trimOutput);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double avgMs = static_cast<double>(duration.count()) / 1000.0 / benchmarkIterations;
        std::cout << "Benchmark: " << benchmarkIterations << " iterations" << std::endl;
        std::cout << "Total time: " << duration.count() / 1000.0 << " ms" << std::endl;
        std::cout << "Average per iteration: " << avgMs << " ms" << std::endl;
        return 0;
    }

    auto result = useJit
        ? TurtleInstructionsJit(logoCode, boardSize, boardSize, '*', trimOutput)
        : TurtleInstructions(logoCode, boardSize, boardSize, '*', trimOutput);
    if (!FileHandler::WriteOutputFile(outputFile, result)) {
        std::cerr << "Error: Failed to save output file" << std::endl;
        return 1;
    }
    std::cout << "Sukces! Wynik zapisano do pliku: " << outputFile << std::endl;
    return 0;
}
