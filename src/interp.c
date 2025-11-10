#include "interp.h"
#include "lexer.h"
#include "diag.h"
#include <stdio.h>
#include <string.h>

static bool eval_expr(const Expr* expr, SymTab* st, const char* filename, int line, int col, int32_t* out) {
    int32_t stack[MAX_EXPR_ITEMS];
    int sp = 0;

    for (int i = 0; i < expr->count; ++i) {
        ExprItem item = expr->items[i];
        switch (item.kind) {
            case EXPR_ITEM_NUMBER:
                if (sp >= MAX_EXPR_ITEMS) {
                    diag_error(filename, line, col, "expression stack overflow");
                    return false;
                }
                stack[sp++] = item.as.number;
                break;
            case EXPR_ITEM_VAR: {
                int32_t value;
                if (!st_get(st, item.as.var, &value)) {
                    char msg[96];
                    snprintf(msg, sizeof(msg), "undefined variable '%s'", item.as.var);
                    diag_error(filename, line, col, msg);
                    return false;
                }
                if (sp >= MAX_EXPR_ITEMS) {
                    diag_error(filename, line, col, "expression stack overflow");
                    return false;
                }
                stack[sp++] = value;
                break;
            }
            case EXPR_ITEM_OP: {
                if (sp < 2) {
                    diag_error(filename, line, col, "not enough operands for operator");
                    return false;
                }
                int32_t rhs = stack[--sp];
                int32_t lhs = stack[--sp];
                int32_t result = 0;
                if (item.as.op == EXPR_OP_ADD) {
                    result = lhs + rhs;
                } else if (item.as.op == EXPR_OP_SUB) {
                    result = lhs - rhs;
                }
                stack[sp++] = result;
                break;
            }
        }
    }

    if (sp != 1) {
        diag_error(filename, line, col, "expression did not reduce to a value");
        return false;
    }

    *out = stack[0];
    return true;
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
            int32_t value;
            if (!eval_expr(&s.printStmt.expr, &st, lx.filename, s.line, s.col, &value)) {
                continue;
            }
            printf("%d\n", value);
        } else if (s.kind == STMT_VAR) {
            if (!s.varStmt.has_value) {
                diag_error(lx.filename, s.line, s.col, "VAR without initializer is not supported yet");
                continue;
            }
            int32_t value;
            if (!eval_expr(&s.varStmt.value_expr, &st, lx.filename, s.line, s.col, &value)) {
                continue;
            }
            if (!st_set(&st, s.varStmt.name, value)) {
                diag_error(lx.filename, s.line, s.col, "symbol table full");
                continue;
            }
        }
    }

    lx_close(&lx);
    return true;
}
