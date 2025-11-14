#include "pch.h"
#include "Canvas.h"

Canvas::Canvas(int width, int height)
{
    this->width = width;
    this->height = height;

    grid = new char* [height];
    for (int i = 0; i < height; ++i) {
        grid[i] = new char[width];
        for (int j = 0; j < width; ++j) {
            grid[i][j] = ' ';
        }
    }
}

void Canvas::setPixel(int x, int y, char c= '*') {
    if (y >= 0 && y < height && x >= 0 && x < width) {
        grid[y][x] = c;
    }
}

char** Canvas::getGrid() const {
    return grid;
}

