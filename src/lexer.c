#include "lexer.h"
#include "morse_table.h"
#include "diag.h"
#include <ctype.h>
#include <string.h>

static int nextc(Lexer* lx) {
    int c = fgetc(lx->fp);
    if (c == '\n') { lx->line++; lx->col = 1; }
    else if (c != EOF) { lx->col++; }
    return c;
}

bool lx_open(Lexer* lx, const char* filename) {
    lx->filename = filename;
    lx->fp = fopen(filename, "r");
    if (!lx->fp) return false;
    lx->line = 1; lx->col = 1;
    lx->cur = nextc(lx);
    return true;
}

void lx_close(Lexer* lx) {
    if (lx->fp) fclose(lx->fp);
    lx->fp = NULL;
}

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
    if (lx->cur == '\n') { lx->cur = nextc(lx); tok.kind = TK_NEWLINE; return tok; }
    if (lx->cur == '/') { lx->cur = nextc(lx); tok.kind = TK_SLASH; return tok; }
    if (lx->cur == '=') { lx->cur = nextc(lx); tok.kind = TK_EQ; return tok; }
    if (lx->cur == ';') { lx->cur = nextc(lx); tok.kind = TK_SEMI; return tok; }

    // Morse letter: a run of '.' and '-'
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
        tok.kind = TK_LETTER;
        tok.ch = ch;
        return tok;
    }

    // 알 수 없는 문자 → 무시하고 계속
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "unexpected character '%c'", lx->cur);
        diag_error(lx->filename, tok.line, tok.col, msg);
        lx->cur = nextc(lx);
        return lx_next(lx);
    }
}
