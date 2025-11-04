#include <stdio.h>
#include "diag.h"

void diag_error(const char *file, int line, int col, const char *msg) {
    fprintf(stderr, "%s:%d:%d: error: %s\n", file ? file : "<stdin>", line, col, msg);
}
