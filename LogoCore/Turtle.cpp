#include "pch.h"
#include "Turtle.h"
#include "InstructionHandler.h"
#include "Canvas.h"
#include "IsInbound.h"

Turtle::Turtle(Canvas& canvas, char pen)
    :canvas(canvas), pen(pen)
{

	// Œrodek p³ótna, pozycja startowa ¿ó³wia
    posX = canvas.width / 2;
    posY = canvas.height / 2;
}

	void Turtle::Forward(int distance)
	{
		if (distance <= 0) return;
		int normalized_angle = (angle % 360 + 360) % 360;
		switch (normalized_angle) {
		case 0:
				if (IsInbound(posX + distance, posY, canvas.width, canvas.height)) {
					for (int i = 0; i < distance; ++i) {
						canvas.setPixel(posX + i, posY, pen);
					}
					posX += distance;
					break;
				}
				else{
					// TODO: Handle out of bounds
					break;
				}
		case 90:
				if (IsInbound(posX, posY - distance, canvas.width, canvas.height)) {
					for (int i = 0; i < distance; ++i) {
						canvas.setPixel(posX, posY - i, pen);
					}
					posY -= distance;
					break;
				}
				else {
					// TODO: Handle out of bounds
					break;
				}
		case 180:
				if (IsInbound(posX - distance, posY, canvas.width, canvas.height)) {
					for (int i = 0; i < distance; ++i) {
						canvas.setPixel(posX - i, posY, pen);
					}
					posX -= distance;
					break;
				}
				else {
					// TODO: Handle out of bounds
					break;
				}
		case 270:
				if (IsInbound(posX, posY + distance, canvas.width, canvas.height)) {
					for (int i = 0; i < distance; ++i) {
						canvas.setPixel(posX, posY + i, pen);
					}
					posY += distance;
					break;
				}
				else {
					// TODO: Handle out of bounds
					break;
				}
			}
		}
	void Turtle::Backward(int distance)
	{
		if (distance <= 0) return;
		int normalized_angle = (angle % 360 + 360) % 360;
		switch (normalized_angle) {
			case 0:
					if (IsInbound(posX - distance, posY, canvas.width, canvas.height)) {
						for (int i = 0; i < distance; ++i) {
							canvas.setPixel(posX - i, posY, pen);
						}
						posX -= distance;
						break;
					}
					else {
						// TODO: Handle out of bounds
						break;
					}
			case 90:
					if (IsInbound(posX, posY + distance, canvas.width, canvas.height)) {
						for (int i = 0; i < distance; ++i) {
							canvas.setPixel(posX, posY + i, pen);
						}
						posY += distance;
						break;
					}
					else {
						// TODO: Handle out of bounds
						break;
					}
			case 180:
					if (IsInbound(posX + distance, posY, canvas.width, canvas.height)) {
						for (int i = 0; i < distance; ++i) {
							canvas.setPixel(posX + i, posY, pen);
						}
						posX += distance;
						break;
					}
					else {
						// TODO: Handle out of bounds
						break;
					}
			case 270:
					if (IsInbound(posX, posY - distance, canvas.width, canvas.height)) {
						for (int i = 0; i < distance; ++i) {
							canvas.setPixel(posX, posY - i, pen);
						}
						posY -= distance;
						break;
					}
					else {
						// TODO: Handle out of bounds
						break;
					}
		}
	}
	void Turtle::Left(int angle)
	{
		this->angle -= angle;
	}
	void Turtle::Right(int angle)
	{
		this->angle += angle;
	}



