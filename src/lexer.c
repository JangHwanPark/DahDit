//========================================
// System Includes
//========================================
#include "lexer.h"
#include "morse_table.h"
#include "diag.h"
#include <ctype.h>
#include <string.h>

/**
 * @brief 파일 포인터에서 다음 문자를 읽고 위치 정보를 업데이트
 */
static int nextc(Lexer* lx) {
    int c = fgetc(lx->fp);
    if (c == '\n') { lx->line++; lx->col = 1; }
    else if (c != EOF) { lx->col++; }
    return c;
}

/**
 * @brief Lexer를 초기화하고 입력 파일오픈
 * @return 성공 시 true, 실패 시 false.
 */
bool lx_open(Lexer* lx, const char* filename) {
    lx->filename = filename;
    lx->fp = fopen(filename, "r");
    if (!lx->fp) return false;
    lx->line = 1; lx->col = 1;
    lx->cur = nextc(lx);
    return true;
}

/**
 * @brief Lexer 사용을 마친 후 파일을 종료
 */
void lx_close(Lexer* lx) {
    if (lx->fp) fclose(lx->fp);
    lx->fp = NULL;
}

/**
 * @brief 공백 문자(' ', '\t', '\r')와 주석('#'으로 시작하여 개행까지)을 스킵
 */
static void skip_ws_and_comments(Lexer* lx) {
    for (;;) {
        // 공백류 스킵
        while (lx->cur == ' ' || lx->cur == '\t' || lx->cur == '\r') {
            lx->cur = nextc(lx);
        }
        // 주석: # ~ 라인 끝
        if (lx->cur == '#') {
            while (lx->cur != '\n' && lx->cur != EOF) lx->cur = nextc(lx);
        } else break;
    }
}

/**
 * @brief 모스 부호 시퀀스(code)를 해당하는 단일 문자(char)로 디코딩
 */
static char decode_morse(const char* code) {
    for (int i = 0; i < MORSE_TABLE_LEN; ++i) {
        if (strcmp(MORSE_TABLE[i].code, code) == 0) return MORSE_TABLE[i].ch;
    }
    return '\0';
}

Token lx_next(Lexer* lx) {
    skip_ws_and_comments(lx);

    Token tok = { .kind = TK_EOF, .ch = 0, .line = lx->line, .col = lx->col };

    if (lx->cur == EOF) { tok.kind = TK_EOF; return tok; }

    //========================================
    // 문자열 리터럴 인식 (TK_STRING)
    //========================================
    if (lx->cur == '"') {
        tok.kind = TK_STRING;
        int n = 0;
        lx->cur = nextc(lx);
        while (lx->cur != '"' && lx->cur != '\n' && lx->cur != EOF) {
            if (n < (int)sizeof(tok.text)-1) tok.text[n++] = (char)lx->cur;
            lx->cur = nextc(lx);
        }
        tok.text[n] = '\0';
        if (lx->cur != '"') {
            diag_error(lx->filename, tok.line, tok.col, "unterminated string literal");
        } else {
            lx->cur = nextc(lx);
        }
        return tok;
    }

    //========================================
    // 단일 문자 / 구분자 인식
    //========================================
    if (lx->cur == '\n') { lx->cur = nextc(lx); tok.kind = TK_NEWLINE; return tok; }
    if (lx->cur == '/') { lx->cur = nextc(lx); tok.kind = TK_SLASH; return tok; }
    if (lx->cur == '=') { lx->cur = nextc(lx); tok.kind = TK_EQ; return tok; }
    if (lx->cur == ';') { lx->cur = nextc(lx); tok.kind = TK_SEMI; return tok; }

    //========================================
    // 모스 부호 인식 및 디코딩
    //========================================
    if (lx->cur == '.' || lx->cur == '-') {
        char buf[16]; int n = 0;
        while ((lx->cur == '.' || lx->cur == '-') && n < (int)sizeof(buf)-1) {
            buf[n++] = (char)lx->cur;
            lx->cur = nextc(lx);
        }

        buf[n] = '\0';
        char ch = decode_morse(buf);

        if (ch == '\0') {
            char msg[64];
            snprintf(msg, sizeof(msg), "unknown morse sequence '%s'", buf);
            diag_error(lx->filename, tok.line, tok.col, msg);
            // 에러 토큰 대신, 진행을 위해 TK_LETTER('?')
            tok.kind = TK_LETTER; tok.ch = '?';
            return tok;
        }

        if (ch == '+') {
            tok.kind = TK_PLUS;
        } else if (ch == '-') {
            tok.kind = TK_MINUS;
        } else if (ch == '*') {
            tok.kind = TK_STAR;
        } else if (ch == '%') {
            tok.kind = TK_PERCENT;
        } else if (ch == '/') {
            //========================================
            // '/'는 TK_SLASH 토큰으로 처리되어야 하지만,
            // 모스 부호 테이블에서는 '/'를 나눗셈으로 사용할 수 있으므로,
            // 현재는 TK_LETTER로 처리하거나, 별도 TK_DIV 토큰으로 분류 필요.
            // (현재 TK_DIV 토큰은 없으므로 TK_LETTER로 처리 후 Parser가 처리하게 둠)
            // 일단 일반 문자로 분류 후 Parser가 처리하도록 함
            //========================================
            tok.kind = TK_LETTER;
            tok.ch = ch;
        } else {
            //========================================
            // 나머지 문자 (A-Z, 0-9, 한글 초성 매핑 문자 등)
            //========================================
            tok.kind = TK_LETTER;
            tok.ch = ch;
        }
        return tok;
    }

    //========================================
    // 알 수 없는 문자 처리 → 무시하고 계속
    //========================================
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "unexpected character '%c'", lx->cur);
        diag_error(lx->filename, tok.line, tok.col, msg);
        lx->cur = nextc(lx);
        return lx_next(lx);
    }
}
