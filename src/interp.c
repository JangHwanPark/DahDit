#include "interp.h"
#include "lexer.h"
#include "diag.h"
#include <stdio.h>
#include <string.h>

static void print_literal(const PrintStmt* ps) {
    for (int i = 0; i < ps->literal_word_count; ++i) {
        if (i) putchar(' ');
        fputs(ps->literal_words[i], stdout);
    }
    putchar('\n');
}

bool run_program(const char* filename) {
    Lexer lx;
    if (!lx_open(&lx, filename)) {
        fprintf(stderr, "cannot open: %s\n", filename);
        return false;
    }

    Parser ps; ps_init(&ps, &lx);
    SymTab st; st_init(&st);

    Stmt s;
    while (ps_next_stmt(&ps, &s)) {
        if (s.kind == STMT_PRINT) {
            if (s.printStmt.is_var) {
                int32_t v;
                if (!st_get(&st, s.printStmt.varName, &v)) {
                    char msg[96];
                    snprintf(msg, sizeof(msg), "undefined variable '%s'", s.printStmt.varName);
                    diag_error(lx.filename, s.line, s.col, msg);
                    continue;
                }
                printf("%d\n", v);
            } else {
                print_literal(&s.printStmt);
            }
        } else if (s.kind == STMT_VAR) {
            if (!s.varStmt.has_value) {
                diag_error(lx.filename, s.line, s.col, "VAR without initializer is not supported yet");
                continue;
            }
            if (!st_set(&st, s.varStmt.name, s.varStmt.value)) {
                diag_error(lx.filename, s.line, s.col, "symbol table full");
                continue;
            }
        }
    }

    lx_close(&lx);
    return true;
}
