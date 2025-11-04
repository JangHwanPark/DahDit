#ifndef PARSER_H
#define PARSER_H
#include "lexer.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STMT_PRINT,
    STMT_VAR
} StmtKind;

// PRINT 리터럴/식별자
typedef struct {
    bool is_var;      // true면 varName 사용, false면 literal 사용
    char varName[64]; // is_var=true일 때
    // literal: 여러 단어(문자열)의 개수와 내용
    int literal_word_count;
    char literal_words[16][64]; // 단어 최대 16, 각 단어 63자 제한
} PrintStmt;

// VAR name = number ;
typedef struct {
    char name[64];
    int32_t value;
    bool has_value;
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