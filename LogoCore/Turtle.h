#pragma once
#include "Canvas.h"

class Turtle {
public:
    Turtle(Canvas& canvas, char pen);
    void Forward(int distance);
    void Backward(int distance);
    void Left(int angle) { this->angle -= angle; }
    void Right(int angle) { this->angle += angle; }
    void PenUp() { penDown = false; }
    void PenDown() { penDown = true; }
    
    int getPosX() const { return posX; }
    int getPosY() const { return posY; }

private:
    Canvas& canvas;
    char pen;
    int posX;
    int posY;
    int angle = 0;
    bool penDown = true;
    
    // Precomputed sin/cos tables (x1000 fixed point)
    static constexpr int TABLE_SIZE = 3600; // 0.1 degree precision
    static int sinTable[TABLE_SIZE];
    static int cosTable[TABLE_SIZE];
    static bool tablesInitialized;
    static void initTables();
    
    // Bresenham line draw
    void drawLine(int x0, int y0, int x1, int y1);
    void drawLineFast(int x0, int y0, int x1, int y1);
    void drawLineSlow(int x0, int y0, int x1, int y1);
};
