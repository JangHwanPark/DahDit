#ifndef SYMTAB_H
#define SYMTAB_H
#include <stdbool.h>
#include <stdint.h>

#define MAX_SYMS 256
#define MAX_NAME 64

typedef struct {
    char name[MAX_NAME];
    int32_t value;
    bool used;
} Symbol;

typedef struct {
    Symbol syms[MAX_SYMS];
} SymTab;

void st_init(SymTab* st);
bool st_set(SymTab* st, const char* name, int32_t value);
bool st_get(SymTab* st, const char* name, int32_t* out);

#endif