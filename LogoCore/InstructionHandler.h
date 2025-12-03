#pragma once
#include <string>
#include <map>
#include <vector>
#include <unordered_map>

class Turtle;
class Tokenizer;

struct FunctionDefinition {
    std::vector<std::string> parameters;
    std::string body;
};

// Enum dla szybkiego rozpoznawania komend
enum class CommandType {
    Unknown,
    Forward,
    Backward,
    Left,
    Right,
    Var,
    If,
    Def,
    Function  // wywołania funkcji użytkownika
};

class Instruction {
public:
    Instruction(Turtle& turtle);
    
    // Główna metoda wykonywania instrukcji
    void Execute(const std::string& instructionSet);
    
    // Obsługa pojedynczej instrukcji
    void HandleInstruction(const std::string& instruction, Tokenizer& tokenizer);
    
    // Szybkie rozpoznawanie komend
    CommandType GetCommandType(const std::string& command) const;

    std::map<std::string, double> variables;
    std::map<std::string, FunctionDefinition> functions;
    
private:
    Turtle& turtle;
    
    // Cache dla rozpoznawania komend
    static const std::unordered_map<std::string, CommandType> commandLookup;
    static std::unordered_map<std::string, CommandType> InitCommandLookup();
};