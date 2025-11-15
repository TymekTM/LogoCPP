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

    std::string dataStr = Tokenizer().ExtractData(instruction);
    int data = std::stoi(dataStr);

    if (command == "Forward" || command == "forward") {
        turtle.Forward(data);
    }
    else if (command == "Backward" || command == "backward") {
        turtle.Backward(data);
    }
    else if (command == "Left" || command == "left") {
        turtle.Left(data);
    }
    else if (command == "Right" || command == "right") {
        turtle.Right(data);
    }
    else if (command == "def") {

    }
    else if (command == "var") {

    }
}


