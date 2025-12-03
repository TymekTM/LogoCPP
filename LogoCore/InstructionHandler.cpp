#include "pch.h"
#include "InstructionHandler.h"
#include "Tokenizer.h"
#include "Turtle.h"
#include <cmath>

// Inicjalizacja statycznej tablicy lookup dla komend
std::unordered_map<std::string, CommandType> Instruction::InitCommandLookup() {
    return {
        {"Forward", CommandType::Forward}, {"forward", CommandType::Forward},
        {"przod", CommandType::Forward}, {"Przod", CommandType::Forward},
        {"Backward", CommandType::Backward}, {"backward", CommandType::Backward},
        {"tyl", CommandType::Backward}, {"Tyl", CommandType::Backward},
        {"Left", CommandType::Left}, {"left", CommandType::Left},
        {"lewo", CommandType::Left}, {"Lewo", CommandType::Left},
        {"Right", CommandType::Right}, {"right", CommandType::Right},
        {"prawo", CommandType::Right}, {"Prawo", CommandType::Right},
        {"var", CommandType::Var},
        {"if", CommandType::If},
        {"def", CommandType::Def}
    };
}

const std::unordered_map<std::string, CommandType> Instruction::commandLookup = Instruction::InitCommandLookup();

Instruction::Instruction(Turtle& turtle)
    : turtle(turtle)
{
}

CommandType Instruction::GetCommandType(const std::string& command) const {
    auto it = commandLookup.find(command);
    if (it != commandLookup.end()) {
        return it->second;
    }
    // Sprawdź czy to zdefiniowana funkcja
    if (functions.find(command) != functions.end()) {
        return CommandType::Function;
    }
    return CommandType::Unknown;
}

void Instruction::Execute(const std::string& instructionSet) {
    Tokenizer tokenizer;
    tokenizer.TokenizeAndExecute(instructionSet, *this);
}

void Instruction::HandleInstruction(const std::string& instruction, Tokenizer& tokenizer)
{
    std::string command = tokenizer.ExtractCommand(instruction);
    CommandType cmdType = GetCommandType(command);

    // Obsługa definicji funkcji
    if (cmdType == CommandType::Def) {
        std::string functionName = tokenizer.ExtractFunctionName(instruction);
        if (functionName.empty()) return;
        
        std::vector<std::string> params = tokenizer.ExtractArguments(instruction);
        
        size_t closeParenPos = instruction.find(')', instruction.find('('));
        size_t bracketPos = instruction.find('{', closeParenPos);
        
        if (bracketPos != std::string::npos) {
            FunctionDefinition funcDef;
            funcDef.parameters = std::move(params);
            funcDef.body = tokenizer.ExtractBracketsContent(instruction, bracketPos);
            functions[functionName] = std::move(funcDef);
        }
        return;
    }

    // Wywołanie funkcji użytkownika
    if (cmdType == CommandType::Function) {
        const FunctionDefinition& funcDef = functions[command];
        std::vector<std::string> args = tokenizer.ExtractArguments(instruction);
        
        // Utwórz lokalną kopię zmiennych
        std::map<std::string, double> localVariables = variables;
        
        for (size_t i = 0; i < funcDef.parameters.size() && i < args.size(); i++) {
            try {
                if (tokenizer.IsArithmetic(args[i])) {
                    localVariables[funcDef.parameters[i]] = tokenizer.ArithmericHandler(args[i], variables);
                } else {
                    localVariables[funcDef.parameters[i]] = std::stod(args[i]);
                }
            } catch (const std::exception&) {
                auto varIt = variables.find(args[i]);
                if (varIt != variables.end()) {
                    localVariables[funcDef.parameters[i]] = varIt->second;
                }
            }
        }
        
        std::map<std::string, double> savedVariables = std::move(variables);
        variables = std::move(localVariables);
        
        tokenizer.TokenizeAndExecute(funcDef.body, *this);
        
        // Przywróć zmienne globalne (zachowaj nowe zmienne nie będące parametrami)
        for (const auto& var : variables) {
            bool isParam = false;
            for (const auto& param : funcDef.parameters) {
                if (var.first == param) {
                    isParam = true;
                    break;
                }
            }
            if (!isParam) {
                savedVariables[var.first] = var.second;
            }
        }
        variables = std::move(savedVariables);
        return;
    }

    // Obsługa zmiennych
    if (cmdType == CommandType::Var) {
        variables.merge(tokenizer.VariableHandler(instruction));
        return;
    }

    // Obsługa warunku if
    if (cmdType == CommandType::If) {
        std::string condition = tokenizer.ExtractData(instruction, variables);
        if (condition.empty()) return;
        
        if (tokenizer.LogicHandler(condition, variables)) {
            size_t bracketPos = instruction.find('{');
            if (bracketPos != std::string::npos) {
                std::string blockContent = tokenizer.ExtractBracketsContent(instruction, bracketPos);
                if (!blockContent.empty()) {
                    tokenizer.TokenizeAndExecute(blockContent, *this);
                }
            }
        }
        return;
    }

    // Komendy ruchu
    std::string dataStr = tokenizer.ExtractData(instruction, variables);
    if (dataStr.empty()) return;
    
    int data = 0;
    try {
        data = static_cast<int>(std::round(std::stod(dataStr)));
    } catch (const std::exception&) {
        return;
    }

    switch (cmdType) {
        case CommandType::Forward:
            turtle.Forward(data);
            break;
        case CommandType::Backward:
            turtle.Backward(data);
            break;
        case CommandType::Left:
            turtle.Left(data);
            break;
        case CommandType::Right:
            turtle.Right(data);
            break;
        default:
            break;
    }
}