#include "pch.h"
#include "Canvas.h"
#include <algorithm>

Canvas::Canvas(int width, int height)
    : initialWidth(width), initialHeight(height),
      minUsedX(INT_MAX), maxUsedX(INT_MIN), 
      minUsedY(INT_MAX), maxUsedY(INT_MIN),
      hasContent(false),
      offsetX(0), offsetY(0)
{
    // Inicjalizuj grid jako vector 2D
    grid.resize(height);
    for (int i = 0; i < height; ++i) {
        grid[i].resize(width, ' ');
    }
}

void Canvas::expandIfNeeded(int x, int y) {
    // Przelicz na wewnętrzne współrzędne z uwzględnieniem offsetu
    int internalX = x + offsetX;
    int internalY = y + offsetY;
    
    int currentHeight = static_cast<int>(grid.size());
    int currentWidth = currentHeight > 0 ? static_cast<int>(grid[0].size()) : 0;
    
    // Rozszerzanie w dół (y >= height)
    if (internalY >= currentHeight) {
        int newHeight = internalY + 1;
        grid.resize(newHeight);
        for (int i = currentHeight; i < newHeight; ++i) {
            grid[i].resize(currentWidth, ' ');
        }
    }
    
    // Rozszerzanie w prawo (x >= width)
    if (internalX >= currentWidth) {
        int newWidth = internalX + 1;
        for (auto& row : grid) {
            row.resize(newWidth, ' ');
        }
    }
    
    // Rozszerzanie w górę (y < 0 po przeliczeniu) - wymaga przesunięcia
    if (internalY < 0) {
        int addRows = -internalY;
        currentHeight = static_cast<int>(grid.size());
        currentWidth = currentHeight > 0 ? static_cast<int>(grid[0].size()) : initialWidth;
        
        std::vector<std::vector<char>> newGrid(currentHeight + addRows);
        
        // Nowe puste wiersze na górze
        for (int i = 0; i < addRows; ++i) {
            newGrid[i].resize(currentWidth, ' ');
        }
        // Kopiuj stare wiersze
        for (int i = 0; i < currentHeight; ++i) {
            newGrid[i + addRows] = std::move(grid[i]);
        }
        grid = std::move(newGrid);
        
        // Zaktualizuj offset - wszystkie Y są teraz przesunięte
        offsetY += addRows;
        
        // Zaktualizuj śledzenie użytego obszaru
        if (hasContent) {
            minUsedY += addRows;
            maxUsedY += addRows;
        }
    }
    
    // Rozszerzanie w lewo (x < 0 po przeliczeniu) - wymaga przesunięcia
    if (internalX < 0) {
        int addCols = -internalX;
        currentWidth = static_cast<int>(grid[0].size());
        int newWidth = currentWidth + addCols;
        
        for (auto& row : grid) {
            std::vector<char> newRow(newWidth, ' ');
            for (int j = 0; j < static_cast<int>(row.size()); ++j) {
                newRow[j + addCols] = row[j];
            }
            row = std::move(newRow);
        }
        
        // Zaktualizuj offset - wszystkie X są teraz przesunięte
        offsetX += addCols;
        
        // Zaktualizuj śledzenie użytego obszaru
        if (hasContent) {
            minUsedX += addCols;
            maxUsedX += addCols;
        }
    }
}

void Canvas::setPixel(int x, int y, char c) {
    // Automatycznie rozszerz canvas jeśli potrzeba
    expandIfNeeded(x, y);
    
    // Przelicz na wewnętrzne współrzędne
    int internalX = x + offsetX;
    int internalY = y + offsetY;
    
    if (internalY >= 0 && internalY < static_cast<int>(grid.size()) && 
        internalX >= 0 && internalX < static_cast<int>(grid[internalY].size())) {
        grid[internalY][internalX] = c;
        
        // Aktualizuj granice użytego obszaru
        hasContent = true;
        minUsedX = std::min(minUsedX, internalX);
        maxUsedX = std::max(maxUsedX, internalX);
        minUsedY = std::min(minUsedY, internalY);
        maxUsedY = std::max(maxUsedY, internalY);
    }
}

std::vector<std::vector<char>>& Canvas::getGrid() {
    return grid;
}

const std::vector<std::vector<char>>& Canvas::getGrid() const {
    return grid;
}

int Canvas::getWidth() const {
    return grid.empty() ? 0 : static_cast<int>(grid[0].size());
}

int Canvas::getHeight() const {
    return static_cast<int>(grid.size());
}

int Canvas::getInitialWidth() const {
    return initialWidth;
}

int Canvas::getInitialHeight() const {
    return initialHeight;
}

void Canvas::getBounds(int& minX, int& maxX, int& minY, int& maxY) const {
    if (!hasContent) {
        minX = maxX = minY = maxY = 0;
        return;
    }
    minX = minUsedX;
    maxX = maxUsedX;
    minY = minUsedY;
    maxY = maxUsedY;
}

void Canvas::trim() {
    if (!hasContent) {
        // Pusta plansza - zostaw jeden wiersz z jednym znakiem
        grid.clear();
        grid.resize(1);
        grid[0].resize(1, ' ');
        return;
    }
    
    // Utwórz nowy grid tylko z użytym obszarem
    int newHeight = maxUsedY - minUsedY + 1;
    int newWidth = maxUsedX - minUsedX + 1;
    
    std::vector<std::vector<char>> trimmedGrid(newHeight);
    for (int i = 0; i < newHeight; ++i) {
        trimmedGrid[i].resize(newWidth);
        for (int j = 0; j < newWidth; ++j) {
            trimmedGrid[i][j] = grid[minUsedY + i][minUsedX + j];
        }
    }
    
    grid = std::move(trimmedGrid);
    
    // Zaktualizuj granice i offset
    offsetX = offsetX - minUsedX;
    offsetY = offsetY - minUsedY;
    maxUsedX = newWidth - 1;
    maxUsedY = newHeight - 1;
    minUsedX = 0;
    minUsedY = 0;
}

