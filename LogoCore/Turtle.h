#pragma once

class Turtle {
	public:
	Turtle(Instruction& instruction, Canvas& canvas, char pen);
	void Forward(int distance);
	void Backward(int distance);
	void Left(int angle);
	void Right(int angle);

	private:
		Instruction& instruction;
		Canvas& canvas;
		char pen;
		int posX;
		int posY;
		int angle = 0;
};