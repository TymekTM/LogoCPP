# -*- coding: utf-8 -*-
# Test wydajnosci - Python Turtle bez okna (headless mode)
import time

# Symulacja turtle - kompatybilna z API
class HeadlessTurtle:
    def __init__(self):
        self.x = 0.0
        self.y = 0.0
        self.angle = 90.0
        self.pen_down = True
        self.operations = 0
        
    def forward(self, distance):
        import math
        if self.pen_down:
            radians = math.radians(self.angle)
            self.x += distance * math.cos(radians)
            self.y += distance * math.sin(radians)
        self.operations += 1
    
    def backward(self, distance):
        self.forward(-distance)
    
    def left(self, angle):
        self.angle += angle
        self.operations += 1
    
    def right(self, angle):
        self.angle -= angle
        self.operations += 1

t = HeadlessTurtle()

def krzaczek(x, n):
    if n > 0:
        t.forward(x)
        t.left(45)
        krzaczek(x * 0.75, n - 1)
        
        t.right(90)
        krzaczek(x * 0.75, n - 1)
        
        t.left(45)
        t.backward(x)

# Pomiar czasu
start_time = time.perf_counter()

krzaczek(50, 8)

end_time = time.perf_counter()
execution_time = end_time - start_time

print(f"\nPython Headless - Czas wykonania: {execution_time:.6f} sekund")
print(f"Liczba operacji: {t.operations}")

with open("python_headless_result.txt", "w") as f:
    f.write(f"Python Headless Performance Test\n")
    f.write(f"Parametry: krzaczek(50, 8)\n")
    f.write(f"Czas wykonania: {execution_time:.6f} sekund\n")
    f.write(f"Liczba operacji: {t.operations}\n")
