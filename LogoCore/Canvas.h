#pragma once
#include <vector>
#include <cstring>
#include <algorithm>

class Canvas {
public:
    Canvas(int width, int height, bool skipInit = false);
    ~Canvas();
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;
    
    void reset(int width, int height, bool skipInit = false);
    
    inline void setPixel(int x, int y, char c = '*') {
        int ix = x + offsetX;
        int iy = y + offsetY;
        if (ix >= 0 && ix < gridWidth && iy >= 0 && iy < gridHeight) [[likely]] {
            grid[iy * gridWidth + ix] = c;
        } else {
            setPixelSlow(x, y, c);
        }
    }
    
    std::vector<std::vector<char>>& getGrid();
    const std::vector<std::vector<char>>& getGrid() const;
    
    int getWidth() const { return gridWidth; }
    int getHeight() const { return gridHeight; }
    int getInitialWidth() const { return initialWidth; }
    int getInitialHeight() const { return initialHeight; }
    
    void trim();
    void getBounds(int& minX, int& maxX, int& minY, int& maxY) const;

    char* grid;
    int gridWidth;
    int gridHeight;
    int offsetX, offsetY;
    
private:
    int initialWidth;
    int initialHeight;
    
    bool hasContent;
    
    mutable std::vector<std::vector<char>> vecCache;
    mutable bool vecCacheDirty;
    
    void expandIfNeeded(int x, int y);
    void setPixelSlow(int x, int y, char c);
};
