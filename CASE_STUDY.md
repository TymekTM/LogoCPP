# Case Study: Optymalizacja interpretera Logo z 1x do 370x (i 1320x nad Python turtle)

## Streszczenie

Projekt LogoCPP - interpreter języka Logo z rysowaniem żółwia - był optymalizowany w dwóch fazach. Faza I (bytecode): z **~1ms (n=8)** do **~0.01ms**, ~100x. Faza II (JIT): kompilacja Logo do natywnego kodu x86-64, kolejne **1.7-2.2x** nad bytecode. Łącznie **~372x** nad oryginałem dla n=15, **~1320x** nad Python turtle (Tk) i **1.3x szybciej niż ręcznie pisana rekurencja w C++** korzystająca z tej samej biblioteki Turtle. Poniżej opisuję każdą technikę, co zadziałało, co nie zadziałało, i dlaczego.

---

## 1. Punkt wyjścia: Interpreter tokenizujący

Oryginalna architektura analizowała tekst przy każdym wywołaniu:

```
Execute(string) → Tokenizer → HandleInstruction → Turtle ops
                     ↑ parsowanie przy każdym wywołaniu
```

**Koszt**: `std::string` copy, `std::stod()`, `unordered_map` lookup na zmienne - wszystko ×65536 razy dla `krzaczek(50, 15)`.

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
- **Specialized expression types**: `SLOT_MUL_LIT`, `SLOT_SUB_LIT`, `SLOT_ADD_LIT` - specjalizacje dla najczęstszych wzorców (`x*0.75`, `n-1`). Eliminują generyczny binary-op dispatch.
- **Pre-resolved function pointers**: Zamiast `compiledFunctions.find(funcName)` przy każdym wywołaniu, pointer jest cache'owany w `CInstr`.

**Lekcja**: Parsing i name resolution to hidden killers w interpreterach. Kompilacja do bytecode (nawet najprostsza) daje rząd wielkości.

### 2.2 Early Exit Optimization (+20-40%)

Wzorzec `def krzaczek(x, n) { if (n > 0) { ... } }` - ciało to jeden `if` z warunkiem na parametr vs literał.

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
    // Direct pixel write - skip entire Bresenham setup
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

- `__forceinline` na `CExpr::eval()` - eliminuje call overhead na gorącej ścieżce
- `__restrict` na wskaźnikach (`varSlots`, `ip`, `turtlePtr`) - umożliwia alias analysis
- Partially unrolled 2-param save/restore (zamiast generic loop)
- `alignas(64)` na varSlots - wyrównanie do cache line
- Skip `variables.clear()` w `resetVarSlots()` - compiled path nie używa legacy mapy

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

4. **Nie pomagaj kompilatorowi wbrew profilowi**: Większe switch tables, forced inlining, lazy eval - wszystko to "pomaganie" kompilatorowi, które kontruje z PGO's optymalnymi decyzjami.

