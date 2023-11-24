#include "HistoryManager.h"
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 1)
    {
        std::cout << "Numarul de argumente nu este corect\n";
        return -1;
    }
    std::cout << "I love " << argv[0] + 2 << '\n';
    return 0;
}