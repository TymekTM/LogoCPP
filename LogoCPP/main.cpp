#include <iostream>
#include "../LogoCore/LogoCore.h"

int main() {

    char** result = TurtleInstructions("Forward(10); Right(90); Forward(10); Right(90);  Forward(10); Right(90);              Forward(10);");
    
    for (int i = 0; i < 25; ++i) {
        for (int j = 0; j < 25; ++j) {
            std::cout << result[i][j];
        }
        std::cout << std::endl;
	}
    
    return 0;
}
