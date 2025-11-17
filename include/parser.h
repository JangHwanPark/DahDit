#ifndef PARSER_H
#define PARSER_H
//========================================
// System Includes
//========================================
#include "lexer.h"
#include <stdbool.h>
#include <stdint.h>

//========================================
// Statement Kinds (문장 종류)
// PRINT <expr> ; (정수 출력)
// PRINT_STR <string> ; (문자열 출력)
// VAR <name> = <expr> ; (변수 선언 및 할당)
//========================================
typedef enum {
    STMT_PRINT,
    STMT_PRINT_STR,
    STMT_VAR
} StmtKind;

//========================================
// Expression Item Kinds (표현식 구성 요소)
//========================================
typedef enum {
    EXPR_ITEM_NUMBER,
    EXPR_ITEM_VAR,
    EXPR_ITEM_OP,
} ExprItemKind;

//========================================
// Expression Operators (연산자)
//========================================
typedef enum {
    EXPR_OP_ADD,    // +
    EXPR_OP_SUB,    // -
    EXPR_OP_MUL,    // * (곱셈 추가)
    EXPR_OP_DIV,    // / (나눗셈 추가)
    EXPR_OP_MOD     // % (나머지 추가)
} ExprOp;

//========================================
// Expression Item Structure
//========================================
typedef struct {
    ExprItemKind kind;
    union {
        int32_t number;     // 리터럴 숫자 값
        char var[64];       // 변수 이름 (식별자)
        ExprOp op;          // 연산자 종류
    } as;
} ExprItem;

#define MAX_EXPR_ITEMS 64

//========================================
// Expression Structure (표현식 전체)
//========================================
typedef struct {
    ExprItem items[MAX_EXPR_ITEMS];
    int count;
} Expr;

//========================================
// Statement Structures
//========================================
typedef struct {
    Expr expr;          // 출력할 연산식
} PrintStmt;

typedef struct {
    char text[128];     // 출력할 문자열 리터럴 (TK_STRING 내용)
} PrintStrStmt;

typedef struct {
    char name[64];      // 변수 이름
    bool has_value;     // 할당 값 유무
    Expr value_expr;    // 할당될 표현식
} VarStmt;

//========================================
// Main Statement Structure
//========================================
typedef struct {
    StmtKind kind;
    int line, col;
    union {
        PrintStmt printStmt;
        PrintStrStmt printStrStmt;
        VarStmt varStmt;
    };
} Stmt;

//========================================
// Parser State Structure
// Lexer: 렉서 포인터
// Token: 현재 토큰 (Lookahead)
//========================================
typedef struct {
    Lexer* lx;
    Token cur;
} Parser;

//========================================
// Function Prototypes
//========================================
void ps_init(Parser* ps, Lexer* lx);
bool ps_next_stmt(Parser* ps, Stmt* out); // 한 문장씩 파싱, EOF면 false

#endif