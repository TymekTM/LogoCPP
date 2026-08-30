# Pure CPython port of tests/performance_test_super_heavy.logo: same
# char-grid model, integer trig tables (x8192), round-half distance, and
# Bresenham lines as LogoCore. No turtle/Tk - this isolates the cost of the
# language itself. Usage: python tree_py_grid.py [depth] [runs]
import math
import sys
import time

depth = int(sys.argv[1]) if len(sys.argv) > 1 else 15
RUNS = int(sys.argv[2]) if len(sys.argv) > 2 else 3

W = H = 120
grid = [[' '] * W for _ in range(H)]
pos = [W // 2, H // 2]
angle = 0
COS = [round(8192 * math.cos(a * math.pi / 180)) for a in range(360)]
SIN = [round(8192 * math.sin(a * math.pi / 180)) for a in range(360)]


def move(d, a):
    if d <= 0:
        return
    dx = d * COS[a]
    dy = d * SIN[a]
    nx = pos[0] + (dx + 4096 + (dx >> 31)) >> 13
    ny = pos[1] + (dy + 4096 + (dy >> 31)) >> 13
    x0, y0 = pos
    dx = abs(nx - x0)
    dy = -abs(ny - y0)
    sx = 1 if x0 < nx else -1
    sy = 1 if y0 < ny else -1
    err = dx + dy
    while True:
        if 0 <= y0 < H and 0 <= x0 < W:
            grid[y0][x0] = '*'
        if x0 == nx and y0 == ny:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy
    pos[0] = nx
    pos[1] = ny


def krzaczek(x, n):
    global angle
    if n > 0:
        move(int(x + 0.5), angle)
        angle = (angle - 45) % 360
        krzaczek(x * 0.75, n - 1)
        angle = (angle + 90) % 360
        krzaczek(x * 0.75, n - 1)
        angle = (angle - 45) % 360
        move(int(x + 0.5), (angle + 180) % 360)


def one_run():
    start = time.perf_counter()
    krzaczek(50.0, depth)
    return time.perf_counter() - start


one_run()  # warmup; grid writes are idempotent, no reset needed
best = min(one_run() for _ in range(RUNS))
print(f"pygrid depth={depth}: {best * 1000:.3f} ms")
