#include "pch.h"
#include "Tokenizer.h"
#include "ParsingHelper.h"
#include <string>
#include <cctype>
#include <map>
#include <sstream>
#include "InstructionHandler.h"

static std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() &&
        std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start &&
        std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}


std::vector<std::string> Tokenizer::Tokenize(const std::string& input) {
    std::vector<std::string> commands;
    size_t begin = 0;
    int braceDepth = 0;

    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        
        if (c == '{') {
            braceDepth++;
        } else if (c == '}') {
            if (braceDepth > 0) braceDepth--;
        }

        if (c == ';' && braceDepth == 0) {
            if (i > begin) {
                std::string command;
                for (size_t j = begin; j < i; ++j) {
                    command += input[j];
                }
                command = trim(command);
                if (!command.empty()) {
                    commands.push_back(command);
                }
            }
            begin = i + 1;
        }
    }

    if (begin < input.size()) {
        std::string command;
        for (size_t j = begin; j < input.size(); ++j) {
            command += input[j];
        }
        command = trim(command);
        if (!command.empty()) {
            commands.push_back(command);
        }
    }

    return commands;
}

std::string Tokenizer::ExtractData(const std::string& input, std::map<std::string,double> variables) {
    std::string data;
    
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '(') {

            for (size_t j = i + 1; j < input.size(); j++) {
                if (input[j] == ')') {
                    // Najpierw sprawdź czy to wyrażenie arytmetyczne
                    if (IsArithmetic(data)) {
                        double result = ArithmericHandler(data, variables);
                        return std::to_string(result);
                    }
                    
                    // Sprawdź czy to zmienna
                    auto it = variables.find(data);
                    if (it != variables.end()) {
                        return std::to_string(it->second);
                    }
                    
                    // W przeciwnym razie zwróć surowe wartości
                    return trim(data);
                }
                data += input[j];
            }
        }
    }
    return "";
}

std::string Tokenizer::ExtractCommand(const std::string& input) {
    std::string command;
    std::string t = trim(input);
    

    if (t.find('=') != std::string::npos && t.find('(') == std::string::npos) {
        return "var";
    }
    else {
        for (size_t i = 0; i < t.size(); i++) {
            if (t[i] == '(' || t[i] == ' ') {
                return trim(command);
            }
            command += t[i];
        }
        return trim(command);
    }
}

std::map<std::string,double> Tokenizer::VariableHandler(const std::string& input) {
	std::string trimmedInput = trim(input);

	size_t equalPos = trimmedInput.find('=');
	if (equalPos == std::string::npos) {
		return {};
	}

	std::string varName = trim(trimmedInput.substr(0, equalPos));
	
	std::string varValueStr = trim(trimmedInput.substr(equalPos + 1));
	
	double varValue = std::stod(varValueStr);
	return { {varName, varValue} };
}

bool Tokenizer::IsArithmetic(const std::string& input) {
    if (input.find('+') != std::string::npos ||
        input.find('-') != std::string::npos ||
        input.find('*') != std::string::npos) {
        return true;
	}
    else return false;
}

double Tokenizer::ArithmericHandler(const std::string& input, std::map<std::string, double> variables) {
    for (size_t i = 0; i < input.size(); i++) {
        char op = input[i];

        if (op == '+' || op == '-' || op == '*') {

            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }

            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }

            left = trim(left);
            right = trim(right);

            double leftValue = ParsingHelper::ParseValue(left, variables);
            double rightValue = ParsingHelper::ParseValue(right, variables);

            if (op == '+') return leftValue + rightValue;
            else if (op == '-') return leftValue - rightValue;
            else if (op == '*') return leftValue * rightValue;
        }
    }
    return 0.0;
}

bool Tokenizer::LogicHandler(const std::string& input, std::map<std::string, double> variables) {
    for (size_t i = 0; i < input.size(); i++) {
        char op = input[i];
        
        // Handle two-character operators first (==, <>)
        if (i + 1 < input.size()) {
            char nextChar = input[i + 1];
            
            if ((op == '=' && nextChar == '=') || (op == '<' && nextChar == '>')) {

                std::string left, right;
                for (size_t j = 0; j < i; j++) {
                    left += input[j];
                }

                for (size_t j = i + 2; j < input.size(); j++) {
                    right += input[j];
                }

                left = trim(left);
                right = trim(right);

                double leftValue = ParsingHelper::ParseValue(left, variables);
                double rightValue = ParsingHelper::ParseValue(right, variables);

                if (op == '=' && nextChar == '=') return leftValue == rightValue;
                else if (op == '<' && nextChar == '>') return leftValue != rightValue;
                else if (op == '>' && nextChar == '=') return leftValue >= rightValue;
                else if (op == '<' && nextChar == '=') return leftValue <= rightValue;
            }
        }
    }
    
    for (size_t i = 0; i < input.size(); i++) {
        char op = input[i];
        
        // Handle single-character operators (>, <, =)
        if (op == '>' || op == '<') {
            // Pomiń jeśli to część dwuznakowego operatora
            if (i + 1 < input.size()) {
                char nextChar = input[i + 1];
                if ((op == '=' && nextChar == '=') || (op == '<' && nextChar == '>') ||
                    (op == '>' && nextChar == '=') || (op == '<' && nextChar == '=')) {
                    continue;
                }
            }
            if (i > 0 && (input[i-1] == '<' || input[i-1] == '>')) {
                continue;
            }
            
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            double leftValue = ParsingHelper::ParseValue(left, variables);
            double rightValue = ParsingHelper::ParseValue(right, variables);

            if (op == '>') return leftValue > rightValue;
            else if (op == '<') return leftValue < rightValue;
        }
    }
    return false;
}

std::string Tokenizer::ExtractBracketsContent(const std::string& input, size_t startPos) {
    std::string content;
    int bracketCount = 0;
    bool foundStart = false;
    
    for (size_t i = startPos; i < input.size(); i++) {
        if (input[i] == '{') {
            if (foundStart) {
                content += input[i];
            }
            bracketCount++;
            foundStart = true;
        }
        else if (input[i] == '}') {
            bracketCount--;
            if (bracketCount == 0) {
                return content;
            }
            content += input[i];
        }
        else if (foundStart) {
            content += input[i];
        }
    }
    
    return content;
}

std::vector<std::string> Tokenizer::ExtractArguments(const std::string& instruction) {
    std::vector<std::string> args;
    
    size_t startParen = instruction.find('(');
    size_t endParen = instruction.find(')');
    
    if (startParen == std::string::npos || endParen == std::string::npos) {
        return args;
    }
    
    std::string argsStr = instruction.substr(startParen + 1, endParen - startParen - 1);
    
    if (argsStr.empty()) {
        return args;
    }
    
    std::stringstream ss(argsStr);
    std::string arg;
    
    while (std::getline(ss, arg, ',')) {
        arg = trim(arg);
        if (!arg.empty()) {
            args.push_back(arg);
        }
    }
    
    return args;
}

std::string Tokenizer::ExtractFunctionName(const std::string& instruction) {
    size_t defPos = instruction.find("def");
    if (defPos == std::string::npos) return "";
    
    size_t nameStart = defPos + 3;
    
    while (nameStart < instruction.size() && std::isspace(instruction[nameStart])) {
        nameStart++;
    }
    
    size_t parenPos = instruction.find('(', nameStart);
    if (parenPos == std::string::npos) return "";
    
    std::string functionName = instruction.substr(nameStart, parenPos - nameStart);
    return trim(functionName);
}