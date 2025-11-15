#pragma once
#include <string>
#include <map>

class Turtle; 

using std::string;

class Instruction {
    public:
    Instruction(Turtle& turtle);
    void Instrucions(string* instructionSet);
	void HandleInstruction(string instruction);
    
    private:
    Turtle& turtle;
    std::map<string, int> variables;
	std::map<string, string> procedures;
};