#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STMT_PRINT,
    STMT_VAR
} StmtKind;

typedef enum {
    EXPR_ITEM_NUMBER,
    EXPR_ITEM_VAR,
    EXPR_ITEM_OP,
} ExprItemKind;

typedef enum {
    EXPR_OP_ADD,
    EXPR_OP_SUB,
} ExprOp;

typedef struct {
    ExprItemKind kind;
    union {
        int32_t number;
        char var[64];
        ExprOp op;
    } as;
} ExprItem;

#define MAX_EXPR_ITEMS 64

typedef struct {
    ExprItem items[MAX_EXPR_ITEMS];
    int count;
} Expr;

typedef struct {
    Expr expr;
} PrintStmt;

typedef struct {
    char name[64];
    bool has_value;
    Expr value_expr;
} VarStmt;

typedef struct {
    StmtKind kind;
    int line, col;
    union {
        PrintStmt printStmt;
        VarStmt varStmt;
    };
} Stmt;

typedef struct {
    Lexer* lx;
    Token cur;
} Parser;

void ps_init(Parser* ps, Lexer* lx);
bool ps_next_stmt(Parser* ps, Stmt* out); // 한 문장씩 파싱, EOF면 false

#endif