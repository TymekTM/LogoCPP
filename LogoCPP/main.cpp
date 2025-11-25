#include <iostream>
#include <string>
#include <vector>
#include "../LogoCore/LogoCore.h"
#include "../LogoCore/FileHandler.h"

void printUsage() {
    std::cout << "Usage: LogoCPP -i <input_file> -o <output_path> -s <canvas_size>" << std::endl;
    std::cout << "  -i  input file in logo format" << std::endl;
    std::cout << "  -o  output file with path traced out" << std::endl;
    std::cout << "  -s  canvas/board size (ex. 100)" << std::endl;
    std::cout << "  -t  trim output to minimal bounding box (optional, faster output)" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    std::string outputFile;
    int boardSize = 50;
    bool trimOutput = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-i" && i + 1 < argc) {
            inputFile = argv[++i];
        }
        else if (arg == "-o" && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg == "-s" && i + 1 < argc) {
            try {
                boardSize = std::stoi(argv[++i]);
            }
            catch (const std::exception&) {
                std::cerr << "Error: Invalid canvas size" << std::endl;
                return 1;
            }
        }
        else if (arg == "-t") {
            trimOutput = true;
        }
    }


    if (inputFile.empty() || outputFile.empty()) {
        printUsage();
        return 1;
    }

    std::string logoCode = FileHandler::ReadInputFile(inputFile);
    if (logoCode.empty()) {
        std::cerr << "Error: Input file is empty or cannot be accessed" << std::endl;
        return 1;
    }

    std::vector<std::vector<char>> result = TurtleInstructions(logoCode, boardSize, boardSize, '*', trimOutput);

    if (!FileHandler::WriteOutputFile(outputFile, result)) {
        std::cerr << "Error: Failed to save output file" << std::endl;
        return 1;
    }

    std::cout << "Sukces! Wynik zapisano do pliku: " << outputFile << std::endl;

    return 0;
}
