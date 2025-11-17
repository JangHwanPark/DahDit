//========================================
// System Includes
//========================================
#include "symtab.h"
#include <string.h>


void st_init(SymTab* st) {
    memset(st, 0, sizeof(*st));
}

bool st_set(SymTab* st, const char* name, int32_t value) {
    // update if exists
    for (int i = 0; i < MAX_SYMS; ++i) {
        if (st->syms[i].used && strcmp(st->syms[i].name, name) == 0) {
            st->syms[i].value = value; return true;
        }
    }
    // insert new
    for (int i = 0; i < MAX_SYMS; ++i) {
        if (!st->syms[i].used) {
            strncpy(st->syms[i].name, name, MAX_NAME-1);
            st->syms[i].name[MAX_NAME-1] = '\0';
            st->syms[i].value = value;
            st->syms[i].used = true;
            return true;
        }
    }
    return false;
}

bool st_get(SymTab* st, const char* name, int32_t* out) {
    for (int i = 0; i < MAX_SYMS; ++i) {
        if (st->syms[i].used && strcmp(st->syms[i].name, name) == 0) {
            if (out) *out = st->syms[i].value;
            return true;
        }
    }
    return false;
}
