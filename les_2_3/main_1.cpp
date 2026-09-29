#include <iostream>
#include <string>

const std::string EXIT = "exit";

int main() {
    /*
        It is a just an example. Programm works only with positive number. Otherwise - uncatched exception
    */
    std::string input;
    std::cout << "Please enter the positive number. Exter 'exit' to the exit." << std::endl;

    while (std::cin >> input && input != EXIT)
        {
            int n = std::stoi(input);

            // Bitwise AND with 1 works as a mask. 
            // It keeps only the last (least significant) bit of n and zeroes out all the other bits. 
            // That last bit alone tells us whether n is odd or even.
            //
            // n = 13 (1101)
            //   1101   (13)
            // & 0001   (1)
            // ------
            //   0001   -> result is 1 (non-zero) -> is odd
            //
            // n = 8 (1000)
            //   1000   (8)
            // & 0001   (1)
            // ------
            //   0000   -> result is 0 -> is even
            if (n & 1)
                std::cout << "Odd" << std::endl;
            else
                std::cout << "Even" << std::endl;
        }
    return 0;
}
