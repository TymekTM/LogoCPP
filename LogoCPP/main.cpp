#include <iostream>
#include "../LogoCore/LogoCore.h"

int main() {

    // Test z if - jeśli x > 5, narysuj kwadrat
    char** result = TurtleInstructions("x=10;y=90;if(x<100){Left(45);Forward(x);Right(y);Forward(x);Right(y);Forward(x);Right(y);Forward(x);};Right(45);Forward(x)", 50, 50);

    for (int i = 0; i < 50; ++i) {
        for (int j = 0; j < 50; ++j) {
            std::cout << result[i][j];
        }
        std::cout << std::endl;
    }

    return 0;
}
