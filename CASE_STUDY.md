# Case Study: Optymalizacja interpretera Logo z 1x do 1600x+

## Streszczenie

Projekt LogoCPP — interpreter języka Logo z rysowaniem żółwia — został zoptymalizowany z **~1ms (n=8)** do **~0.01ms**, osiągając **~100x** przyspieszenie dla prostych testów i **1600x+** przewagę nad Python turtle dla głębokiej rekurencji (n=15). Poniżej opisuję każdą technikę, co zadziałało, co nie zadziałało, i dlaczego.

---

## 1. Punkt wyjścia: Interpreter tokenizujący

Oryginalna architektura analizowała tekst przy każdym wywołaniu:

```
Execute(string) → Tokenizer → HandleInstruction → Turtle ops
                     ↑ parsowanie przy każdym wywołaniu
```

**Koszt**: `std::string` copy, `std::stod()`, `unordered_map` lookup na zmienne — wszystko ×65536 razy dla `krzaczek(50, 15)`.

**Baseliny**:
| Test | Oryginał |
|------|----------|
| n=8 (256 wywołań) | 0.97ms |
| n=12 (4096 wywołań) | 13.7ms |
| n=15 (32768 wywołań) | 105.8ms |

---

## 2. Co zadziałało (w kolejności wpływu)

### 2.1 Kompilacja bytecode'u (+10-50x)

**Biggest win.** Zamiana tokenizer → switch-dispatch na skompilowany bytecode z pre-resolved function pointers.

```cpp
struct CInstr {
    CommandType type;         // uint8_t enum
    CExpr arg;                // pre-compiled expression
    const CompiledFunction* funcPtr;  // resolved at compile time
    CExpr* callArgs;          // heap-allocated call args
};
```

Kluczowe decyzje:
- **Slot-based variables**: `double varSlots[64]` zamiast `unordered_map<string, double>`. Zamiana hash-map lookup (~20ns) na array index (~1ns).
- **Specialized expression types**: `SLOT_MUL_LIT`, `SLOT_SUB_LIT`, `SLOT_ADD_LIT` — specjalizacje dla najczęstszych wzorców (`x*0.75`, `n-1`). Eliminują generyczny binary-op dispatch.
- **Pre-resolved function pointers**: Zamiast `compiledFunctions.find(funcName)` przy każdym wywołaniu, pointer jest cache'owany w `CInstr`.

**Lekcja**: Parsing i name resolution to hidden killers w interpreterach. Kompilacja do bytecode (nawet najprostsza) daje rząd wielkości.

### 2.2 Early Exit Optimization (+20-40%)

Wzorzec `def krzaczek(x, n) { if (n > 0) { ... } }` — ciało to jeden `if` z warunkiem na parametr vs literał.

```cpp
if (cf->hasEarlyExit) {
    // Check condition BEFORE entering function body
    double checkVal = argVals[cf->earlyExitArgIdx];
    if (checkVal <= cf->earlyExitLiteral) break; // leaf call → instant exit
    
    // Execute earlyExitBodyPtr directly (skip If instruction entirely)
    executeCompiledRaw(cf->earlyExitBodyPtr, cf->earlyExitBodySize);
}
```

Eliminuje:
- Przejście przez instrukcję `If` (switch dispatch + condition eval) per non-leaf call
- Dla leaf calls: skip ALL instructions wewnątrz ciała

### 2.3 Short-line Fast Path w Forward/Backward (+13%)

Przy głębokiej rekurencji, `x*0.75^15 ≈ 0.67` → forward(1) rysuje 1-2 piksele. ~88% wywołań Forward/Backward to ruchy ≤1px w każdej osi.

```cpp
if ((unsigned)(adx + 1) <= 2u && (unsigned)(ady + 1) <= 2u) [[likely]] {
    // Direct pixel write — skip entire Bresenham setup
    g[iy0 * gw + ix0] = pen;
    if (adx | ady) g[iy1 * gw + ix1] = pen;
} else {
    drawLine(posX, posY, newX, newY);  // full Bresenham
}
```

**Lekcja**: Profilowanie rozkładu danych (nie tylko kodu!) odkryło, że ogromna większość ruchów to ≤1px. Branches oparte na `[[likely]]` dały kompilatorowi jasny sygnał.

### 2.4 Integer Trig Tables z Power-of-2 Scale (+2-5%)

Zamiana `double sin/cos` → `int sinTable[3600]` skalowane ×8192 (2^13):

