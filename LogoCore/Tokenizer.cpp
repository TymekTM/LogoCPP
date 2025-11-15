#include "pch.h"
#include "Tokenizer.h"
#include <string>
#include <cctype>

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

std::string Tokenizer::ExtractData(const std::string& input) {
    std::string data;
    
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '(') {

            for (size_t j = i + 1; j < input.size(); j++) {
                if (input[j] == ')') {

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
    
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '(') {
            return trim(command);
        }
        command += input[i];
    }
    return trim(command);
}

