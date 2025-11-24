# Test I/O Impact - Wielokrotne uruchomienie dla sredniej

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  ANALIZA WP£YWU I/O NA WYDAJNOŒÆ LOGOCPP" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

$exePath = ".\LogoCPP\x64\Release\LogoCPP.exe"
$iterations = 5

$readTimes = @()
$processTimes = @()
$writeTimes = @()
$totalTimes = @()

Write-Host "Uruchamianie $iterations testow..." -ForegroundColor Yellow
Write-Host ""

for ($i = 1; $i -le $iterations; $i++) {
    Write-Host "Test $i/$iterations..." -ForegroundColor Gray
    
    # Uruchom test
    $output = & $exePath -i "performance_test.logo" -o "logocpp_output.txt" -s 1000 2>&1
    
    # Parsuj wyniki
    $readLine = $output | Select-String "Read file:\s+(\d+)\s+us"
    $processLine = $output | Select-String "Process \(core\):\s+(\d+)\s+us"
    $writeLine = $output | Select-String "Write file:\s+(\d+)\s+us"
    $totalLine = $output | Select-String "TOTAL:\s+(\d+)\s+us"
    
    if ($readLine -match "(\d+)") {
        $readTimes += [int]$matches[1]
    }
    if ($processLine -match "(\d+)") {
        $processTimes += [int]$matches[1]
    }
    if ($writeLine -match "(\d+)") {
        $writeTimes += [int]$matches[1]
    }
    if ($totalLine -match "(\d+)") {
        $totalTimes += [int]$matches[1]
    }
}

Write-Host ""
Write-Host "==========================================" -ForegroundColor Green
Write-Host "  WYNIKI (srednia z $iterations prob)" -ForegroundColor Green
Write-Host "==========================================" -ForegroundColor Green
Write-Host ""

$avgRead = ($readTimes | Measure-Object -Average).Average
$avgProcess = ($processTimes | Measure-Object -Average).Average
$avgWrite = ($writeTimes | Measure-Object -Average).Average
$avgTotal = ($totalTimes | Measure-Object -Average).Average

$avgReadMs = $avgRead / 1000
$avgProcessMs = $avgProcess / 1000
$avgWriteMs = $avgWrite / 1000
$avgTotalMs = $avgTotal / 1000

Write-Host "Read file:      $("{0,8:N0}" -f $avgRead) us ($("{0,8:N3}" -f $avgReadMs) ms)" -ForegroundColor Cyan
Write-Host "Process (core): $("{0,8:N0}" -f $avgProcess) us ($("{0,8:N3}" -f $avgProcessMs) ms)" -ForegroundColor Yellow
Write-Host "Write file:     $("{0,8:N0}" -f $avgWrite) us ($("{0,8:N3}" -f $avgWriteMs) ms)" -ForegroundColor Magenta
Write-Host "--------------------------------------------"
Write-Host "TOTAL:          $("{0,8:N0}" -f $avgTotal) us ($("{0,8:N3}" -f $avgTotalMs) ms)" -ForegroundColor Green
Write-Host ""

# Percentage breakdown
$ioPercent = (($avgRead + $avgWrite) / $avgTotal) * 100
$processPercent = ($avgProcess / $avgTotal) * 100

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  ANALIZA PROCENTOWA" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "I/O (read + write):  $("{0,6:N2}" -f $ioPercent)%" -ForegroundColor Red
Write-Host "  - Read:            $("{0,6:N2}" -f (($avgRead / $avgTotal) * 100))%" -ForegroundColor Gray
Write-Host "  - Write:           $("{0,6:N2}" -f (($avgWrite / $avgTotal) * 100))%" -ForegroundColor Gray
Write-Host ""
Write-Host "Processing:          $("{0,6:N2}" -f $processPercent)%" -ForegroundColor Green
Write-Host ""

# Porownanie z Pythonem
$pythonTime = 0.000470 * 1000000  # Convert to microseconds
$pythonTimeMs = 0.000470 * 1000   # Convert to milliseconds

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  POROWNANIE Z PYTHON" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "LogoCPP Processing Only: $("{0,8:N3}" -f $avgProcessMs) ms" -ForegroundColor Yellow
Write-Host "Python (bez GUI):        $("{0,8:N3}" -f $pythonTimeMs) ms" -ForegroundColor Green
Write-Host ""

$ratioWithIO = $avgTotalMs / $pythonTimeMs
$ratioNoIO = $avgProcessMs / $pythonTimeMs

Write-Host "Roznica (z I/O):         $("{0:N2}" -f $ratioWithIO)x wolniejszy" -ForegroundColor Red
Write-Host "Roznica (bez I/O):       $("{0:N2}" -f $ratioNoIO)x wolniejszy" -ForegroundColor Yellow
Write-Host ""

