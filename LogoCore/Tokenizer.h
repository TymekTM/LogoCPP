#pragma once
#include <string>
#include <vector>
#include <map>

class Tokenizer {
public:
    std::vector<std::string> Tokenize(const std::string& input);
	std::string ExtractData(const std::string& input, std::map<std::string, int> variables);
	std::string ExtractCommand(const std::string& input);
	std::map<std::string, int> VariableHandler(const std::string& input);

private:
	std::map<std::string, int> variables;
};