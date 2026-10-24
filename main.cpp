#include "runners/runner.h"

#include <iostream>

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: compiler <source-file>" << endl;
        return 1;
    }
    return runSourceFile(argv[1]);
}
