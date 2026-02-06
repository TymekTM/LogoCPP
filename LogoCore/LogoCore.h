#ifndef LOGOCORE_H
#define LOGOCORE_H

#include <vector>
#include <string>

// Original API (returns grid as vector<vector<char>>)
std::vector<std::vector<char>> TurtleInstructions(const std::string& instructionSet, int width = 25, int height = 25, char pen = '*', bool trimOutput = false);

// Fast benchmark API (skips grid copy, caches compilation after first call)
void TurtleInstructionsBenchmark(const std::string& instructionSet, int width = 25, int height = 25, char pen = '*', bool trimOutput = false);

#endif
