#pragma once
#include <string>

class FileHandler {
public:
    static std::string ReadInputFile(const std::string& filename);
    static bool WriteOutputFile(const std::string& filename, char** grid, int size);
};
