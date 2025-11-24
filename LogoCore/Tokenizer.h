#pragma once
#include <string>
#include <vector>
#include <map>

class Tokenizer {
public:
    std::vector<std::string> Tokenize(const std::string& input);
	std::string ExtractData(const std::string& input, std::map<std::string, double> variables);
	std::string ExtractCommand(const std::string& input);
	std::map<std::string, double> VariableHandler(const std::string& input);
	double ArithmericHandler(const std::string& input, std::map<std::string, double> variables);
	bool IsArithmetic(const std::string& input);
	bool LogicHandler(const std::string& input, std::map<std::string, double> variables);
	std::string ExtractBracketsContent(const std::string& input, size_t startPos);
	std::vector<std::string> ExtractArguments(const std::string& instruction);
	std::string ExtractFunctionName(const std::string& instruction);
private:
	std::map<std::string, double> variables;
};
