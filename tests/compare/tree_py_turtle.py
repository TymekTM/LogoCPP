# Mirror of tests/performance_test_super_heavy.logo using Python's stdlib
# turtle (Tk). tracer(0) disables animation, so this measures the Tk turtle
# engine at full speed. Usage: python tree_py_turtle.py [depth]
import sys
import time
import turtle

depth = int(sys.argv[1]) if len(sys.argv) > 1 else 15


def krzaczek(x, n):
    if n > 0:
        t.forward(x)
        t.left(45)
        krzaczek(x * 0.75, n - 1)
        t.right(90)
        krzaczek(x * 0.75, n - 1)
        t.left(45)
        t.backward(x)


t = turtle.Turtle()
t.speed(0)
screen = turtle.Screen()
screen.tracer(0)
start = time.perf_counter()
krzaczek(50.0, depth)
elapsed = time.perf_counter() - start
print(f"pyturtle depth={depth}: {elapsed * 1000:.3f} ms")
turtle.bye()
