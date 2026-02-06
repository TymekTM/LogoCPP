#include "pch.h"
#include "Turtle.h"
#include "Canvas.h"
#include <cmath>
#include <numbers>
#include <cstdlib>

int Turtle::sinTable[TABLE_SIZE];
int Turtle::cosTable[TABLE_SIZE];
bool Turtle::tablesInitialized = false;

void Turtle::initTables() {
    if (tablesInitialized) return;
    for (int i = 0; i < TABLE_SIZE; ++i) {
        double rad = i * std::numbers::pi / 1800.0;
        sinTable[i] = static_cast<int>(std::round(std::sin(rad) * 10000.0));
        cosTable[i] = static_cast<int>(std::round(std::cos(rad) * 10000.0));
    }
    tablesInitialized = true;
}

Turtle::Turtle(Canvas& canvas, char pen)
    : canvas(canvas), pen(pen)
{
    initTables();
    posX = canvas.getInitialWidth() / 2;
    posY = canvas.getInitialHeight() / 2;
}

void Turtle::drawLine(int x0, int y0, int x1, int y1) {
    // Check if both endpoints are within canvas - if so use branchless fast path
    const int ox = canvas.offsetX, oy = canvas.offsetY;
    const int gw = canvas.gridWidth, gh = canvas.gridHeight;
    
    if ((unsigned)(x0 + ox) < (unsigned)gw && (unsigned)(y0 + oy) < (unsigned)gh &&
        (unsigned)(x1 + ox) < (unsigned)gw && (unsigned)(y1 + oy) < (unsigned)gh) {
        drawLineFast(x0, y0, x1, y1);
    } else {
        drawLineSlow(x0, y0, x1, y1);
    }
}

void Turtle::drawLineFast(int x0, int y0, int x1, int y1) {
    // Both endpoints in bounds - no per-pixel bounds check needed
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    
    char* g = canvas.grid;
    const int gw = canvas.gridWidth;
    const int ox = canvas.offsetX;
    const int oy = canvas.offsetY;
    
    while (true) {
        g[(y0 + oy) * gw + (x0 + ox)] = pen;
        
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Turtle::drawLineSlow(int x0, int y0, int x1, int y1) {
    // Bounds checking per pixel, handles expansion
    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    
    while (true) {
        canvas.setPixel(x0, y0, pen);
        
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Turtle::Forward(int distance) {
    if (distance <= 0) return;
    
    int a = angle % 360;
    if (a < 0) a += 360;
    int idx = a * 10;
    
    int cs = cosTable[idx];
    int sn = sinTable[idx];
    
    // Integer rounding: (val + 5000) / 10000 rounds to nearest for positive,
    // (val - 5000) / 10000 for negative
    int dx = distance * cs;
    int dy = distance * sn;
    int newX = posX + (dx >= 0 ? (dx + 5000) / 10000 : (dx - 5000) / 10000);
    int newY = posY + (dy >= 0 ? (dy + 5000) / 10000 : (dy - 5000) / 10000);
    
    if (penDown) {
        drawLine(posX, posY, newX, newY);
    }
    
    posX = newX;
    posY = newY;
}

void Turtle::Backward(int distance) {
    if (distance <= 0) return;
    
    int a = (angle + 180) % 360;
    if (a < 0) a += 360;
    int idx = a * 10;
    
    int cs = cosTable[idx];
    int sn = sinTable[idx];
    
    int dx = distance * cs;
    int dy = distance * sn;
    int newX = posX + (dx >= 0 ? (dx + 5000) / 10000 : (dx - 5000) / 10000);
    int newY = posY + (dy >= 0 ? (dy + 5000) / 10000 : (dy - 5000) / 10000);
    
    if (penDown) {
        drawLine(posX, posY, newX, newY);
    }
    
    posX = newX;
    posY = newY;
}
