#include "pch.h"
#include "Turtle.h"
#include "InstructionHandler.h"
#include "Canvas.h"
#include "IsInbound.h"
#include <iostream>
#include <cmath>
#include <numbers>

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


		// Oblicz wektor kierunku na podstawie k¹ta
		// K¹t 0° = prawo (1, 0)
		// K¹t 90° = dó³ (0, 1) - w uk³adzie ekranowym Y roœnie w dó³
		// K¹t 180° = lewo (-1, 0)
		// K¹t 270° = góra (0, -1)
		
		double radians = normalized_angle * std::numbers::pi / 180.0;
		double dx = cos(radians);
		double dy = sin(radians);


		for (int i = 0; i <= distance; ++i) {
			int newX = posX + static_cast<int>(round(i * dx));
			int newY = posY + static_cast<int>(round(i * dy));
			
			if (IsInbound(newX, newY, canvas.width, canvas.height)) {
				canvas.setPixel(newX, newY, pen);
			}

		}
		
		posX = posX + static_cast<int>(round(distance * dx));
		posY = posY + static_cast<int>(round(distance * dy));
		
		if (posX < 0) posX = 0;
		if (posX >= canvas.width) posX = canvas.width - 1;
		if (posY < 0) posY = 0;
		if (posY >= canvas.height) posY = canvas.height - 1;
		

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
			
			if (IsInbound(newX, newY, canvas.width, canvas.height)) {
				canvas.setPixel(newX, newY, pen);
			}

		}
		
		posX = posX + static_cast<int>(round(distance * dx));
		posY = posY + static_cast<int>(round(distance * dy));
		
		if (posX < 0) posX = 0;
		if (posX >= canvas.width) posX = canvas.width - 1;
		if (posY < 0) posY = 0;
		if (posY >= canvas.height) posY = canvas.height - 1;

	}
	
	void Turtle::Left(int angle)
	{
		this->angle -= angle;
	}
	void Turtle::Right(int angle)
	{
		this->angle += angle;
	}



