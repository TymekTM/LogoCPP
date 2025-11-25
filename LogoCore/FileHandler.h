#pragma once
#include <string>
#include <vector>

class FileHandler {
public:
    static std::string ReadInputFile(const std::string& filename);
    static bool WriteOutputFile(const std::string& filename, const std::vector<std::vector<char>>& grid);
};
