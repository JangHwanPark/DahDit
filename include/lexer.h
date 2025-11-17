#ifndef LEXER_H
#define LEXER_H
//========================================
// System Includes
//========================================
#include <stdio.h>
#include <stdbool.h>

//========================================
// Token Kinds (토큰 종류)
//========================================
typedef enum {
    TK_EOF = 0,

    // 리터럴 및 식별자
    TK_LETTER, // payload: single char 'A'..'Z','0'..'9'
    TK_STRING,

    // 연산자
    TK_EQ,      // '=' (할당)
    TK_PLUS,    // '+' (덧셈)
    TK_MINUS,   // '-' (뺄셈)
    TK_STAR,    // '*' (곱셈)
    TK_DIV,     // '/' (나눗셈)
    TK_PERCENT, // '%' (나머지 연산)

    // 구분자 및 구문 종결자
    TK_SLASH,    // '/' (모스 부호 섹션 구분자)
    TK_SEMI,     // ';' (구문 종결자)
    TK_NEWLINE,  // '\n' (문장 경계 힌트용. 파서는 무시 가능)
} TokenKind;

//========================================
// Token Structure (토큰 구조체)
//========================================
typedef struct {
    TokenKind kind;
    char ch;        // TK_LETTER 일 때만 유효
    char text[128]; // TK_STRING 일 때만 유효 (null-terminated)
    int line, col;  // 토큰 시작 위치
} Token;

//========================================
// Lexer Structure (렉서 상태 구조체)
//========================================
typedef struct {
    const char *filename;
    FILE *fp;
    int line, col;
    int cur; // current char (lookahead)
} Lexer;

//========================================
// Function Prototypes (함수 원형)
//========================================
bool lx_open(Lexer *lx, const char *filename);
void lx_close(Lexer *lx);
Token lx_next(Lexer *lx); // 다음 토큰

#endif
