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
        sinTable[i] = static_cast<int>(std::round(std::sin(rad) * TRIG_SCALE));
        cosTable[i] = static_cast<int>(std::round(std::cos(rad) * TRIG_SCALE));
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
    
    char* __restrict g = canvas.grid;
    const int gw = canvas.gridWidth;
    const int ox = canvas.offsetX;
    const int oy = canvas.offsetY;
    const char p = pen;
    
    // Use row pointer for incremental updates (avoid multiply per pixel)
    char* __restrict row = g + (y0 + oy) * gw;
    const int rowDelta = sy * gw;
    int cx = x0 + ox;
    
    while (true) {
        row[cx] = p;
        
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; cx += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; row += rowDelta; }
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
    
    int idx = angle * 10;  // angle is already in [0, 360)
    
    int cs = cosTable[idx];
    int sn = sinTable[idx];
    
    int dx = distance * cs;
    int dy = distance * sn;
    // Power-of-2 rounding: (val + HALF) >> SHIFT for positive, -((-val + HALF) >> SHIFT) for negative
    int newX = posX + (dx >= 0 ? (dx + TRIG_HALF) >> TRIG_SHIFT : -(((-dx) + TRIG_HALF) >> TRIG_SHIFT));
    int newY = posY + (dy >= 0 ? (dy + TRIG_HALF) >> TRIG_SHIFT : -(((-dy) + TRIG_HALF) >> TRIG_SHIFT));
    
    if (penDown) {
        int adx = newX - posX;
        int ady = newY - posY;
        // Fast path: movement ≤ 1 pixel in each axis → at most 2 pixels, skip Bresenham
        if ((unsigned)(adx + 1) <= 2u && (unsigned)(ady + 1) <= 2u) [[likely]] {
            const int ox = canvas.offsetX, oy = canvas.offsetY;
            const int gw = canvas.gridWidth;
            const unsigned ugw = static_cast<unsigned>(gw);
            const unsigned ugh = static_cast<unsigned>(canvas.gridHeight);
            char* __restrict g = canvas.grid;
            const char p = pen;
            int ix0 = posX + ox, iy0 = posY + oy;
            if ((unsigned)ix0 < ugw && (unsigned)iy0 < ugh) [[likely]]
                g[iy0 * gw + ix0] = p;
            if (adx | ady) { // not same pixel
                int ix1 = newX + ox, iy1 = newY + oy;
                if ((unsigned)ix1 < ugw && (unsigned)iy1 < ugh) [[likely]]
                    g[iy1 * gw + ix1] = p;
            }
        } else {
            drawLine(posX, posY, newX, newY);
        }
    }
    
    posX = newX;
    posY = newY;
}

void Turtle::Backward(int distance) {
    if (distance <= 0) return;
    
    int a = angle + 180;
    if (a >= 360) a -= 360;  // angle was in [0,360), so a is in [180,540) → just subtract 360 if needed
    int idx = a * 10;
    
    int cs = cosTable[idx];
    int sn = sinTable[idx];
    
    int dx = distance * cs;
    int dy = distance * sn;
    int newX = posX + (dx >= 0 ? (dx + TRIG_HALF) >> TRIG_SHIFT : -(((-dx) + TRIG_HALF) >> TRIG_SHIFT));
    int newY = posY + (dy >= 0 ? (dy + TRIG_HALF) >> TRIG_SHIFT : -(((-dy) + TRIG_HALF) >> TRIG_SHIFT));
    
    if (penDown) {
        int adx = newX - posX;
        int ady = newY - posY;
        if ((unsigned)(adx + 1) <= 2u && (unsigned)(ady + 1) <= 2u) [[likely]] {
            const int ox = canvas.offsetX, oy = canvas.offsetY;
            const int gw = canvas.gridWidth;
            const unsigned ugw = static_cast<unsigned>(gw);
            const unsigned ugh = static_cast<unsigned>(canvas.gridHeight);
            char* __restrict g = canvas.grid;
            const char p = pen;
            int ix0 = posX + ox, iy0 = posY + oy;
            if ((unsigned)ix0 < ugw && (unsigned)iy0 < ugh) [[likely]]
                g[iy0 * gw + ix0] = p;
            if (adx | ady) {
                int ix1 = newX + ox, iy1 = newY + oy;
                if ((unsigned)ix1 < ugw && (unsigned)iy1 < ugh) [[likely]]
                    g[iy1 * gw + ix1] = p;
            }
        } else {
            drawLine(posX, posY, newX, newY);
        }
    }
    
    posX = newX;
    posY = newY;
}
