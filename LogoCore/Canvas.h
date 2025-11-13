#pragma once
class Canvas {
public:
    Canvas(int width, int height);
    void setPixel(int x, int y, char c);
private:
    int width;
    int height;
    char** grid;
};
