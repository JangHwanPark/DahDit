#ifndef LEXER_H
#define LEXER_H
#include <stdio.h>
#include <stdbool.h>

typedef enum {
    TK_EOF = 0,
    TK_LETTER,   // payload: single char 'A'..'Z','0'..'9'
    TK_SLASH,    // '/'
    TK_EQ,       // '='
    TK_PLUS,     // '+'
    TK_MINUS,    // '-'
    TK_SEMI,     // ';'
    TK_NEWLINE,  // '\n' (문장 경계 힌트용. 파서는 무시 가능)
} TokenKind;

typedef struct {
    TokenKind kind;
    char ch;           // TK_LETTER 일 때만 유효
    int line, col;     // 토큰 시작 위치
} Token;

typedef struct {
    const char* filename;
    FILE* fp;
    int line, col;
    int cur;           // current char (lookahead)
} Lexer;

bool lx_open(Lexer* lx, const char* filename);
void lx_close(Lexer* lx);
Token lx_next(Lexer* lx);  // 다음 토큰

#endif