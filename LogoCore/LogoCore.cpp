// LogoCore.cpp : Definiuje funkcje biblioteki statycznej.
//

#include "pch.h"
#include "framework.h"
#include <iostream>
#include "LogoCore.h"
#include "InstructionHandler.h"
#include "Turtle.h"
#include "Canvas.h"

char** TurtleInstructions(std::string instructions, int width, int height, char pen)
{
	Canvas canvas(width, height);
	Turtle turtle(canvas, pen);
    Instruction instructionHandler(turtle);
    
    instructionHandler.Instrucions(&instructions);

    return canvas.getGrid();
}
