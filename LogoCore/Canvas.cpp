#include "pch.h"
#include "Canvas.h"
#include <algorithm>
#include <climits>

Canvas::Canvas(int width, int height, bool skipInit)
    : initialWidth(width), initialHeight(height),
      hasContent(false), offsetX(0), offsetY(0),
      vecCacheDirty(true)
{
    // Allocate exact size + small margin; expand dynamically as needed
    gridWidth = width + 20;
    gridHeight = height + 20;
    offsetX = 10;
    offsetY = 10;
    grid = new char[gridWidth * gridHeight];
    if (!skipInit) std::memset(grid, ' ', gridWidth * gridHeight);
}

Canvas::~Canvas() {
    delete[] grid;
}

void Canvas::reset(int width, int height, bool skipInit) {
    int newGW = width + 20;
    int newGH = height + 20;
    int needed = newGW * newGH;
    if (needed > gridWidth * gridHeight) {
        delete[] grid;
        grid = new char[needed];
    }
    gridWidth = newGW;
    gridHeight = newGH;
    offsetX = 10;
    offsetY = 10;
    initialWidth = width;
    initialHeight = height;
    hasContent = false;
    vecCacheDirty = true;
    if (!skipInit) std::memset(grid, ' ', gridWidth * gridHeight);
}

void Canvas::expandIfNeeded(int x, int y) {
    int ix = x + offsetX;
    int iy = y + offsetY;

    int newWidth = gridWidth;
    int newHeight = gridHeight;
    int newOffX = offsetX;
    int newOffY = offsetY;
    
    if (ix < 0) {
        int add = -ix + 100;
        newWidth += add;
        newOffX += add;
    }
    if (ix >= gridWidth) {
        newWidth = ix + 101;
    }
    if (iy < 0) {
        int add = -iy + 100;
        newHeight += add;
        newOffY += add;
    }
    if (iy >= gridHeight) {
        newHeight = iy + 101;
    }
    
    if (newWidth != gridWidth || newHeight != gridHeight) {
        char* newGrid = new char[newWidth * newHeight];
        std::memset(newGrid, ' ', newWidth * newHeight);
        
        // Copy old data
        int dOffX = newOffX - offsetX;
        int dOffY = newOffY - offsetY;
        for (int row = 0; row < gridHeight; ++row) {
            std::memcpy(newGrid + (row + dOffY) * newWidth + dOffX, 
                       grid + row * gridWidth, gridWidth);
        }
        
        delete[] grid;
        grid = newGrid;
        
        offsetX = newOffX;
        offsetY = newOffY;
        gridWidth = newWidth;
        gridHeight = newHeight;
    }
}

void Canvas::setPixelSlow(int x, int y, char c) {
    expandIfNeeded(x, y);
    int ix = x + offsetX;
    int iy = y + offsetY;
    grid[iy * gridWidth + ix] = c;
    hasContent = true;
}

std::vector<std::vector<char>>& Canvas::getGrid() {
    vecCache.resize(gridHeight);
    for (int i = 0; i < gridHeight; ++i) {
        vecCache[i].assign(grid + i * gridWidth, grid + (i + 1) * gridWidth);
    }
    return vecCache;
}

const std::vector<std::vector<char>>& Canvas::getGrid() const {
    vecCache.resize(gridHeight);
    for (int i = 0; i < gridHeight; ++i) {
        vecCache[i].assign(grid + i * gridWidth, grid + (i + 1) * gridWidth);
    }
    return vecCache;
}

void Canvas::getBounds(int& minX, int& maxX, int& minY, int& maxY) const {
    // Scan grid for actual content bounds
    minX = gridWidth; maxX = -1; minY = gridHeight; maxY = -1;
    for (int y = 0; y < gridHeight; ++y) {
        const char* row = grid + y * gridWidth;
        for (int x = 0; x < gridWidth; ++x) {
            if (row[x] != ' ') {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
        }
    }
    if (maxX < 0) { minX = maxX = minY = maxY = 0; }
}

void Canvas::trim() {
    int minX, maxX, minY, maxY;
    getBounds(minX, maxX, minY, maxY);
    if (maxX < 0) {
        delete[] grid;
        gridWidth = 1;
        gridHeight = 1;
        grid = new char[1];
        grid[0] = ' ';
        return;
    }
    
    int newW = maxX - minX + 1;
    int newH = maxY - minY + 1;
    char* newGrid = new char[newW * newH];
    
    for (int i = 0; i < newH; ++i) {
        std::memcpy(newGrid + i * newW,
                   grid + (minY + i) * gridWidth + minX, newW);
    }
    
    delete[] grid;
    grid = newGrid;
    offsetX = offsetX - minX;
    offsetY = offsetY - minY;
    gridWidth = newW;
    gridHeight = newH;
}
