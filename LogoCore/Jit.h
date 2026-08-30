#pragma once
#include <cstdint>

class Instruction;

// Compiles the instruction handler's bytecode (CInstr trees) to native x86-64
// machine code. Each Logo function becomes one native function with the
// calling convention:
//
//     void fn(Turtle* turtle /*rcx*/, double* vars /*rdx*/, double* args /*r8*/)
//
// Globals live in vars[64] (base cached in RBX, callee-saved), the turtle
// pointer is cached in RSI, and function parameters live in the callee's own
// stack frame (the interpreter's save/restore of param slots becomes native
// frame lifetime). Falls back silently to the bytecode executor when JIT is
// unavailable or disabled (LOGOCPP_NO_JIT=1).
class Jit {
public:
    // Emits native code for every compiled function plus the top-level
    // program; on success stores the top-level entry in ih.jitEntry.
    // Returns false when nothing was compiled.
    static bool compile(Instruction& ih);
};
