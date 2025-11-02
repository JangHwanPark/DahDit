#include <stdio.h>
#include <stdlib.h>
#include "engine.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <hello.morse>\n", argv[0]);
        return 1;
    }
    return engine_run_file(argv[1]);
}

