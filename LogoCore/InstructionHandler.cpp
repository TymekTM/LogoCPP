#include "pch.h"
#include "InstructionHandler.h"
#include "Tokenizer.h"
#include "Turtle.h"
#include <cmath>

Instruction::Instruction(Turtle& turtle)
    : turtle(turtle)
{
}

void Instruction::Instrucions(string* instructionSet)
{
	Tokenizer tokenizer;
	for (const auto& instruction : tokenizer.Tokenize(*instructionSet)) {
		HandleInstruction(instruction);
	}
}

void Instruction::HandleInstruction(string instruction)
{
	Tokenizer tokenizer;
	std::string command = tokenizer.ExtractCommand(instruction);

    // Obs³uga definiowania funkcji
    if (command == "def") {
        std::string functionName = tokenizer.ExtractFunctionName(instruction);
        if (functionName.empty()) return;
        
        std::vector<std::string> params = tokenizer.ExtractArguments(instruction);
        
        size_t closeParenPos = instruction.find(')', instruction.find('('));
        size_t bracketPos = instruction.find('{', closeParenPos);
        
        if (bracketPos != std::string::npos) {
            std::string functionBody = tokenizer.ExtractBracketsContent(instruction, bracketPos);
            
            FunctionDefinition funcDef;
            funcDef.parameters = params;
            funcDef.body = functionBody;
            
            functions[functionName] = funcDef;
        }
        
        return;
    }

    // SprawdŸ czy to wywo³anie funkcji
    auto funcIt = functions.find(command);
    if (funcIt != functions.end()) {
        std::vector<std::string> args = tokenizer.ExtractArguments(instruction);
        
        // Utwórz lokaln¹ kopiê zmiennych
        std::map<std::string, double> localVariables = variables;
        
        // Przypisz argumenty do parametrów
        const FunctionDefinition& funcDef = funcIt->second;
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
        
        // Tymczasowo podmieñ zmienne na lokalne
        std::map<std::string, double> savedVariables = variables;
        variables = localVariables;
        
        // Wykonaj cia³o funkcji
        std::string body = funcDef.body;
        Instrucions(&body);
        
        // Przywróæ zmienne globalne
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
        variables = savedVariables;
        
        return;
    }

    if (command == "var") {
		variables.merge(tokenizer.VariableHandler(instruction));
        return;
    }

    if (command == "if") {
        std::string condition = tokenizer.ExtractData(instruction, variables);
        
        if (condition.empty()) {
            return;
        }
        
        bool conditionResult = tokenizer.LogicHandler(condition, variables);
        
        if (conditionResult) {
            size_t bracketPos = instruction.find('{');
            if (bracketPos != std::string::npos) {
                std::string blockContent = tokenizer.ExtractBracketsContent(instruction, bracketPos);
                
                if (!blockContent.empty()) {
                    Instrucions(&blockContent);
                }
            }
        }
        return;
    }

    std::string dataStr = tokenizer.ExtractData(instruction, variables);

    if (dataStr.empty()) {
        return;
    }
    
    int data = 0;
    try {
        data = static_cast<int>(std::round(std::stod(dataStr)));
    } catch (const std::exception&) {
        return;
    }

    if (command == "Forward" || command == "forward") {
        turtle.Forward(data);
		return;
    }
    else if (command == "Backward" || command == "backward") {
        turtle.Backward(data);
		return;
    }
    else if (command == "Left" || command == "left") {
        turtle.Left(data);
        return;
    }
    else if (command == "Right" || command == "right") {
        turtle.Right(data);
        return;
    }
}