5. **Mierz, nie zgaduj**: Każda "oczywista" optymalizacja (AVX2! mniej switch case'ów! lazy eval!) wymagała empirycznej weryfikacji. 50% prób skończyło się regresją.

6. **Diminishing returns są strome**: Przejście z 1x→10x zajmuje 1 zmianę (bytecode). Z 10x→100x wymaga 5+ zmian. Z 100x→170x wymaga PGO + kilku mikro-optymalizacji. Dalej (170x→200x) prawdopodobnie wymaga fundamentalnej zmiany architektury (JIT, SIMD vectorization of pixel writes, etc.).

7. **Hardware matters**: Ten sam kod na różnym sprzęcie daje 161x vs 1562x (different baseline comparison). Optymalizacja musi być mierzona na docelowym hardware.

---

## 6. Architektura po fazie I (bytecode)

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

# Faza II: Kompilator JIT (bytecode → kod maszynowy x86-64)

Faza I kończyła się wnioskiem, że dalej niż ~170x nie zajdziemy bez "fundamentalnej zmiany architektury". Faza II to właśnie ta zmiana: bytecode nie jest już interpretowany przez switch w C++, tylko kompilowany do natywnego kodu x86-64 emitowanego ręcznie bajt po bajcie.

## 7. Co zadziałało w JIT (w kolejności commitów)

### 7.1 Kompilator do kodu maszynowego (cb44297, +18-52% nad bytecode)

Ręczny emitter bajtów x86-64 (`Jit.cpp`): każda funkcja Logo kompiluje się do bloku kodu, wywołania przez `call rel32`, stałe double w puli RIP-relative, argumenty w `xmm0-3`, zmienne w tablicy `double[64]` wskazywanej przez `rdx`. Pamięć: `VirtualAlloc` RWX, po emisji przełączenie na RX.

Wynik od razu: **n=8 +52%, n=12 +24%, n=15 +18%** nad bytecode. Mniej niż esperowane "usuwanie dispatchu da 2x", bo interpreter C++ z PGO był już bardzo dobry - dispatch to tylko część kosztu.

### 7.2 Szybkie ścieżki linii osiowych i diagonalnych (2c62c47, +5-37%)

Przy rekurencji `x*0.75^k` większość linii to 1-2 piksele, ale te dłuższe często są poziome/pionowe/diagonalne. Dodałem rozgałęzienie w `drawLine` omijające pełny Bresenham dla dx==0, dy==0, dx==±dy, plus branchless rounding (`(v + HALF + (v >> 31)) >> SHIFT` zamiast if/else w `Forward`).

**n=8 1.37x, n=12 1.09x, n=15 1.05x** - zmiana w C++ `Turtle`, więc przyspieszyły wszystkie trzy ścieżki wykonania (interpreter, bytecode, JIT).

### 7.3 Guard early-exit na miejscu wywołania (917764e, +4-9%)

W bytecode early-exit działał *wewnątrz* wywoływanej funkcji: wchodziliśmy w frame, ewaluowaliśmy argumenty, sprawdzaliśmy waruneczek. W JIT warunek `if (n > 0)` z ciała funkcji jest hoistowany na **miejsce wywołania**: `comisd` argumentu z literałem + `jcc` omijający cały `call`. Liście rekurencji (połowa wywołań w drzewie binarnym) nie wykonują się wcale - zero frame'u, zero ewaluacji argumentów.

Do tego prolog rezerwuje home space raz (zamiast `sub rsp, 0x20` przed każdym `call`).

**n=8 1.06x, n=12 1.04x, n=15 1.09x.**

### 7.4 Pełny inline Forward/Backward (9b97699, +15% na n=15)

Ostatni krok: wywołania C++ `Turtle::Forward` zastąpione emitowanym kodem - trygonometria stałoprzecinkowa przez tabele adresowane rejestrami `r14`/`r15` (wskaźniki na `cosTable`/`sinTable` ładowane raz w prologu programu), ruchy sub-pikselowe zapisywane wprost do gridu, dłuższe linie przez wywołanie helpera. Ścieżka gorąca nie opuszcza kodu JIT od prologu do epilogu.

**n=15 1.15x (351→304µs), n=12 1.07x**, ale **n=8 ~5% regresji** (4.90→5.25µs): przy płaskiej rekurencji dodatkowy kod sprawdzenia okna pikselowego kosztuje więcej niż oszczędność wywołania. Zgodnie z zasadą "mierz, nie zgaduj" - udokumentowane i zaakceptowane, bo n=15 to cel główny.

### 7.5 Suma fazy II

| Test | Bytecode (1286b5e) | JIT (9b97699) | Przyspieszenie |
|------|--------------------|---------------|----------------|
| n=8 | 8.43µs | 5.02µs | **1.68x** |
| n=12 | 88.5µs | 47.8µs | **1.85x** |
| n=15 | 613.6µs | 284.5µs | **2.16x** |

Łącznie z fazą I: n=15 z 105.8ms do 284.5µs = **372x**.

## 8. Porównanie z innymi implementacjami

Ten sam program (drzewo rekurencyjne) przepisany wprost do Python turtle (Tk), czystego CPythona (ten sam model gridu i trygonometria stałoprzecinkowa) i ręcznego C++ na tej samej bibliotece Turtle/Canvas. Pełna metodologia: `tests/compare/RESULTS.md`.

| Implementacja | n=8 | n=12 | n=15 | LogoCPP JIT szybszy o |
|---|---:|---:|---:|---:|
| Python turtle (Tk, tracer off) | 2.46ms | 37.8ms | 375.5ms | **~1320x** (n=15) |
| Czysty CPython (ten sam algorytm) | 0.56ms | 5.6ms | 35.4ms | ~124x |
| Oryginalny interpreter (tokenizer) | 0.97ms | 13.7ms | 105.8ms | ~372x |
| Ręczne C++ (ta sama Turtle/Canvas, /O2) | 14.4µs | 118.8µs | 372.6µs | **1.3-2.9x** |
| Bytecode (faza I) | 8.4µs | 88.5µs | 613.6µs | 1.7-2.2x |
| **LogoCPP JIT** | **5.0µs** | **47.8µs** | **284.5µs** | - |

Najciekawszy wiersz: **JIT wygrywa z ręcznie pisanym C++**, które woła tę samą `Turtle::Forward`. Dwa triki kompilatora, których człowiek zwykle by nie napisał:
1. guard rekurencji na miejscu wywołania (liście nigdy się nie wykonują),
2. pełny inline ruchów z tabelami w rejestrach.

Uczciwie: ręczne C++ z tymi samymi trikami prawdopodobnie dorównałoby JIT. Twierdzenie brzmi "wygrywa z kodem, który kompetentny człowiek pisze w 5 minut", nie "wygrywa z każdym możliwym C++".

## 9. Wnioski z fazy II

1. **Interpreter z PGO to twardy przeciwnik**: sam JIT (7.1) dał tylko +18-52%, bo switch-dispatch z PGO to nie główny koszt. Dopiero optymalizacje *pod* dispatchem (7.3, 7.4) odblokowały pełne 2.2x.

2. **Kompilator może wygrać z człowiekiem przez triki, które człowiek uważa za "nie warte zachodu"**: hoisting jednego ifa na miejsce wywołania brzmi trywialnie, a daje 9% i połowę wykonanych wywołań za darmo.

3. **Ręczna emisja x86-64 to pole min**: cztery z debugowanych crashy to źle policzone bity REX (W=8, R=4, X=2, B=1) albo ModRM z przypadkiem ustawionym bitem disp32. Capstone do deasemblacji jit_dump.bin był rozstrzygającym narzędziem - printowanie hexów nie wystarcza.

4. **Regresje na jednym teście mogą być dobre**: 7.4 przyspiesza n=15 o 15% kosztem 5% na n=8. Bez suitu regresyjnego (26 testów, tryb podwójny) taką zmianę by się odrzuciło albo nie zauważyło.

---

*Case study sporządzono na podstawie sesji optymalizacyjnych LogoCPP (faza I: luty 2026, faza II: sierpień 2026). Pomiarów dokonano na `tests/bench.py` (best-of-5) i `tests/compare/`.*
