// LogoCore.cpp : Definiuje funkcje biblioteki statycznej.
//

#include "pch.h"
#include "framework.h"
#include <iostream>
#include "LogoCore.h"
#include "InstructionHandler.h"
#include "Turtle.h"
#include "Canvas.h"

std::vector<std::vector<char>> TurtleInstructions(const std::string& instructions, int width, int height, char pen, bool trimOutput)
{
	Canvas canvas(width, height);
	Turtle turtle(canvas, pen);
    Instruction instructionHandler(turtle);
    
    // Kopiujemy string bo Instructions wymaga non-const pointer
    std::string InstructionsCoppy = instructions;
    instructionHandler.Instrucions(&InstructionsCoppy);

    // Opcjonalne trimowanie dla optymalizacji I/O
    if (trimOutput) {
        canvas.trim();
    }

    return canvas.getGrid();
}
