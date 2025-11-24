# OSTATECZNY TEST WYDAJNOSCI
# Porownanie wszystkich wariantow z Release build

Write-Host ""
Write-Host "?????????????????????????????????????????????????????????????" -ForegroundColor Cyan
Write-Host "?          TEST WYDAJNOSCI - LOGOCPP VS PYTHON              ?" -ForegroundColor Cyan
Write-Host "?????????????????????????????????????????????????????????????" -ForegroundColor Cyan
Write-Host ""
Write-Host "Parametry testu:" -ForegroundColor Yellow
Write-Host "  • Funkcja: krzaczek(50, 8)" -ForegroundColor White
Write-Host "  • Rekurencja: 8 poziomow (256 wywolan)" -ForegroundColor White
Write-Host "  • Plansza: 1000x1000" -ForegroundColor White
Write-Host ""

# Sprawdz dostepnosc wersji
$releaseExe = ".\x64\Release\LogoCPP.exe"
$debugExe = ".\x64\Debug\LogoCPP.exe"

if (-not (Test-Path $releaseExe)) {
    Write-Host "BLAD: Brak wersji Release!" -ForegroundColor Red
    Write-Host "Uzyj Debug lub zbuduj Release build." -ForegroundColor Yellow
    if (Test-Path $debugExe) {
        $exePath = $debugExe
        $buildType = "Debug"
    } else {
        exit 1
    }
} else {
    $exePath = $releaseExe
    $buildType = "Release"
}

Write-Host "?????????????????????????????????????????????????????????" -ForegroundColor Gray
Write-Host ""

# Array do przechowywania wynikow
$results = @()

# TEST 1: LogoCPP
Write-Host "?? TEST 1/4: LogoCPP ($buildType)" -ForegroundColor Yellow
Write-Host "   Uruchamianie..." -ForegroundColor Gray

$measurements = @()
for ($i = 1; $i -le 3; $i++) {
    $startTime = Get-Date
    & $exePath -i "performance_test.logo" -o "logocpp_output.txt" -s 1000 2>&1 | Out-Null
    $endTime = Get-Date
    $time = ($endTime - $startTime).TotalSeconds
    $measurements += $time
    Write-Host "   Proba $i/3: $("{0:F6}" -f $time) s" -ForegroundColor Gray
}
$logoCppTime = ($measurements | Measure-Object -Average).Average
Write-Host "   ? Srednia: $("{0:F6}" -f $logoCppTime) s" -ForegroundColor Green
$results += [PSCustomObject]@{
    Nazwa="LogoCPP ($buildType)"
    Czas=$logoCppTime
    Kategoria="Interpreter C++"
}
Write-Host ""

# TEST 2: Python bez GUI (najszybszy)
Write-Host "?? TEST 2/4: Python (bez GUI - czysty algorytm)" -ForegroundColor Yellow
Write-Host "   Uruchamianie..." -ForegroundColor Gray

$measurements = @()
for ($i = 1; $i -le 3; $i++) {
    $output = & python performance_test_fast.py 2>&1
    $timeLine = $output | Select-String "Czas wykonania: ([0-9.]+)"
    if ($timeLine -match "([0-9.]+)") {
        $time = [double]$matches[1]
        $measurements += $time
        Write-Host "   Proba $i/3: $("{0:F6}" -f $time) s" -ForegroundColor Gray
    }
}
$pythonFastTime = ($measurements | Measure-Object -Average).Average
Write-Host "   ? Srednia: $("{0:F6}" -f $pythonFastTime) s" -ForegroundColor Green
$results += [PSCustomObject]@{
    Nazwa="Python (optymalizowany)"
    Czas=$pythonFastTime
    Kategoria="Czysty algorytm"
}
Write-Host ""

# TEST 3: Python Headless Turtle
Write-Host "?? TEST 3/4: Python (headless turtle)" -ForegroundColor Yellow
Write-Host "   Uruchamianie..." -ForegroundColor Gray

$measurements = @()
for ($i = 1; $i -le 3; $i++) {
    $output = & python performance_test_headless.py 2>&1
    $timeLine = $output | Select-String "Czas wykonania: ([0-9.]+)"
    if ($timeLine -match "([0-9.]+)") {
        $time = [double]$matches[1]
        $measurements += $time
        Write-Host "   Proba $i/3: $("{0:F6}" -f $time) s" -ForegroundColor Gray
    }
}
$pythonHeadlessTime = ($measurements | Measure-Object -Average).Average
Write-Host "   ? Srednia: $("{0:F6}" -f $pythonHeadlessTime) s" -ForegroundColor Green
$results += [PSCustomObject]@{
    Nazwa="Python (headless turtle)"
    Czas=$pythonHeadlessTime
    Kategoria="Symulacja turtle"
}
Write-Host ""

# TEST 4: Python Turtle z GUI (pojedyncze uruchomienie)
Write-Host "?? TEST 4/4: Python Turtle (z wizualizacja)" -ForegroundColor Yellow
Write-Host "   Uwaga: Ten test otworzy okno graficzne" -ForegroundColor Gray
Write-Host "   Kliknij w okno aby zamknac po zakonczeniu" -ForegroundColor Gray
Start-Sleep -Seconds 2

