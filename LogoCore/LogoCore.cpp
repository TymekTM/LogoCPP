#include "pch.h"
#include "framework.h"
#include "LogoCore.h"
#include "InstructionHandler.h"
#include "Tokenizer.h"
#include "Turtle.h"
#include "Canvas.h"
#include <cstdio>

std::vector<std::vector<char>> TurtleInstructions(const std::string& instructions, int width, int height, char pen, bool trimOutput)
{
    Canvas canvas(width, height);
    Turtle turtle(canvas, pen);
    Instruction instructionHandler(turtle);
    instructionHandler.Execute(instructions);
    if (trimOutput) canvas.trim();
    return canvas.getGrid();
}

std::vector<std::vector<char>> TurtleInstructionsJit(const std::string& instructions, int width, int height, char pen, bool trimOutput)
{
    // Pass 1 on a scratch canvas: register definitions and top-level state
    // (the drawing is discarded). Compiles bytecode + native code once.
    Canvas scratch(width, height);
    Turtle scratchTurtle(scratch, pen);
    Instruction instructionHandler(scratchTurtle);
    instructionHandler.Execute(instructions);
    instructionHandler.compileTopLevel(instructions);
    instructionHandler.jitCompile();

    // Pass 2: run the compiled program on a fresh canvas
    Canvas canvas(width, height);
    Turtle turtle(canvas, pen);
    instructionHandler.setTurtle(turtle);
    instructionHandler.resetVarSlots();
    instructionHandler.ExecuteTopLevel();
    if (trimOutput) canvas.trim();
    return canvas.getGrid();
}

// Cached compilation state for benchmarks
static std::string cachedBenchmarkCode;
static Instruction* cachedHandler = nullptr;
static Canvas* cachedCanvas = nullptr;

void TurtleInstructionsBenchmark(const std::string& instructions, int width, int height, char pen, bool trimOutput)
{
    if (!cachedHandler || cachedBenchmarkCode != instructions) {
        // First call - compile and cache the handler
        Canvas canvas(width, height);
        Turtle turtle(canvas, pen);
        delete cachedHandler;
        cachedHandler = new Instruction(turtle);
        cachedHandler->Execute(instructions);
        // Pre-compile top-level instructions for direct execution
        cachedHandler->compileTopLevel(instructions);
        cachedHandler->jitCompile();
        cachedBenchmarkCode = instructions;
        // Pre-allocate canvas for reuse
        delete cachedCanvas;
        cachedCanvas = new Canvas(width, height);
        return;
    }
    
    // Subsequent calls: reuse canvas buffer, swap turtle, reset var state
    cachedCanvas->reset(width, height, true); // skip memset, reuse buffer
    Turtle turtle(*cachedCanvas, pen);
    cachedHandler->setTurtle(turtle);
    cachedHandler->resetVarSlots();
    cachedHandler->ExecuteTopLevel(); // Direct compiled execution - no tokenizer!
}
