# -*- coding: utf-8 -*-
import turtle
import time

# Konfiguracja - plansza 1000x1000
screen = turtle.Screen()
screen.setup(1000, 1000)
screen.title("Performance Test - Python Turtle")
screen.tracer(0)  # Wylacz animacje dla lepszej wydajnosci

t = turtle.Turtle()
t.speed(0)
t.hideturtle()

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

# Wyswietl wynik
screen.update()
print(f"Python Turtle - Czas wykonania: {execution_time:.6f} sekund")

# Zapisz wynik do pliku
with open("python_result.txt", "w") as f:
    f.write(f"Python Turtle Performance Test\n")
    f.write(f"Parametry: krzaczek(50, 8) na planszy 1000x1000\n")
    f.write(f"Czas wykonania: {execution_time:.6f} sekund\n")

# Zapisz obraz
try:
    screen.getcanvas().postscript(file="python_output.eps")
    print("Wynik zapisany do: python_output.eps")
except:
    pass

print("Kliknij w okno aby zamknac...")
screen.exitonclick()
