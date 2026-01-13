#pragma once
#include "Canvas.h"
#include "InstructionHandler.h"

class Turtle {
	public:
	Turtle(Canvas& canvas, char pen);
	void Forward(int distance);
	void Backward(int distance);
	void Left(int angle);
	void Right(int angle);
	void PenUp();
	void PenDown();
	
	// Gettery pozycji (dla debugowania/testów)
	int getPosX() const { return posX; }
	int getPosY() const { return posY; }

	private:
		Canvas& canvas;
		char pen;
		int posX;
		int posY;
		int angle = 0;
		bool penDown = true;  // Stan pióra - domyślnie opuszczone (rysuje)
		
		// Offset do śledzenia przesunięć canvas przy dynamicznym rozszerzaniu
		int offsetX = 0;
		int offsetY = 0;
};