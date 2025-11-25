#include "pch.h"
#include "Turtle.h"
#include "InstructionHandler.h"
#include "Canvas.h"
#include <iostream>
#include <cmath>
#include <numbers>

Turtle::Turtle(Canvas& canvas, char pen)
    :canvas(canvas), pen(pen), offsetX(0), offsetY(0)
{
	// Środek płótna, pozycja startowa żółwia
    posX = canvas.getInitialWidth() / 2;
    posY = canvas.getInitialHeight() / 2;
}

void Turtle::Forward(int distance)
{
	if (distance <= 0) return;
	int normalized_angle = (angle % 360 + 360) % 360;

	// Oblicz wektor kierunku na podstawie kąta
	// Kąt 0° = prawo (1, 0)
	// Kąt 90° = dół (0, 1) - w układzie ekranowym Y rośnie w dół
	// Kąt 180° = lewo (-1, 0)
	// Kąt 270° = góra (0, -1)
	
	double radians = normalized_angle * std::numbers::pi / 180.0;
	double dx = cos(radians);
	double dy = sin(radians);

	for (int i = 0; i <= distance; ++i) {
		int newX = posX + static_cast<int>(round(i * dx));
		int newY = posY + static_cast<int>(round(i * dy));
		
		canvas.setPixel(newX, newY, pen);
		canvas.setPixel(newX, newY, pen);
	}
	
	posX = posX + static_cast<int>(round(distance * dx));
	posY = posY + static_cast<int>(round(distance * dy));
}

void Turtle::Backward(int distance)
{
	if (distance <= 0) return;
	
	int normalized_angle = (angle % 360 + 360) % 360;

	double radians = (normalized_angle + 180) * std::numbers::pi / 180.0;
	double dx = cos(radians);
	double dy = sin(radians);

	for (int i = 0; i <= distance; ++i) {
		int newX = posX + static_cast<int>(round(i * dx));
		int newY = posY + static_cast<int>(round(i * dy));
		
		canvas.setPixel(newX, newY, pen);
	}
	
	posX = posX + static_cast<int>(round(distance * dx));
	posY = posY + static_cast<int>(round(distance * dy));
}

void Turtle::Left(int angle)
{
	this->angle -= angle;
}

void Turtle::Right(int angle)
{
	this->angle += angle;
}



