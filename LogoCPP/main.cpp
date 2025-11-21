#include <iostream>
#include "../LogoCore/LogoCore.h"

void printResult(const char* testName, char** result, int size) {
    std::cout << "\n=== " << testName << " ===" << std::endl;
    int count = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            std::cout << result[i][j];
            if (result[i][j] != ' ') count++;
        }
        std::cout << std::endl;
    }
}

int main() {
    // Test 5: Rekurencja - Spirala
    printResult("Test 1: Spirala (rekurencja)", 
        TurtleInstructions("minSize=2; def spirala(size) { if(size>minSize) { Forward(size); Right(90); spirala(size-2); }; }; spirala(20);", 50, 50), 50);

    // Test 6: Fraktal - Drzewo
    printResult("Test 2: FRAKTAL - Drzewo", 
        TurtleInstructions("minLen=3; angle=30; def drzewo(len) { if(len>minLen) { Forward(len); Left(angle); drzewo(len-2); Right(angle); Right(angle); drzewo(len-2); Left(angle); Backward(len); }; }; drzewo(12);", 100, 100), 100);
    return 0;
}
