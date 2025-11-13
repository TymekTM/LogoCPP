#pragma once
class Canvas {
public:
    Canvas(int width, int height);
    void setPixel(int x, int y, char c);
    int width;
    int height;
private:
    char** grid;
};
