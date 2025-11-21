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
	int ArithmericHandler(const std::string& input, std::map<std::string, int> variables);
	bool IsArithmetic(const std::string& input);
	bool LogicHandler(const std::string& input, std::map<std::string, int> variables);
	std::string ExtractBracketsContent(const std::string& input, size_t startPos);
private:
	std::map<std::string, int> variables;
};
