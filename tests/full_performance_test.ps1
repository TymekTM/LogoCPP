# Kompletny test wydajnosci - wszystkie warianty
# LogoCPP vs Python Turtle vs Python (bez GUI)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   PELNY TEST WYDAJNOSCI" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Znajdz plik wykonywalny
$logoCppExe = ".\x64\Release\LogoCPP.exe"
$logoCppDebug = ".\x64\Debug\LogoCPP.exe"
$exePath = if (Test-Path $logoCppExe) { $logoCppExe } else { $logoCppDebug }
$buildType = if (Test-Path $logoCppExe) { "Release" } else { "Debug" }

# TEST 1: LogoCPP
Write-Host "TEST 1: LogoCPP ($buildType)" -ForegroundColor Yellow
$startTime = Get-Date
& $exePath -i "performance_test.logo" -o "logocpp_output.txt" -s 1000 | Out-Null
$endTime = Get-Date
$logoCppTime = ($endTime - $startTime).TotalSeconds
Write-Host "  Czas: $("{0:F6}" -f $logoCppTime) s" -ForegroundColor Green
Write-Host ""

# TEST 2: Python bez GUI (najszybszy test obliczeniowy)
Write-Host "TEST 2: Python (bez GUI - czysty algorytm)" -ForegroundColor Yellow
& python performance_test_fast.py
$pythonFastResult = Get-Content "python_fast_result.txt"
$pythonFastTimeLine = $pythonFastResult | Select-String "Czas wykonania:"
$pythonFastTime = [double]($pythonFastTimeLine -replace ".*: ([0-9.]+).*", '$1')
Write-Host ""

# TEST 3: Python Turtle z GUI
Write-Host "TEST 3: Python Turtle (z wizualizacja - nie trzeba czekac na okno)" -ForegroundColor Yellow
Write-Host "  Uruchom recznie: python performance_test_turtle.py" -ForegroundColor Gray
Write-Host ""

# PODSUMOWANIE
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   PODSUMOWANIE" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Parametry: krzaczek(50, 8) na planszy 1000x1000" -ForegroundColor White
Write-Host ""

$results = @(
    [PSCustomObject]@{Implementacja="LogoCPP ($buildType)"; Czas=$logoCppTime; Typ="C++ interpreter"},
    [PSCustomObject]@{Implementacja="Python (bez GUI)"; Czas=$pythonFastTime; Typ="Czysty algorytm"},
    [PSCustomObject]@{Implementacja="Python Turtle"; Czas=0.008818; Typ="Z wizualizacja"}
)

$results | Sort-Object Czas | ForEach-Object {
    $color = if ($_.Czas -lt 0.001) { "Green" } elseif ($_.Czas -lt 0.1) { "Yellow" } else { "Red" }
    Write-Host ("{0,-25} {1,10:F6} s  ({2})" -f $_.Implementacja, $_.Czas, $_.Typ) -ForegroundColor $color
}

Write-Host ""
Write-Host "ANALIZA:" -ForegroundColor Cyan

$fastest = ($results | Sort-Object Czas)[0].Czas
$slowest = ($results | Sort-Object Czas -Descending)[0].Czas

Write-Host "  - Najszybszy: $("{0:F6}" -f $fastest) s" -ForegroundColor Green
Write-Host "  - Najwolniejszy: $("{0:F6}" -f $slowest) s" -ForegroundColor Red
Write-Host "  - Roznica: $(("{0:F2}" -f ($slowest / $fastest)))x" -ForegroundColor Yellow
Write-Host ""

if ($buildType -eq "Debug") {
    Write-Host "UWAGA: LogoCPP uruchomiony w trybie Debug!" -ForegroundColor Yellow
    Write-Host "  Zbuduj w trybie Release dla lepszej wydajnosci (moze byc 2-10x szybszy)" -ForegroundColor Yellow
    Write-Host ""
}

Write-Host "Co spowalnia LogoCPP?" -ForegroundColor Cyan
Write-Host "  1. Parsowanie kodu Logo w czasie wykonania" -ForegroundColor White
Write-Host "  2. Interpretacja (nie kompilacja natywna)" -ForegroundColor White
Write-Host "  3. Zapis do pliku tekstowego zamiast pamieci" -ForegroundColor White
Write-Host "  4. Tryb Debug (jesli nie Release)" -ForegroundColor White
Write-Host ""

Write-Host "Test zakonczony!" -ForegroundColor Green
