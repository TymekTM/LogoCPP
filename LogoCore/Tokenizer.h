#pragma once
#include <string>
#include <vector>

class Tokenizer {
public:
    std::vector<std::string> Tokenize(const std::string& input);
	std::string ExtractData(const std::string& input);
};