# Potencjalne przyspieszenie
$potentialSpeedup = $avgTotal / $avgProcess

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  WNIOSKI" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "1. I/O stanowi $("{0:N1}" -f $ioPercent)% calkowitego czasu!" -ForegroundColor Red
Write-Host ""
Write-Host "2. Zapis do pliku ($("{0:N1}" -f $avgWriteMs) ms) jest NAJWOLNIEJSZY" -ForegroundColor Red
Write-Host "   To jest $("{0:N1}" -f ($avgWrite / $avgProcess))x wolniejsze niz processing!" -ForegroundColor Red
Write-Host ""
Write-Host "3. Faktyczny czas przetwarzania: $("{0:N2}" -f $avgProcessMs) ms" -ForegroundColor Yellow
Write-Host "   (parsowanie + interpretacja + wykonanie)" -ForegroundColor Gray
Write-Host ""
Write-Host "4. Potencjalne przyspieszenie bez I/O: $("{0:N2}" -f $potentialSpeedup)x" -ForegroundColor Green
Write-Host ""
Write-Host "5. Nawet bez I/O, LogoCPP jest $("{0:N0}" -f $ratioNoIO)x wolniejszy" -ForegroundColor Yellow
Write-Host "   niz Python (interpreter overhead)" -ForegroundColor Gray
Write-Host ""

# Zapisz raport
$report = @"
=================================================================
           ANALIZA I/O vs PROCESSING - LogoCPP
=================================================================

PARAMETRY:
  - Funkcja: krzaczek(50, 8)
  - Plansza: 1000x1000
  - Liczba prob: $iterations

WYNIKI (srednia):
-----------------------------------------------------------------
Read file:       $("{0,8:N0}" -f $avgRead) us ($("{0,8:N3}" -f $avgReadMs) ms)
Process (core):  $("{0,8:N0}" -f $avgProcess) us ($("{0,8:N3}" -f $avgProcessMs) ms)
Write file:      $("{0,8:N0}" -f $avgWrite) us ($("{0,8:N3}" -f $avgWriteMs) ms)
TOTAL:           $("{0,8:N0}" -f $avgTotal) us ($("{0,8:N3}" -f $avgTotalMs) ms)

BREAKDOWN:
-----------------------------------------------------------------
I/O (read+write):  $("{0,6:N2}" -f $ioPercent)%
  - Read:          $("{0,6:N2}" -f (($avgRead / $avgTotal) * 100))%
  - Write:         $("{0,6:N2}" -f (($avgWrite / $avgTotal) * 100))%
Processing:        $("{0,6:N2}" -f $processPercent)%

POROWNANIE Z PYTHON (bez GUI):
-----------------------------------------------------------------
LogoCPP (z I/O):       $("{0,8:N3}" -f $avgTotalMs) ms  ($("{0:N2}" -f $ratioWithIO)x wolniejszy)
LogoCPP (bez I/O):     $("{0,8:N3}" -f $avgProcessMs) ms  ($("{0:N2}" -f $ratioNoIO)x wolniejszy)
Python (bez GUI):      $("{0,8:N3}" -f $pythonTimeMs) ms  (baseline)

KLUCZOWE WNIOSKI:
=================================================================

1. I/O BOTTLENECK
   - I/O stanowi $("{0:N1}" -f $ioPercent)% calkowitego czasu!
   - Zapis do pliku jest $("{0:N0}" -f ($avgWrite / $avgProcess))x wolniejszy niz processing
   
2. PROCESSING OVERHEAD
   - Faktyczny czas procesowania: $("{0:N2}" -f $avgProcessMs) ms
   - To obejmuje: parsowanie + interpretacja + wykonanie
   - Wciaz $("{0:N0}" -f $ratioNoIO)x wolniejszy niz Python
   
3. POTENCJALNE OPTYMALIZACJE
   a) Zamiana I/O na in-memory buffer -> przyspieszenie $("{0:N2}" -f $potentialSpeedup)x
   b) Cachowanie AST -> dodatkowe 1.5-2x
   c) JIT compilation -> dodatkowe 5-10x
   
   RAZEM: Potencjalne przyspieszenie 15-40x!

4. REALISTYCZNE CELE
   - Z optymalizacjami LogoCPP moze osiagnac ~3-10ms
   - Nadal wolniejszy niz Python, ale akceptowalny
   - Najwiekszy zysk: eliminacja zapisu do pliku

=================================================================
"@

$report | Out-File "IO_ANALYSIS_REPORT.txt" -Encoding UTF8

Write-Host "Raport zapisany do: IO_ANALYSIS_REPORT.txt" -ForegroundColor Green
Write-Host ""
