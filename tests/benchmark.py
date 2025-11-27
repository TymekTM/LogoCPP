#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Ustandaryzowany benchmark porownawczy LogoCPP vs Python turtle
Testuje wydajnosc parsowania i wykonania rekurencyjnej funkcji fraktalnej
"""

import time
import turtle
import subprocess
import os
import sys

# Konfiguracja testu
ITERATIONS = 1000
DEPTH_STANDARD = 8
DEPTH_HEAVY = 12
DEPTH_SUPER_HEAVY = 15
CANVAS_SIZE = 500


def krzaczek(t, x, n):
    """Rekurencyjna funkcja fraktalna - identyczna z wersja Logo"""
    if n > 0:
        t.forward(x)
        t.left(45)
        krzaczek(t, x * 0.75, n - 1)
        
        t.right(90)
        krzaczek(t, x * 0.75, n - 1)
        
        t.left(45)
        t.backward(x)


def benchmark_python(iterations, depth):
    """Benchmark biblioteki turtle w Pythonie"""
    # Reset turtle dla nowego testu
    turtle.clearscreen()
    screen = turtle.Screen()
    screen.setup(CANVAS_SIZE, CANVAS_SIZE)
    screen.tracer(0, 0)  # Wylacz animacje
    t = turtle.Turtle()
    t.speed(0)
    t.hideturtle()
    
    # Warmup
    for _ in range(3):
        t.clear()
        t.penup()
        t.goto(0, 0)
        t.pendown()
        krzaczek(t, 50, depth)
        screen.update()
    
    # Pomiar
    start = time.perf_counter()
    for _ in range(iterations):
        t.clear()
        t.penup()
        t.goto(0, 0)
        t.pendown()
        krzaczek(t, 50, depth)
        screen.update()
    end = time.perf_counter()
    
    total_ms = (end - start) * 1000
    avg_ms = total_ms / iterations
    
    return total_ms, avg_ms


def cleanup_turtle():
    """Zamknij okno turtle na koniec"""
    try:
        turtle.bye()
    except:
        pass


def benchmark_logocpp(iterations, logo_file):
    """Benchmark LogoCPP (wymaga skompilowanego exe)"""
    exe_path = os.path.join(os.path.dirname(__file__), '..', 'x64', 'Release', 'LogoCPP.exe')
    
    if not os.path.exists(exe_path):
        return None, None, "LogoCPP.exe not found"
    
    total_ms = 0.0
    avg_ms = 0.0
    
    try:
        result = subprocess.run(
            [exe_path, '-i', logo_file, '-o', 'benchmark_output.txt', '-s', str(CANVAS_SIZE), '-b', str(iterations)],
            capture_output=True,
            text=True,
            cwd=os.path.dirname(__file__)
        )
        
        # Parsuj output
        for line in result.stdout.split('\n'):
            if 'Average per iteration' in line:
                avg_ms = float(line.split(':')[1].strip().replace(' ms', ''))
            if 'Total time' in line:
                total_ms = float(line.split(':')[1].strip().replace(' ms', ''))
        
        return total_ms, avg_ms, None
    except Exception as e:
        return None, None, str(e)


def print_separator():
    print("=" * 60)


def main():
    print_separator()
    print("       BENCHMARK POROWNAWCZY: LogoCPP vs Python")
    print_separator()
    print(f"\nKonfiguracja:")
    print(f"  - Iteracje: {ITERATIONS}")
    print(f"  - Canvas: {CANVAS_SIZE}x{CANVAS_SIZE}")
    print(f"  - Test standardowy: krzaczek(50, {DEPTH_STANDARD})")
    print(f"  - Test ciezki: krzaczek(50, {DEPTH_HEAVY})")
    print(f"  - Test super ciezki: krzaczek(50, {DEPTH_SUPER_HEAVY})")
    
    # Test standardowy (n=8)
    print(f"\n{'='*60}")
    print(f"  TEST STANDARDOWY (n={DEPTH_STANDARD})")
    print(f"{'='*60}")
    
    print("\n[Python turtle]")
    py_total, py_avg = benchmark_python(ITERATIONS, DEPTH_STANDARD)
    print(f"  Czas calkowity: {py_total:.2f} ms")
    print(f"  Srednia/iteracje: {py_avg:.4f} ms")
    
    print("\n[LogoCPP]")
    logo_file = os.path.join(os.path.dirname(__file__), 'performance_test.logo')
    cpp_total, cpp_avg, cpp_err = benchmark_logocpp(ITERATIONS, logo_file)
    
    if cpp_err:
        print(f"  Blad: {cpp_err}")
    else:
        print(f"  Czas calkowity: {cpp_total:.2f} ms")
        print(f"  Srednia/iteracje: {cpp_avg:.4f} ms")
        
        # Porownanie
        if py_avg > 0 and cpp_avg > 0:
            ratio = py_avg / cpp_avg
            faster = "LogoCPP" if ratio > 1 else "Python"
            ratio = ratio if ratio > 1 else 1/ratio
            print(f"\n  >>> {faster} jest {ratio:.2f}x szybszy <<<")
    
    # Test ciezki (n=12)
    print(f"\n{'='*60}")
    print(f"  TEST CIEZKI (n={DEPTH_HEAVY})")
    print(f"{'='*60}")
    
    heavy_iterations = 100  # Mniej iteracji dla ciezkiego testu
    
    print("\n[Python turtle]")
    py_total_h, py_avg_h = benchmark_python(heavy_iterations, DEPTH_HEAVY)
    print(f"  Czas calkowity: {py_total_h:.2f} ms ({heavy_iterations} iteracji)")
    print(f"  Srednia/iteracje: {py_avg_h:.4f} ms")
    
    print("\n[LogoCPP]")
    logo_file_heavy = os.path.join(os.path.dirname(__file__), 'performance_test_heavy.logo')
    cpp_total_h, cpp_avg_h, cpp_err_h = benchmark_logocpp(heavy_iterations, logo_file_heavy)
    
    if cpp_err_h:
        print(f"  Blad: {cpp_err_h}")
    else:
        print(f"  Czas calkowity: {cpp_total_h:.2f} ms ({heavy_iterations} iteracji)")
        print(f"  Srednia/iteracje: {cpp_avg_h:.4f} ms")
        
        if py_avg_h > 0 and cpp_avg_h > 0:
            ratio_h = py_avg_h / cpp_avg_h
            faster_h = "LogoCPP" if ratio_h > 1 else "Python"
            ratio_h = ratio_h if ratio_h > 1 else 1/ratio_h
            print(f"\n  >>> {faster_h} jest {ratio_h:.2f}x szybszy <<<")
    
    # Test super ciezki (n=15)
    print(f"\n{'='*60}")
    print(f"  TEST SUPER CIEZKI (n={DEPTH_SUPER_HEAVY})")
    print(f"{'='*60}")
    
    super_heavy_iterations = 10  # Jeszcze mniej iteracji dla super ciezkiego testu
    
    print("\n[Python turtle]")
    py_total_sh, py_avg_sh = benchmark_python(super_heavy_iterations, DEPTH_SUPER_HEAVY)
    print(f"  Czas calkowity: {py_total_sh:.2f} ms ({super_heavy_iterations} iteracji)")
    print(f"  Srednia/iteracje: {py_avg_sh:.4f} ms")
    
    print("\n[LogoCPP]")
    logo_file_super_heavy = os.path.join(os.path.dirname(__file__), 'performance_test_super_heavy.logo')
    cpp_total_sh, cpp_avg_sh, cpp_err_sh = benchmark_logocpp(super_heavy_iterations, logo_file_super_heavy)
    
    if cpp_err_sh:
        print(f"  Blad: {cpp_err_sh}")
    else:
        print(f"  Czas calkowity: {cpp_total_sh:.2f} ms ({super_heavy_iterations} iteracji)")
        print(f"  Srednia/iteracje: {cpp_avg_sh:.4f} ms")
        
        if py_avg_sh > 0 and cpp_avg_sh > 0:
            ratio_sh = py_avg_sh / cpp_avg_sh
            faster_sh = "LogoCPP" if ratio_sh > 1 else "Python"
            ratio_sh = ratio_sh if ratio_sh > 1 else 1/ratio_sh
            print(f"\n  >>> {faster_sh} jest {ratio_sh:.2f}x szybszy <<<")
    
    print_separator()
    print("\nBenchmark zakonczony.")
    cleanup_turtle()


if __name__ == "__main__":
    main()
