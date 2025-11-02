#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "util.h"

int run_spaces(const char** pp) {
    const char* p = *pp; int n = 0;
    while (*p==' '||*p=='\t'||*p=='\n'||*p=='\r'||*p=='\f'||*p=='\v') { ++n; ++p; }
    *pp = p; return n;
}

int run_dots(const char** pp) {
    const char* p = *pp; int n = 0;
    while (*p=='.') { ++n; ++p; }
    *pp = p; return n;
}

int run_dashes(const char** pp) {
    const char* p = *pp; int n = 0;
    while (*p=='-') { ++n; ++p; }
    *pp = p; return n;
}

char* slurp_file(const char* path, size_t* size_out) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t r = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (r != (size_t)sz) { free(buf); return NULL; }

    buf[sz] = '\0';
    if (size_out) *size_out = (size_t)sz;
    return buf;
}
