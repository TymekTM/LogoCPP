#include "Turtle.h"
#include "InstructionHandler.h"
#include "Canvas.h"

Turtle::Turtle(Instruction& instruction, Canvas& canvas, char pen)
    : instruction(instruction), canvas(canvas), pen(pen)
{

	// Œrodek p³ótna, pozycja startowa ¿ó³wia
    posX = canvas.width / 2;
    posY = canvas.height / 2;
}

	void Turtle::Forward(int distance)
	{
		if (distance < 0) return;
		int normalized_angle = (angle % 360 + 360) % 360;
		switch (normalized_angle) {
			case 0:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX + i, posY, pen);
				}
				posX += distance;
				break;
			case 90:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX, posY - i, pen);
				}
				posY -= distance;
				break;
			case 180:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX - i, posY, pen);
				}
				posX -= distance;
				break;
			case 270:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX, posY + i, pen);
				}
				posY += distance;
				break;
		}
	}
	void Turtle::Backward(int distance)
	{
		if (distance < 0) return;
		int normalized_angle = (angle % 360 + 360) % 360;
		switch (normalized_angle) {
			case 0:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX - i, posY, pen);
				}
				posX -= distance;
				break;
			case 90:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX, posY + i, pen);
				}
				posY += distance;
				break;
			case 180:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX + i, posY, pen);
				}
				posX += distance;
				break;
			case 270:
				for (int i = 0; i < distance; ++i) {
					canvas.setPixel(posX, posY - i, pen);
				}
				posY -= distance;
				break;
		}
	}
void Turtle::Left(int angle)
{
	angle -= angle;
}
void Turtle::Right(int angle)
{
	angle += angle;
}



