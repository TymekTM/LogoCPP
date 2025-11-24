# -*- coding: utf-8 -*-
# Szybki test wydajnosci - bez GUI
# Ten test dziala szybciej bo nie wyswietla okna graficznego

import time
import math

# Symulacja turtle bez GUI - tylko obliczenia
class FastTurtle:
    def __init__(self):
        self.x = 0
        self.y = 0
        self.angle = 90  # kat w stopniach
        self.path = [(self.x, self.y)]
    
    def forward(self, distance):
        radians = math.radians(self.angle)
        self.x += distance * math.cos(radians)
        self.y += distance * math.sin(radians)
        self.path.append((self.x, self.y))
    
    def backward(self, distance):
        self.forward(-distance)
    
    def left(self, angle):
        self.angle += angle
    
    def right(self, angle):
        self.angle -= angle

t = FastTurtle()

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

print(f"\nPython (bez GUI) - Czas wykonania: {execution_time:.6f} sekund")
print(f"Liczba punktow w sciezce: {len(t.path)}")

# Zapisz wynik do pliku
with open("python_fast_result.txt", "w") as f:
    f.write(f"Python Fast Performance Test (bez GUI)\n")
    f.write(f"Parametry: krzaczek(50, 8)\n")
    f.write(f"Czas wykonania: {execution_time:.6f} sekund\n")
    f.write(f"Liczba punktow: {len(t.path)}\n")