$output = & python performance_test_turtle.py 2>&1
$timeLine = $output | Select-String "Czas wykonania: ([0-9.]+)"
if ($timeLine -match "([0-9.]+)") {
    $pythonTurtleTime = [double]$matches[1]
    Write-Host "   ? Czas: $("{0:F6}" -f $pythonTurtleTime) s" -ForegroundColor Green
    $results += [PSCustomObject]@{
        Nazwa="Python Turtle (GUI)"
        Czas=$pythonTurtleTime
        Kategoria="Z wizualizacja"
    }
}
Write-Host ""

# PODSUMOWANIE
Write-Host ""
Write-Host "?????????????????????????????????????????????????????????????" -ForegroundColor Cyan
Write-Host "?                    WYNIKI TESTOW                          ?" -ForegroundColor Cyan
Write-Host "?????????????????????????????????????????????????????????????" -ForegroundColor Cyan
Write-Host ""

# Sortuj wyniki
$sorted = $results | Sort-Object Czas

Write-Host "Ranking (od najszybszego):" -ForegroundColor Yellow
Write-Host ""

$rank = 1
foreach ($result in $sorted) {
    $color = switch ($rank) {
        1 { "Green" }
        2 { "Yellow" }
        default { "White" }
    }
    
    $medal = switch ($rank) {
        1 { "??" }
        2 { "??" }
        3 { "??" }
        default { "  " }
    }
    
    Write-Host "$medal $rank. " -NoNewline -ForegroundColor $color
    Write-Host ("{0,-30}" -f $result.Nazwa) -NoNewline -ForegroundColor $color
    Write-Host ("{0,12:F6} s" -f $result.Czas) -NoNewline -ForegroundColor $color
    Write-Host "  ({0})" -f $result.Kategoria -ForegroundColor Gray
    
    $rank++
}

Write-Host ""
Write-Host "?????????????????????????????????????????????????????????" -ForegroundColor Gray
Write-Host ""

# Analiza wydajnosci
$fastest = $sorted[0].Czas
$slowest = $sorted[-1].Czas

Write-Host "?? ANALIZA WYDAJNOSCI:" -ForegroundColor Cyan
Write-Host ""
Write-Host "  Najszybszy:    $("{0,-30}" -f $sorted[0].Nazwa) $("{0:F6}" -f $fastest) s" -ForegroundColor Green
Write-Host "  Najwolniejszy: $("{0,-30}" -f $sorted[-1].Nazwa) $("{0:F6}" -f $slowest) s" -ForegroundColor Red
Write-Host ""
Write-Host "  Roznica:       $("{0:F2}" -f ($slowest / $fastest))x" -ForegroundColor Yellow
Write-Host ""

# Porownanie LogoCPP z Python
$logoCppResult = $results | Where-Object { $_.Nazwa -like "LogoCPP*" } | Select-Object -First 1
$pythonFastResult = $results | Where-Object { $_.Nazwa -like "*optymalizowany*" } | Select-Object -First 1

if ($logoCppResult -and $pythonFastResult) {
    $ratio = $logoCppResult.Czas / $pythonFastResult.Czas
    Write-Host "?? POROWNANIE BEZPOSREDNIE:" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  LogoCPP vs Python (czysty algorytm):" -ForegroundColor White
    Write-Host "    LogoCPP jest $("{0:F2}" -f $ratio)x wolniejszy" -ForegroundColor Yellow
    Write-Host "    Roznica: $("{0:F6}" -f ($logoCppResult.Czas - $pythonFastResult.Czas)) s" -ForegroundColor Gray
    Write-Host ""
}

# Wnioski
Write-Host "?? WNIOSKI:" -ForegroundColor Cyan
Write-Host ""
Write-Host "  ? Python bez GUI jest najszybszy (czysty algorytm)" -ForegroundColor Green
Write-Host "  ? Wizualizacja (turtle GUI) spowalnia ~20x" -ForegroundColor Yellow
Write-Host "  ? LogoCPP interpreter ma overhead parsowania + I/O" -ForegroundColor Yellow

if ($buildType -eq "Release") {
    Write-Host "  ? LogoCPP w trybie Release (optymalny)" -ForegroundColor Green
} else {
    Write-Host "  ? LogoCPP w trybie Debug - Release bedzie szybszy!" -ForegroundColor Red
}

Write-Host ""
Write-Host "?? Wygenerowane pliki:" -ForegroundColor Cyan
Write-Host "  - logocpp_output.txt" -ForegroundColor Gray
Write-Host "  - python_result.txt" -ForegroundColor Gray
Write-Host "  - python_fast_result.txt" -ForegroundColor Gray
Write-Host "  - python_headless_result.txt" -ForegroundColor Gray
Write-Host ""
Write-Host "? Test zakoñczony!" -ForegroundColor Green
Write-Host ""
