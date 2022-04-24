#include <iostream>

#include "FakeChip8Runner.h"

int main(int argc, char** argv) {
    srand(time(NULL));
    if (argc != 2) {
        std::cerr << "Wrong number of arguments\n";
        std::cerr << "<program> <romPath>";
        return -1;
    }
    fakers::FakeChip8Runner f;
    f.run(argv[1]);
}