#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <string_view>

class Instruction;

class Tokenizer {
public:
    void TokenizeAndExecute(const std::string& input, Instruction& handler);
    
    std::string ExtractData(const std::string& input, const std::unordered_map<std::string, double>& variables);
    std::string ExtractCommand(const std::string& input);
    std::string ExtractBracketsContent(const std::string& input, size_t startPos);
    std::vector<std::string> ExtractArguments(const std::string& instruction);
    std::string ExtractFunctionName(const std::string& instruction);
    
    std::pair<std::string, double> VariableHandler(const std::string& input, const std::unordered_map<std::string, double>& variables);
    double ArithmericHandler(const std::string& input, const std::unordered_map<std::string, double>& variables);
    bool IsArithmetic(const std::string& input) const noexcept;
    bool LogicHandler(const std::string& input, const std::unordered_map<std::string, double>& variables);
    
    static std::string_view TrimView(std::string_view s) noexcept;
    static std::string Trim(const std::string& s);
};
