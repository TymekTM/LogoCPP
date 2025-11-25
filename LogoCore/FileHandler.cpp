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

bool FileHandler::WriteOutputFile(const std::string& filename, const std::vector<std::vector<char>>& grid) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Error: Failed to create output file: " << filename << std::endl;
        return false;
    }
    
    const size_t height = grid.size();
    if (height == 0) {
        file.close();
        return true;
    }
    
    const size_t width = grid[0].size();
    
    // Buforowanie całego outputu dla szybszego zapisu
    std::string buffer;
    buffer.reserve(height * (width + 1)); // +1 na newline
    
    for (size_t i = 0; i < height; ++i) {
        buffer.append(grid[i].begin(), grid[i].end());
        buffer += '\n';
    }
    
    file << buffer;
    file.close();
    return true;
}