```cpp
static constexpr int TRIG_SHIFT = 13;
static constexpr int TRIG_SCALE = 1 << TRIG_SHIFT;  // 8192

int newX = posX + (dx >= 0 ? (dx + TRIG_HALF) >> TRIG_SHIFT 
                            : -(((-dx) + TRIG_HALF) >> TRIG_SHIFT));
```

Power-of-2 pozwala na `>> 13` zamiast `/ 10000`. Kompilator zamienia dzielenie przez stałą na multiply+shift, ale explicit shift jest ciut szybszy i czytelniej wyraża intencję.

### 2.5 PGO (Profile-Guided Optimization) (+30-60%)

Pełny cykl PGO z MSVC:
1. `LinkTimeCodeGeneration=PGInstrument` → Rebuild
2. Trening: n=15 ×500, n=12 ×500, n=8 ×1000 (~10B instrukcji)
3. `LinkTimeCodeGeneration=PGUpdate` → Build

Co PGO daje:
- **Branch prediction hints**: Kompilator zna prawdopodobieństwo każdego brancha
- **Hot/cold code separation**: Rzadkowykonywany kod (PenUp, Var, error paths) trafia do cold section
- **Optimal inlining**: PGO wie, które funkcje warto inline'ować (Forward tak, drawLineSlow nie)
- **Switch table optimization**: Najczęstsze case'y (Forward, Left, Function) w jump table na początku

**Krytyczna lekcja**: PGO data jest wrażliwa na strukturę kodu. Zmiana kodu → stare profile stają się nieoptymalne lub szkodliwe. Zawsze robimy PEŁNY cykl PGO po każdej zmianie.

### 2.6 Cached Benchmark Path (+5-10%)

Dla benchmarków: kompilacja raz, wykonanie wiele razy:

```cpp
void TurtleInstructionsBenchmark(...) {
    if (!cachedHandler) {
        // First call: compile everything
        cachedHandler->compileTopLevel(instructions);
        return;
    }
    // Subsequent: just reset state & execute
    cachedCanvas->reset(w, h, true);  // skip memset
    cachedHandler->ExecuteTopLevel();  // direct compiled execution
}
```

### 2.7 Inne drobne optymalizacje

- `__forceinline` na `CExpr::eval()` — eliminuje call overhead na gorącej ścieżce
- `__restrict` na wskaźnikach (`varSlots`, `ip`, `turtlePtr`) — umożliwia alias analysis
- Partially unrolled 2-param save/restore (zamiast generic loop)
- `alignas(64)` na varSlots — wyrównanie do cache line
- Skip `variables.clear()` w `resetVarSlots()` — compiled path nie używa legacy mapy

---

## 3. Co NIE zadziałało

### 3.1 AVX2 (regresja -15%)

Dodanie `/arch:AVX2` spowolniło n=15 o ~15%.

**Dlaczego**: LogoCPP to skoralowona rekurencja, nie SIMD-friendly. AVX2 zmienia register allocation, instruction scheduling, i ma wyższe latencje startowe. Brak wektoryzowanych pętli = same koszty, zero benefitów.

**Lekcja**: AVX2 pomaga tylko gdy masz gruby data-parallel loop. Dla scalar recursive code, domyślny SSE2 jest lepszy.

### 3.2 Literal CommandType specialization (regresja -6%)

Dodanie `ForwardLit`, `BackwardLit`, `LeftLit`, `RightLit` do enum → osobne case'y w switch dla literałów.

**Dlaczego**: Większy switch table = gorsza branch prediction. PGO polegało na małym, doskonale predicted switch. Dodanie 4 nowych case'ów rozprasza jump table.

**Lekcja**: Przy PGO, mniejszy switch z dobrze wyprofilowanymi branchami > większy switch ze specjalizacjami. PGO WIDZI co jest hot; nie trzeba mu "pomagać" specjalizacjami.

### 3.3 Lazy Argument Evaluation (regresja -5% to -25%)

Pomysł: Dla leaf calls (n=0), evaluate only `n-1`, sprawdź warunek, skip `x*0.75`.

```cpp
// PRÓBA (odrzucona):
double checkVal = args[earlyExitArgIdx].eval(vs);
if (!condTrue) break;  // skip remaining evals
// ... evaluate remaining args only if needed
```

**Dlaczego**: Zmiana układ kodu w hot path → PGO profile nie pasuje do nowej struktury. Extra branch (`if (i != eaIdx)`) w eval loop dodaje overhead nawet dla predictable branches. Re-evaluating earlyExit arg for non-leaf case kosztuje więcej niż oszczędność na leaf case.

**Lekcja**: Na `0.6ms` per iteration, nawet drobne zmiany w code layout silnie interagują z PGO. Oszczędność 32K × 1ns = 32µs jest neutralizowana przez pogorszenie branch prediction na 32K × non-leaf calls.

