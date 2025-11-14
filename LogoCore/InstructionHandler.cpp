#include "pch.h"
#include "InstructionHandler.h"
#include "Tokenizer.h"
#include "Turtle.h"

void Instruction::Instrucions(string* instructionSet)
{
	Tokenizer tokenizer;
	for (const auto& instruction : tokenizer.Tokenize(*instructionSet)) {
		HandleInstruction(instruction);
	}
}

void Instruction::HandleInstruction(string instruction)
{
	Turtle turtle(*this, *(new Canvas(100, 100)), '*');
	switch (instruction[0]) {
		case "Forward"[0]: {
			turtle.Forward(std::stoi(Tokenizer().ExtractData(instruction)));
			break;
		}
		case "Backward"[0]: {
			turtle.Backward(std::stoi(Tokenizer().ExtractData(instruction)));
			break;
		}
		case "Left"[0]: {
			turtle.Left(std::stoi(Tokenizer().ExtractData(instruction)));
			break;
		}
		case "Right"[0]: {
			turtle.Right(std::stoi(Tokenizer().ExtractData(instruction)));
			break;
		}
	}

}


