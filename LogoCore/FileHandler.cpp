#include "pch.h"
#include "FileHandler.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::string FileHandler::ReadInputFile(const std::string& filename) {
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open file: " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return buffer.str();
}

bool FileHandler::WriteOutputFile(const std::string& filename, char** grid, int size) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Failed to create output file: " << filename << std::endl;
        return false;
    }
    
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            file << grid[i][j];
        }
        file << '\n';
    }
    
    file.close();
    return true;
}
