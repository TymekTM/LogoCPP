#include "pch.h"
#include "Tokenizer.h"
#include "ParsingHelper.h"
#include <string>
#include <cctype>
#include <unordered_map>
#include "InstructionHandler.h"

std::string_view Tokenizer::TrimView(std::string_view s) noexcept {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string Tokenizer::Trim(const std::string& s) {
    return std::string(TrimView(s));
}

void Tokenizer::TokenizeAndExecute(const std::string& input, Instruction& handler) {
    size_t begin = 0;
    int braceDepth = 0;
    const size_t inputSize = input.size();

    for (size_t i = 0; i < inputSize; ++i) {
        char c = input[i];
        if (c == '{') ++braceDepth;
        else if (c == '}') { if (braceDepth > 0) --braceDepth; }
        
        if (c == ';' && braceDepth == 0) {
            if (i > begin) {
                std::string_view cmdView(input.data() + begin, i - begin);
                cmdView = TrimView(cmdView);
                if (!cmdView.empty()) {
                    handler.HandleInstruction(std::string(cmdView), *this);
                }
            }
            begin = i + 1;
        }
    }

    if (begin < inputSize) {
        std::string_view cmdView(input.data() + begin, inputSize - begin);
        cmdView = TrimView(cmdView);
        if (!cmdView.empty()) {
            handler.HandleInstruction(std::string(cmdView), *this);
        }
    }
}

std::string Tokenizer::ExtractData(const std::string& input, const std::unordered_map<std::string,double>& variables) {
    const size_t inputSize = input.size();
    
    for (size_t i = 0; i < inputSize; i++) {
        if (input[i] == '(') {
            size_t start = i + 1;
            for (size_t j = start; j < inputSize; j++) {
                if (input[j] == ')') {
                    std::string data = input.substr(start, j - start);
                    
                    if (IsArithmetic(data)) {
                        double result = ArithmericHandler(data, variables);
                        return std::to_string(result);
                    }
                    
                    std::string trimmedData = Trim(data);
                    auto it = variables.find(trimmedData);
                    if (it != variables.end()) {
                        return std::to_string(it->second);
                    }
                    return trimmedData;
                }
            }
            break;
        }
    }
    return "";
}

std::string Tokenizer::ExtractCommand(const std::string& input) {
    std::string_view t = TrimView(input);
    
    bool hasEquals = false;
    bool hasParen = false;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == '=') hasEquals = true;
        else if (t[i] == '(') { hasParen = true; break; }
    }
    
    if (hasEquals && !hasParen) return "var";
    
    size_t cmdEnd = 0;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == '(' || t[i] == ' ') { cmdEnd = i; break; }
        cmdEnd = i + 1;
    }
    
    return std::string(TrimView(t.substr(0, cmdEnd)));
}

std::pair<std::string, double> Tokenizer::VariableHandler(const std::string& input) {
    std::string_view trimmedInput = TrimView(input);
    size_t equalPos = trimmedInput.find('=');
    if (equalPos == std::string_view::npos) return {"", 0.0};

    std::string_view varNameView = TrimView(trimmedInput.substr(0, equalPos));
    std::string_view varValueView = TrimView(trimmedInput.substr(equalPos + 1));
    
    return {std::string(varNameView), std::stod(std::string(varValueView))};
}

bool Tokenizer::IsArithmetic(const std::string& input) const noexcept {
    for (char c : input) {
        if (c == '+' || c == '-' || c == '*') return true;
    }
    return false;
}

double Tokenizer::ArithmericHandler(const std::string& input, const std::unordered_map<std::string, double>& variables) {
    const size_t inputSize = input.size();
    
    for (size_t i = 0; i < inputSize; i++) {
        char op = input[i];
        if (op == '+' || op == '-' || op == '*') {
            std::string left = Trim(input.substr(0, i));
            std::string right = Trim(input.substr(i + 1));
            double lv = ParsingHelper::ParseValue(left, variables);
            double rv = ParsingHelper::ParseValue(right, variables);
            if (op == '+') return lv + rv;
            if (op == '-') return lv - rv;
            return lv * rv;
        }
    }
    return 0.0;
}

