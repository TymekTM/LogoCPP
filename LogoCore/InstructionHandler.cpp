#include "pch.h"
#include "InstructionHandler.h"
#include "Tokenizer.h"
#include "Turtle.h"

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
	std::string command = Tokenizer().ExtractCommand(instruction);


    if (command == "var") {
		variables.merge(Tokenizer().VariableHandler(instruction));
        return;
    }

    if (command == "if") {
        Tokenizer tokenizer;
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

    std::string dataStr = Tokenizer().ExtractData(instruction, variables);

    if (dataStr.empty()) {
        return;
    }
    
    int data = std::stoi(dataStr);

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


