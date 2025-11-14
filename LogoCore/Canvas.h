#pragma once
class Canvas {
public:
    Canvas(int width, int height);
    void setPixel(int x, int y, char c);
    char** getGrid() const;
    int width;
    int height;
private:
    char** grid;
};
