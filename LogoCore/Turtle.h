#pragma once
#include "Canvas.h"

class Turtle {
public:
    Turtle(Canvas& canvas, char pen);
    void Forward(int distance);
    void Backward(int distance);
    void Left(int angle) {
        this->angle -= angle;
        if (this->angle < 0) this->angle += 360;
        else if (this->angle >= 360) this->angle -= 360;
    }
    void Right(int angle) {
        this->angle += angle;
        if (this->angle >= 360) this->angle -= 360;
        else if (this->angle < 0) this->angle += 360;
    }
    void PenUp() { penDown = false; }
    void PenDown() { penDown = true; }
    
    int getPosX() const { return posX; }
    int getPosY() const { return posY; }

    // Complete a precomputed move: draw when the pen is down, then update
    // position. The JIT emits an exact inline copy of this logic, so any
    // change here must be mirrored in Jit.cpp (emitMove).
    void MoveTo(int nx, int ny);

    // Hot state, accessed directly by the JIT-generated machine code
    // (offsets baked in via offsetof in Jit.cpp). Do not reorder.
    int posX;
    int posY;
    int angle = 0;
    bool penDown = true;
    Canvas& canvas;
    char pen;

    // Fixed-point trig constants (x8192, power-of-2 for fast shifts)
    static constexpr int TRIG_SHIFT = 13;                 // 2^13 = 8192
    static constexpr int TRIG_SCALE = 1 << TRIG_SHIFT;    // 8192
    static constexpr int TRIG_HALF = TRIG_SCALE >> 1;     // 4096

    // Trig tables, read directly by JIT-generated code via r14/r15.
    static constexpr int TABLE_SIZE = 3600; // 0.1 degree precision
    static int sinTable[TABLE_SIZE];
    static int cosTable[TABLE_SIZE];

private:
    static bool tablesInitialized;
    static void initTables();
    
    // Bresenham line draw
    void drawLine(int x0, int y0, int x1, int y1);
    void drawLineFast(int x0, int y0, int x1, int y1);
    void drawLineSlow(int x0, int y0, int x1, int y1);
};
