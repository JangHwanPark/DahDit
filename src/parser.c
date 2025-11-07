#include "parser.h"
#include "diag.h"
#include <string.h>
#include <ctype.h>

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

static void skip_newlines(Parser* ps) {
    while (ps->cur.kind == TK_NEWLINE) advance(ps);
}

static bool is_kw(const char* w, const char* kw) {
    return strcmp(w, kw) == 0;
}

void ps_init(Parser* ps, Lexer* lx) {
    ps->lx = lx;
    ps->cur = lx_next(lx);
}

// PRINT <words or var> ;
static bool parse_print(Parser* ps, Stmt* out) {
    out->kind = STMT_PRINT;
    out->printStmt.is_var = false;
    out->printStmt.literal_word_count = 0;
    out->line = ps->cur.line; out->col = ps->cur.col;

    // 다음 토큰부터 ';' 전까지를 수집
    // 단어는 LETTER들의 모음, 단어 사이 '/'가 나오면 공백으로 이어짐
    // 케이스1: 첫 단어가 변수명이고 그 다음이 바로 ';'라면 변수 출력
    // 케이스2: 하나 이상의 단어/슬래시가 있으면 리터럴 출력
    char first[64] = {0};

    // 최소 한 단어 필요
    if (ps->cur.kind != TK_LETTER) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expected letters after PRINT");
        return false;
    }
    // 첫 단어
    if (!parse_word(ps, first, sizeof(first))) return false;

    // 이어지는 부분을 살핀다
    int wc = 0;
    int pending_space = 0; // '/'를 만나면 다음 단어 앞에 공백 의미
    while (ps->cur.kind != TK_SEMI && ps->cur.kind != TK_EOF) {
        if (ps->cur.kind == TK_SLASH) {
            pending_space = 1; advance(ps); continue;
        } else if (ps->cur.kind == TK_LETTER) {
            // 새 단어 시작
            char w[64] = {0};
            if (!parse_word(ps, w, sizeof(w))) break;
            if (wc < 16) {
                if (pending_space && wc > 0) {
                    // 이전 단어와의 구분은 출력 단계에서 공백으로 처리
                }
                strncpy(out->printStmt.literal_words[wc], w, 63);
                out->printStmt.literal_words[wc][63] = '\0';
                wc++;
                pending_space = 0;
            }
        } else if (ps->cur.kind == TK_NEWLINE) {
            advance(ps);
        } else if (ps->cur.kind == TK_EQ) {
            diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "unexpected '=' in PRINT");
            return false;
        } else {
            // 기타 토큰 만나면 루프 종료
            break;
        }
    }

    // 만약 추가 단어가 하나도 없었고, 바로 ';'이면 → 변수 출력으로 간주
    if (wc == 0) {
        out->printStmt.is_var = true;
        strncpy(out->printStmt.varName, first, 63);
    } else {
        // literal 모드: 첫 단어를 배열 맨 앞에 넣기
        out->printStmt.is_var = false;
        strncpy(out->printStmt.literal_words[0], first, 63);
        wc += 1;
    }
    out->printStmt.literal_word_count = wc == 0 ? 0 : wc;

    // 세미콜론 필수
    if (ps->cur.kind != TK_SEMI) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "missing ';' after PRINT");
        return false;
    }
    advance(ps); // eat ';'
    return true;
}

// VAR name (= number)? ;
static bool parse_var(Parser* ps, Stmt* out) {
    out->kind = STMT_VAR;
    out->varStmt.name[0] = '\0';
    out->varStmt.value = 0;
    out->varStmt.has_value = false;
    out->line = ps->cur.line; out->col = ps->cur.col;

    // 식별자
    if (ps->cur.kind != TK_LETTER) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expected identifier after VAR");
        return false;
    }
    if (!parse_word(ps, out->varStmt.name, sizeof(out->varStmt.name))) return false;

    // '='?
    if (ps->cur.kind == TK_EQ) {
        advance(ps);
        // 숫자(연속 LETTER이지만 실제로는 '0'~'9')
        if (ps->cur.kind != TK_LETTER) {
            diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "expected number after '='");
            return false;
        }
        char numbuf[64] = {0};
        if (!parse_word(ps, numbuf, sizeof(numbuf))) return false;

        // 숫자 확인
        for (int i=0; numbuf[i]; ++i) {
            if (!isdigit((unsigned char)numbuf[i])) {
                diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "number must be digits 0-9");
                return false;
            }
        }
        out->varStmt.value = (int32_t)strtol(numbuf, NULL, 10);
        out->varStmt.has_value = true;
    }

    if (ps->cur.kind != TK_SEMI) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "missing ';' after VAR statement");
        return false;
    }
    advance(ps); // eat ';'
    return true;
}

bool ps_next_stmt(Parser* ps, Stmt* out) {
    skip_newlines(ps);
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