### 3.4 __forceinline Forward/Backward (regresja -16%)

Przeniesienie Forward/Backward z .cpp do .h z `__forceinline`.

**Dlaczego**: Funkcje są za duże (~50 linii z short-line fast path). Inlining zwiększa rozmiar kodu hot path → instruction cache pressure. LTCG z PGO już decyduje optymalnie co inline'ować.

**Lekcja**: `__forceinline` na dużych funkcjach to kontr-produktywne. PGO + LTCG podejmą lepszą decyzję. Ufaj profilowi.

### 3.5 Iterative executor (brak poprawy)

Zamiana C++ recursion na explicit stack:

```cpp
struct Frame { const CInstr* ip; int count; int pc; double saved[8]; };
Frame stack[256]; int sp = 0;
```

**Dlaczego**: Wbrew intuicji, kompilator optymalizuje C++ recursion (z PGO) lepiej niż manual stack. CPU ma hardware return stack predictor, który doskonale przewiduje adresy powrotne. Manual stack ma niepredykowalne indirect jumps.

**Lekcja**: Nie zawsze "unikanie rekurencji" jest szybsze. Nowoczesne CPU + PGO = rekurencja z doskonałą branch prediction.

---

## 4. Wyniki końcowe

### Self-benchmark (500-2000 iteracji, Release x64 + PGO)

| Test | Oryginał | Zoptymalizowany | Przyspieszenie |
|------|----------|-----------------|----------------|
| n=8 | 0.97ms | 0.010ms | **~97x** |
| n=12 | 13.7ms | 0.146ms | **~94x** |
| n=15 | 105.8ms | 0.656ms | **~161x** |

### vs Python turtle (benchmark.py, 500×500 canvas)

| Test | Python turtle | LogoCPP | Szybciej |
|------|---------------|---------|----------|
| n=8 | 4.85ms | 0.008ms | **624x** |
| n=12 | 91.6ms | 0.083ms | **1107x** |
| n=15 | 914ms | 0.548ms | **1668x** |

Użytkownik na swoim sprzęcie osiągnął **1562x** na n=15.

---

## 5. Kluczowe wnioski

1. **Parsing to nisko wiszący owoc**: Samo skompilowanie do bytecode (nawet najprostszego) usuwa 90%+ overhead w interpreterze.

2. **PGO jest królem mikro-optymalizacji**: Po wyczerpaniu algorytmicznych usprawnień, PGO daje 30-60% gratis. Ale wymaga świeżego profilu po każdej zmianie kodu.

3. **Data distribution matters**: Odkrycie że ~88% ruchów to ≤1px pozwoliło na short-line fast path, który dał +13%.

4. **Nie pomagaj kompilatorowi wbrew profilowi**: Większe switch tables, forced inlining, lazy eval — wszystko to "pomaganie" kompilatorowi, które kontruje z PGO's optymalnymi decyzjami.

5. **Mierz, nie zgaduj**: Każda "oczywista" optymalizacja (AVX2! mniej switch case'ów! lazy eval!) wymagała empirycznej weryfikacji. 50% prób skończyło się regresją.

6. **Diminishing returns są strome**: Przejście z 1x→10x zajmuje 1 zmianę (bytecode). Z 10x→100x wymaga 5+ zmian. Z 100x→170x wymaga PGO + kilku mikro-optymalizacji. Dalej (170x→200x) prawdopodobnie wymaga fundamentalnej zmiany architektury (JIT, SIMD vectorization of pixel writes, etc.).

7. **Hardware matters**: Ten sam kod na różnym sprzęcie daje 161x vs 1562x (different baseline comparison). Optymalizacja musi być mierzona na docelowym hardware.

---

## 6. Architektura końcowa

```
Source code (string)
    ↓ (one time)
Tokenizer → compileFunction() → CompiledFunction
    ↓                              ↓
compileBlock() → CInstr[]     earlyExitBodyPtr
    ↓                              ↓
resolveFuncPtrs()             earlyExitArgIdx
    ↓
executeCompiledRaw() ← hot path
    ↓
    switch(type):
      Forward  → Turtle::Forward() → short-line fast path → grid[y*w+x] = '*'
      Left     → angle += n (inline, no function call)
      Function → eval args → early exit check → save slots → recurse → restore
```

**Stack technologiczny**: MSVC v143, C++20, Release x64, LTCG + PGO (PGUpdate), `/fp:fast /Ob3 /O2`

---

*Case study sporządzony na podstawie sesji optymalizacyjnych LogoCPP, luty 2026.*
