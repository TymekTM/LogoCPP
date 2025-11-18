#include <iostream>
#include "../LogoCore/LogoCore.h"

int main() {

    char** result = TurtleInstructions("x = 10; a=10; b=10; Forward(10); Right(a*b); Forward(a+b); Right(90);  Forward(10); Right(90);              Forward(x);", 50,50);

    
    for (int i = 0; i < 50; ++i) {
        for (int j = 0; j < 50; ++j) {
            std::cout << result[i][j];
        }
        std::cout << std::endl;
    }
    
    return 0;
}
