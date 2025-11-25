#ifndef LOGOCORE_H
#define LOGOCORE_H

#include <vector>
#include <string>

std::vector<std::vector<char>> TurtleInstructions(const std::string& instructionSet, int width = 25, int height = 25, char pen = '*', bool trimOutput = false);

#endif // LOGOCORE_H
