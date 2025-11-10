#include "parser.h"
#include "diag.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static void advance(Parser* ps) { ps->cur = lx_next(ps->lx); }

// 연속 LETTER를 모아 하나의 단어(식별자/숫자열)로 만든다.
// '/'나 ';', '=', NEWLINE 전까지 글자만 모음.
static bool parse_word(Parser* ps, char* out, int outsz) {
    int n = 0;
    while (ps->cur.kind == TK_LETTER) {
        if (n < outsz-1) out[n++] = ps->cur.ch;
        advance(ps);
        // LETTER 다음이 공백/NEWLINE은 lexer가 먹음, 여기선 LETTER 아니면 word 종료
    }
    out[n] = '\0';
    return n > 0;
}

static void skip_separators(Parser* ps) {
    while (ps->cur.kind == TK_NEWLINE || ps->cur.kind == TK_SLASH) advance(ps);
}

static bool is_kw(const char* w, const char* kw) {
    return strcmp(w, kw) == 0;
}

void ps_init(Parser* ps, Lexer* lx) {
    ps->lx = lx;
    ps->cur = lx_next(lx);
}

static bool expr_push_number(Parser* ps, Expr* expr, int32_t value) {
    if (expr->count >= MAX_EXPR_ITEMS) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expression too long");
        return false;
    }
    expr->items[expr->count].kind = EXPR_ITEM_NUMBER;
    expr->items[expr->count].as.number = value;
    expr->count++;
    return true;
}

static bool expr_push_var(Parser* ps, Expr* expr, const char* name) {
    if (expr->count >= MAX_EXPR_ITEMS) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expression too long");
        return false;
    }
    expr->items[expr->count].kind = EXPR_ITEM_VAR;
    strncpy(expr->items[expr->count].as.var, name, sizeof(expr->items[expr->count].as.var));
    expr->items[expr->count].as.var[sizeof(expr->items[expr->count].as.var)-1] = '\0';
    expr->count++;
    return true;
}

static bool expr_push_op(Parser* ps, Expr* expr, ExprOp op) {
    if (expr->count >= MAX_EXPR_ITEMS) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expression too long");
        return false;
    }
    expr->items[expr->count].kind = EXPR_ITEM_OP;
    expr->items[expr->count].as.op = op;
    expr->count++;
    return true;
}

static bool parse_factor(Parser* ps, Expr* expr) {
    skip_separators(ps);
    if (ps->cur.kind != TK_LETTER) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expected number or identifier in expression");
        return false;
    }
    char word[64] = {0};
    if (!parse_word(ps, word, sizeof(word))) return false;

    bool all_digits = true;
    for (int i = 0; word[i]; ++i) {
        if (!isdigit((unsigned char)word[i])) { all_digits = false; break; }
    }

    if (all_digits && word[0] != '\0') {
        int32_t value = (int32_t)strtol(word, NULL, 10);
        return expr_push_number(ps, expr, value);
    }

    return expr_push_var(ps, expr, word);
}

static bool parse_expr(Parser* ps, Expr* expr) {
    expr->count = 0;
    if (!parse_factor(ps, expr)) return false;

    for (;;) {
        skip_separators(ps);
        if (ps->cur.kind == TK_PLUS || ps->cur.kind == TK_MINUS) {
            TokenKind op_kind = ps->cur.kind;
            advance(ps);
            if (!parse_factor(ps, expr)) return false;
            ExprOp op = (op_kind == TK_PLUS) ? EXPR_OP_ADD : EXPR_OP_SUB;
            if (!expr_push_op(ps, expr, op)) return false;
        } else {
            break;
        }
    }
    return true;
}

// PRINT <expr> ;
static bool parse_print(Parser* ps, Stmt* out) {
    out->kind = STMT_PRINT;
    out->line = ps->cur.line; out->col = ps->cur.col;
    if (!parse_expr(ps, &out->printStmt.expr)) return false;

    skip_separators(ps);
    if (ps->cur.kind != TK_SEMI) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "missing ';' after PRINT");
        return false;
    }
    advance(ps);
    return true;
}

// VAR name (= expr)? ;
static bool parse_var(Parser* ps, Stmt* out) {
    out->kind = STMT_VAR;
    out->varStmt.name[0] = '\0';
    out->varStmt.has_value = false;
    out->line = ps->cur.line; out->col = ps->cur.col;

    skip_separators(ps);
    if (ps->cur.kind != TK_LETTER) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expected identifier after VAR");
        return false;
    }
    if (!parse_word(ps, out->varStmt.name, sizeof(out->varStmt.name))) return false;

    skip_separators(ps);
    if (ps->cur.kind == TK_EQ) {
        advance(ps);
        if (!parse_expr(ps, &out->varStmt.value_expr)) return false;
        out->varStmt.has_value = true;
    }

    skip_separators(ps);
    if (ps->cur.kind != TK_SEMI) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "missing ';' after VAR statement");
        return false;
    }
    advance(ps);
    return true;
}

bool ps_next_stmt(Parser* ps, Stmt* out) {
    skip_separators(ps);
    if (ps->cur.kind == TK_EOF) return false;

    // 첫 단어(키워드) 읽기
    if (ps->cur.kind != TK_LETTER) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expected statement");
        // 에러 동기화: 세미콜론까지 스킵
        while (ps->cur.kind != TK_SEMI && ps->cur.kind != TK_EOF) advance(ps);
        if (ps->cur.kind == TK_SEMI) advance(ps);
        return true; // 계속 진행
    }

    char kw[16] = {0};
    if (!parse_word(ps, kw, sizeof(kw))) return false;

    if (is_kw(kw, "PRINT")) {
        return parse_print(ps, out);
    } else if (is_kw(kw, "VAR")) {
        return parse_var(ps, out);
    } else {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "unknown statement (expected PRINT or VAR)");
        // 세미콜론까지 스킵
        while (ps->cur.kind != TK_SEMI && ps->cur.kind != TK_EOF) advance(ps);
        if (ps->cur.kind == TK_SEMI) advance(ps);
        return true;
    }
}
