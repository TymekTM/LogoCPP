# Performance Test Script - LogoCPP vs Python Turtle
# Test wydajnoœci interpretera LogoCPP vs biblioteki turtle w Pythonie

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "   TEST WYDAJNOŒCI: LogoCPP vs Python Turtle" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Parametry testu:" -ForegroundColor Yellow
Write-Host "  - Funkcja: krzaczek(50, 8)" -ForegroundColor White
Write-Host "  - Plansza: 1000x1000" -ForegroundColor White
Write-Host "  - G³êbokoœæ rekurencji: 8" -ForegroundColor White
Write-Host ""

# SprawdŸ czy istnieje plik wykonywalny LogoCPP
$logoCppExe = ".\x64\Release\LogoCPP.exe"
$logoCppDebug = ".\x64\Debug\LogoCPP.exe"

if (-not (Test-Path $logoCppExe) -and -not (Test-Path $logoCppDebug)) {
    Write-Host "B£¥D: Nie znaleziono pliku wykonywalnego LogoCPP.exe" -ForegroundColor Red
    Write-Host "SprawdŸ œcie¿ki:" -ForegroundColor Yellow
    Write-Host "  - $logoCppExe" -ForegroundColor White
    Write-Host "  - $logoCppDebug" -ForegroundColor White
    Write-Host ""
    Write-Host "Próba zbudowania projektu..." -ForegroundColor Yellow
    
    # Szukaj pliku .vcxproj
    $vcxproj = Get-ChildItem -Recurse -Filter "LogoCPP.vcxproj" | Select-Object -First 1
    
    if ($vcxproj) {
        Write-Host "Znaleziono projekt: $($vcxproj.FullName)" -ForegroundColor Green
        Write-Host "Uruchom najpierw build projektu w Visual Studio (Release mode)" -ForegroundColor Yellow
    }
    
    exit 1
}

# Wybierz wersjê Release jeœli istnieje
$exePath = if (Test-Path $logoCppExe) { $logoCppExe } else { $logoCppDebug }
$buildType = if (Test-Path $logoCppExe) { "Release" } else { "Debug" }

Write-Host "================================================" -ForegroundColor Green
Write-Host "TEST 1: LogoCPP ($buildType)" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green

# Uruchom LogoCPP i zmierz czas
$startTime = Get-Date

& $exePath -i "performance_test.logo" -o "logocpp_output.txt" -s 1000

$endTime = Get-Date
$logoCppTime = ($endTime - $startTime).TotalSeconds

Write-Host ""
if ($LASTEXITCODE -eq 0) {
    Write-Host "? LogoCPP - Czas wykonania: $("{0:F6}" -f $logoCppTime) sekund" -ForegroundColor Green
} else {
    Write-Host "? LogoCPP - B³¹d wykonania" -ForegroundColor Red
    $logoCppTime = -1
}

Write-Host ""
Write-Host "================================================" -ForegroundColor Green
Write-Host "TEST 2: Python Turtle" -ForegroundColor Green
Write-Host "================================================" -ForegroundColor Green

# SprawdŸ czy Python jest zainstalowany
$pythonCmd = Get-Command python -ErrorAction SilentlyContinue

if (-not $pythonCmd) {
    Write-Host "B£¥D: Python nie jest zainstalowany lub nie jest w PATH" -ForegroundColor Red
    Write-Host "Zainstaluj Python z https://www.python.org/" -ForegroundColor Yellow
    exit 1
}

# Uruchom test Python
& python performance_test_turtle.py

Write-Host ""

# Wczytaj wyniki z pliku
if (Test-Path "python_result.txt") {
    $pythonResult = Get-Content "python_result.txt"
    $pythonTimeLine = $pythonResult | Select-String "Czas wykonania:"
    
    if ($pythonTimeLine) {
        $pythonTimeStr = $pythonTimeLine -replace ".*: ([0-9.]+).*", '$1'
        $pythonTime = [double]$pythonTimeStr
        
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "   PODSUMOWANIE WYNIKÓW" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""
        
        Write-Host "LogoCPP ($buildType):  $("{0:F6}" -f $logoCppTime) s" -ForegroundColor Yellow
        Write-Host "Python Turtle:         $("{0:F6}" -f $pythonTime) s" -ForegroundColor Yellow
        Write-Host ""
        
        if ($logoCppTime -gt 0 -and $pythonTime -gt 0) {
            $ratio = $pythonTime / $logoCppTime
            
            if ($logoCppTime -lt $pythonTime) {
                Write-Host "LogoCPP jest SZYBSZY o:" -ForegroundColor Green
                Write-Host "  - $("{0:F2}" -f ($pythonTime - $logoCppTime)) sekund" -ForegroundColor Green
                Write-Host "  - $("{0:F2}" -f $ratio)x razy" -ForegroundColor Green
            } else {
                $ratio = $logoCppTime / $pythonTime
                Write-Host "Python Turtle jest szybszy o:" -ForegroundColor Red
                Write-Host "  - $("{0:F2}" -f ($logoCppTime - $pythonTime)) sekund" -ForegroundColor Red
                Write-Host "  - $("{0:F2}" -f $ratio)x razy" -ForegroundColor Red
            }
        }
        
        Write-Host ""
        Write-Host "Wygenerowane pliki:" -ForegroundColor Cyan
        Write-Host "  - logocpp_output.txt (wynik LogoCPP)" -ForegroundColor White
        Write-Host "  - python_result.txt (wynik Python)" -ForegroundColor White
        if (Test-Path "python_output.eps") {
            Write-Host "  - python_output.eps (obraz Python)" -ForegroundColor White
        }
    }
}

Write-Host ""
Write-Host "Test zakoñczony!" -ForegroundColor Green
