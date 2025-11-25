#pragma once
#include <vector>

class Canvas {
public:
    Canvas(int width, int height);
    void setPixel(int x, int y, char c = '*');
    
    std::vector<std::vector<char>>& getGrid();
    const std::vector<std::vector<char>>& getGrid() const;
    
    // Zwraca aktualny rozmiar (może być większy niż początkowy po rozszerzeniu)
    int getWidth() const;
    int getHeight() const;
    
    // Początkowy rozmiar (do określenia środka)
    int getInitialWidth() const;
    int getInitialHeight() const;
    
    // Trimuje canvas do minimalnego prostokąta zawierającego wszystkie znaki
    void trim();
    
    // Zwraca granice użytego obszaru (minX, maxX, minY, maxY)
    void getBounds(int& minX, int& maxX, int& minY, int& maxY) const;
    
private:
    std::vector<std::vector<char>> grid;
    int initialWidth;
    int initialHeight;
    
    // Śledzenie granic dla optymalizacji trimowania
    int minUsedX, maxUsedX, minUsedY, maxUsedY;
    bool hasContent;
    
    // Offset do śledzenia przesunięć przy dynamicznym rozszerzaniu
    int offsetX, offsetY;
    
    // Rozszerza canvas w danym kierunku
    void expandIfNeeded(int x, int y);
};