bool Tokenizer::LogicHandler(const std::string& input, const std::unordered_map<std::string, double>& variables) {
    const size_t inputSize = input.size();
    
    for (size_t i = 0; i + 1 < inputSize; i++) {
        char op = input[i];
        char next = input[i + 1];
        
        if ((op == '=' && next == '=') || (op == '<' && next == '>') ||
            (op == '>' && next == '=') || (op == '<' && next == '=')) {
            
            std::string left = Trim(input.substr(0, i));
            std::string right = Trim(input.substr(i + 2));
            double lv = ParsingHelper::ParseValue(left, variables);
            double rv = ParsingHelper::ParseValue(right, variables);

            if (op == '=' && next == '=') return lv == rv;
            if (op == '<' && next == '>') return lv != rv;
            if (op == '>' && next == '=') return lv >= rv;
            return lv <= rv;
        }
    }
    
    for (size_t i = 0; i < inputSize; i++) {
        char op = input[i];
        if (op == '>' || op == '<') {
            if (i + 1 < inputSize) {
                char next = input[i + 1];
                if (next == '=' || (op == '<' && next == '>')) continue;
            }
            if (i > 0 && (input[i-1] == '<' || input[i-1] == '>' || input[i-1] == '=')) continue;
            
            std::string left = Trim(input.substr(0, i));
            std::string right = Trim(input.substr(i + 1));
            double lv = ParsingHelper::ParseValue(left, variables);
            double rv = ParsingHelper::ParseValue(right, variables);
            return op == '>' ? lv > rv : lv < rv;
        }
    }
    return false;
}

std::string Tokenizer::ExtractBracketsContent(const std::string& input, size_t startPos) {
    int count = 0;
    size_t contentStart = 0;
    const size_t inputSize = input.size();
    
    for (size_t i = startPos; i < inputSize; i++) {
        if (input[i] == '{') {
            count++;
            if (count == 1) contentStart = i + 1;
        } else if (input[i] == '}') {
            count--;
            if (count == 0) return input.substr(contentStart, i - contentStart);
        }
    }
    return "";
}

std::vector<std::string> Tokenizer::ExtractArguments(const std::string& instruction) {
    std::vector<std::string> args;
    size_t startParen = instruction.find('(');
    size_t endParen = instruction.find(')');
    
    if (startParen == std::string::npos || endParen == std::string::npos || endParen <= startParen + 1)
        return args;
    
    args.reserve(4);
    size_t start = startParen + 1;
    for (size_t i = startParen + 1; i <= endParen; i++) {
        if (i == endParen || instruction[i] == ',') {
            if (i > start) {
                std::string_view argView(instruction.data() + start, i - start);
                argView = TrimView(argView);
                if (!argView.empty()) args.emplace_back(argView);
            }
            start = i + 1;
        }
    }
    return args;
}

std::string Tokenizer::ExtractFunctionName(const std::string& instruction) {
    size_t defPos = 0;
    const size_t instrSize = instruction.size();
    
    while (defPos < instrSize) {
        size_t pos = instruction.find("def", defPos);
        if (pos == std::string::npos) return "";
        
        bool validStart = (pos == 0) || std::isspace(static_cast<unsigned char>(instruction[pos - 1]));
        bool validEnd = (pos + 3 < instrSize) && std::isspace(static_cast<unsigned char>(instruction[pos + 3]));
        
        if (validStart && validEnd) { defPos = pos; break; }
        defPos = pos + 1;
    }
    
    if (defPos >= instrSize) return "";
    
    size_t nameStart = defPos + 3;
    while (nameStart < instrSize && std::isspace(static_cast<unsigned char>(instruction[nameStart]))) nameStart++;
    
    size_t parenPos = instruction.find('(', nameStart);
    if (parenPos == std::string::npos) return "";
    
    return std::string(TrimView(std::string_view(instruction.data() + nameStart, parenPos - nameStart)));
}
