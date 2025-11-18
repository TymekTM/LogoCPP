#include <iostream>
#include "../LogoCore/LogoCore.h"

int main() {

    char** result = TurtleInstructions("x=10;y=90;Left(0);Forward(x);Right(y);Forward(x);Right(y);Forward(x);Right(y);Forward(x);Right(y);", 50, 50);
    
    for (int i = 0; i < 50; ++i) {
        for (int j = 0; j < 50; ++j) {
            std::cout << result[i][j];
        }
        std::cout << std::endl;
    }
    
    return 0;
}
