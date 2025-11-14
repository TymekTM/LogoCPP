#pragma once
#include <string>

class Turtle; 

using std::string;

class Instruction {
    public:
    Instruction(Turtle& turtle);
    void Instrucions(string* instructionSet);
	void HandleInstruction(string instruction);
    
    private:
    Turtle& turtle;
};