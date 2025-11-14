// LogoCore.cpp : Definiuje funkcje biblioteki statycznej.
//

#include "pch.h"
#include "framework.h"
#include <iostream>
#include "LogoCore.h"
#include "InstructionHandler.h"
#include "Turtle.h"

// TODO: To jest przykład funkcji biblioteki
void fnLogoCore()
{
}

void LogoCoreTest()
{
    std::cout << "Biblioteka LogoCore dodana i dziala!" << std::endl;
}

char** TurtleInstructions(std::string instructions, int width, int hight, char pen)
{
	Canvas canvas(width, hight);

	Turtle turtle(*(new Instruction()), canvas, pen);

    Instruction instructionHandler;
    instructionHandler.Instrucions(&instructions);

    char** result = new char* [1];
    return result;
}
