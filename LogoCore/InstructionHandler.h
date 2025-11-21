#pragma once
#include <string>
#include <map>
#include <vector>

class Turtle; 

using std::string;

struct FunctionDefinition {
    std::vector<std::string> parameters;
    std::string body;
};

class Instruction {
    public:
    Instruction(Turtle& turtle);
    void Instrucions(string* instructionSet);
	void HandleInstruction(string instruction);

    std::map<string, int> variables;
    std::map<string, string> procedures;
    std::map<string, FunctionDefinition> functions;
    
    private:
    Turtle& turtle;
    
};