#pragma once

class Turtle {
	public:
	Turtle(Instruction instruction, Canvas canvas);
	void Forward(int distance);
	void Backward(int distance);
	void Left(int angle);
	void Right(int angle);

};