#include "interp.h"
#include <stdio.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.dit>\n", argv[0]);
        return 1;
    }
    if (!run_program(argv[1])) return 1;
    return 0;
}
