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

	private:
		Canvas& canvas;
		char pen;
		int posX;
		int posY;
		int angle = 0;
};