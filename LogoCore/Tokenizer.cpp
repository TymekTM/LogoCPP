#include "pch.h"
#include "Tokenizer.h"
#include <string>
#include <cctype>
#include <map>
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
    
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == ';') {
            if (i > begin) {
                std::string command;
                for (size_t j = begin; j < i; j++) {
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
        for (size_t j = begin; j < input.size(); j++) {
            command += input[j];
        }
        command = trim(command);
        if (!command.empty()) {
            commands.push_back(command);
        }
    }
    
    return commands;
}

std::string Tokenizer::ExtractData(const std::string& input, std::map<std::string,int> variables) {
    std::string data;
    
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '(') {

            for (size_t j = i + 1; j < input.size(); j++) {
                if (input[j] == ')') {
					
					auto it = variables.find(data);
                    if (it != variables.end()) {
                        int modified = it->second;
						if (IsArithmetic(data)) {
                            modified = ArithmericHandler(data, variables);
                        }
						return std::to_string(modified);
                    }
                    else {
						if (IsArithmetic(data)) {
                            int result = ArithmericHandler(data, variables);
                            return std::to_string(result);
                        }
						return trim(data);
                    }
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
            if (t[i] == '(') {
                return trim(command);
            }
            command += t[i];
        }
        return trim(command);
    }
}

std::map<std::string,int> Tokenizer::VariableHandler(const std::string& input) {
	std::string trimmedInput = trim(input);

	size_t equalPos = trimmedInput.find('=');
	if (equalPos == std::string::npos) {
		return {};
	}

	std::string varName = trim(trimmedInput.substr(0, equalPos));
	

	std::string varValueStr = trim(trimmedInput.substr(equalPos + 1));
	
	
	int varValue = std::stoi(varValueStr);
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

//TO DO: BOTH, VERY REPETETIVE CODE, OPTIMIZE, SAME HELPER FUNCTION FOR BOTH
int Tokenizer::ArithmericHandler(const std::string& input, std::map<std::string, int> variables) {
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '+') {
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            int leftValue = 0;
            int rightValue = 0;


            try {
                int leftValue = std::stoi(left);
                int rightValue = std::stoi(right);
                return leftValue + rightValue;
            }
            catch (const std::exception&) {
                auto it = variables.find(left);
                if (it != variables.end()) {
                    leftValue = it->second;
                }
                else {

                    return 0; 
                }
            }


            try {
                rightValue = std::stoi(right);
            }
            catch (const std::exception&) {
                auto it = variables.find(right);
                if (it != variables.end()) {
                    rightValue = it->second;
                }
                else {

                    return 0; 
                }
            }

        }
        else if(input[i] == '-') {
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            int leftValue = 0;
            int rightValue = 0;


            try {
                leftValue = std::stoi(left);
            }
            catch (const std::exception&) {
                auto it = variables.find(left);
                if (it != variables.end()) {
                    leftValue = it->second;
                }
                else {
                    return 0;
                }
            }


            try {
                rightValue = std::stoi(right);
            }
            catch (const std::exception&) {
                auto it = variables.find(right);
                if (it != variables.end()) {
                    rightValue = it->second;
                }
                else {
                    return 0; 
                }
            }
            return leftValue - rightValue;
		}
        else if (input[i] == '*') {
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            int leftValue = 0;
            int rightValue = 0;


            try {
                leftValue = std::stoi(left);
            }
            catch (const std::exception&) {
                auto it = variables.find(left);
                if (it != variables.end()) {
                    leftValue = it->second;
                }
                else {
                    return 0; 
                }
            }


            try {
                rightValue = std::stoi(right);
            }
            catch (const std::exception&) {
                auto it = variables.find(right);
                if (it != variables.end()) {
                    rightValue = it->second;
                }
                else {
                    return 0; 
                }
            }
            return leftValue * rightValue;
        }
    }
    return 0;
}

bool isLogicalOperator(char c) {
    return c == '>' || c == '<' || c == '==' || c == '<>';
}

bool LogicHandler(const std::string& input, std::map<std::string, int> variables) {
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '>') {
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            int leftValue = 0;
            int rightValue = 0;


            try {
                int leftValue = std::stoi(left);
                int rightValue = std::stoi(right);
                if (leftValue > rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
            catch (const std::exception&) {
                auto it = variables.find(left);
                if (it != variables.end()) {
                    leftValue = it->second;
                }
                else {
                    return false;
                }

                try {
                    rightValue = std::stoi(right);
                }
                catch (const std::exception&) {
                    auto it = variables.find(right);
                    if (it != variables.end()) {
                        rightValue = it->second;
                    }
                    else {
                        return false;
                    }
                }
                if (leftValue > rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }
        if (input[i] == '<') {
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            int leftValue = 0;
            int rightValue = 0;


            try {
                int leftValue = std::stoi(left);
                int rightValue = std::stoi(right);
                if (leftValue < rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
            catch (const std::exception&) {
                auto it = variables.find(left);
                if (it != variables.end()) {
                    leftValue = it->second;
                }
                else {
                    return false;
                }

                try {
                    rightValue = std::stoi(right);
                }
                catch (const std::exception&) {
                    auto it = variables.find(right);
                    if (it != variables.end()) {
                        rightValue = it->second;
                    }
                    else {
                        return false;
                    }
                }
                if (leftValue > rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }
        if (input[i] == '==') {
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            int leftValue = 0;
            int rightValue = 0;


            try {
                int leftValue = std::stoi(left);
                int rightValue = std::stoi(right);
                if (leftValue == rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
            catch (const std::exception&) {
                auto it = variables.find(left);
                if (it != variables.end()) {
                    leftValue = it->second;
                }
                else {
                    return false;
                }

                try {
                    rightValue = std::stoi(right);
                }
                catch (const std::exception&) {
                    auto it = variables.find(right);
                    if (it != variables.end()) {
                        rightValue = it->second;
                    }
                    else {
                        return false;
                    }
                }
                if (leftValue > rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }
        if (input[i] == '<>') {
            std::string left, right;
            for (size_t j = 0; j < i; j++) {
                left += input[j];
            }
            for (size_t j = i + 1; j < input.size(); j++) {
                right += input[j];
            }
            left = trim(left);
            right = trim(right);

            int leftValue = 0;
            int rightValue = 0;


            try {
                int leftValue = std::stoi(left);
                int rightValue = std::stoi(right);
                if (leftValue != rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
            catch (const std::exception&) {
                auto it = variables.find(left);
                if (it != variables.end()) {
                    leftValue = it->second;
                }
                else {
                    return false;
                }

                try {
                    rightValue = std::stoi(right);
                }
                catch (const std::exception&) {
                    auto it = variables.find(right);
                    if (it != variables.end()) {
                        rightValue = it->second;
                    }
                    else {
                        return false;
                    }
                }
                if (leftValue > rightValue) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }
    }
}