//========================================
// System Includes
//========================================
#include "parser.h"
#include "diag.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//========================================
// Utility Functions
//========================================

/**
 * @brief 현재 토큰을 한 칸 앞으로 이동
 */
static void advance(Parser* ps) { ps->cur = lx_next(ps->lx); }

/**
 * @brief 모스 부호 디코딩으로 얻은 연속된 문자를 하나의 단어(식별자/숫자열)로 만듬
 * @return 단어가 하나라도 있으면 true, 아니면 false.
 */
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

/**
 * @brief 문장 사이의 구분자(개행, 슬래시)를 스킵
 */
static void skip_separators(Parser* ps) {
    while (ps->cur.kind == TK_NEWLINE || ps->cur.kind == TK_SLASH) advance(ps);
}

/**
 * @brief 현재 단어가 키워드(kw)와 일치하는지 확인
 */
static bool is_kw(const char* w, const char* kw) {
    return strcmp(w, kw) == 0;
}


//========================================
// Initialization
//========================================

/**
 * @brief Parser를 초기화 및 첫 번째 토큰을 읽어옴
 */
void ps_init(Parser* ps, Lexer* lx) {
    ps->lx = lx;
    ps->cur = lx_next(lx);
}


//========================================
// Expression Building Helpers
//========================================
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


//========================================
// Expression Parsing Levels
//========================================

/**
 * @brief 가장 낮은 우선순위: 숫자 리터럴 또는 변수(식별자)를 파싱
 * @return 성공 시 true, 실패 시 false.
 */
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

/**
 * @brief 중간 우선순위: 곱셈('*'), 나머지('%') 연산을 처리(Term)
 *
 * 파서에게 RPN 스타일로 연산자를 피연산자 뒤에 배치하도록 지시
 */
static bool parse_term(Parser* ps, Expr* expr) {
    // Expression은 Term으로 시작 (Term은 *, % 처리를 포함)
    if (!parse_factor(ps, expr)) return false;

    for (;;) {
        skip_separators(ps);
        if (ps->cur.kind == TK_STAR || ps->cur.kind == TK_PERCENT) {
            TokenKind op_kind = ps->cur.kind;
            advance(ps);

            // 다음 Term을 파싱하고 푸시 (수정된 부분)
            if (!parse_factor(ps, expr)) return false;

            // 연산자 푸시 (RPN 스타일)
            ExprOp op;
            if (op_kind == TK_STAR) op = EXPR_OP_MUL;
            else op = EXPR_OP_MOD;

            if (!expr_push_op(ps, expr, op)) return false;
        } else {
            break;
        }
    }
    return true;
}

/**
 * @brief 가장 낮은 우선순위: 덧셈('+'), 뺄셈('-') 연산을 처리(Expression)
 * Term을 기반으로 연산을 수행
 */
static bool parse_expr(Parser* ps, Expr* expr) {
    expr->count = 0;
    if (!parse_term(ps, expr)) return false;

    for (;;) {
        skip_separators(ps);
        if (ps->cur.kind == TK_PLUS || ps->cur.kind == TK_MINUS) {
            TokenKind op_kind = ps->cur.kind;
            advance(ps);
            if (!parse_term(ps, expr)) return false;
            ExprOp op = (op_kind == TK_PLUS) ? EXPR_OP_ADD : EXPR_OP_SUB;
            if (!expr_push_op(ps, expr, op)) return false;
        } else {
            break;
        }
    }
    return true;
}


//========================================
// Statement Parsers
//========================================

/**
 * @brief PRINT <expr | string> ; 문장을 파싱
 */
static bool parse_print(Parser* ps, Stmt* out) {
    out->line = ps->cur.line; out->col = ps->cur.col;
    skip_separators(ps);

    // 문자열 출력 (STMT_PRINT_STR - TK_STRING 토큰 사용)
    if (ps->cur.kind == TK_STRING) {
        out->kind = STMT_PRINT_STR;
        strncpy(out->printStrStmt.text, ps->cur.text, sizeof(out->printStrStmt.text));
        out->printStrStmt.text[sizeof(out->printStrStmt.text)-1] = '\0';
        advance(ps);
    }

    else {
        out->kind = STMT_PRINT;
        if (!parse_expr(ps, &out->printStmt.expr)) return false;

        skip_separators(ps);

        // PRINT 뒤에 여러 단어(예: HELLO WORLD)가 오는 경우
        // 기존 표현식 파싱은 첫 단어만 소비하고 세미콜론을 기대하므로 오류가 발생함.
        // 표현식이 단일 토큰이며, 이후에 단어/슬래시가 더 이어지면 문자열 출력으로 간주.
        if (ps->cur.kind != TK_SEMI &&
            out->printStmt.expr.count == 1 &&
            (ps->cur.kind == TK_LETTER || ps->cur.kind == TK_SLASH || ps->cur.kind == TK_NEWLINE)) {

            char buffer[sizeof(out->printStrStmt.text)] = {0};

            // 1) 표현식의 첫 항을 문자열로 복원
            ExprItem first = out->printStmt.expr.items[0];
            if (first.kind == EXPR_ITEM_NUMBER) {
                snprintf(buffer, sizeof(buffer), "%d", first.as.number);
            } else if (first.kind == EXPR_ITEM_VAR) {
                strncpy(buffer, first.as.var, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';
            }

            // 2) 남은 토큰을 세미콜론 전까지 이어 붙이며 공백을 삽입
            bool pending_space = buffer[0] != '\0';
            while (ps->cur.kind != TK_SEMI && ps->cur.kind != TK_EOF) {
                size_t len = strlen(buffer);

                if (pending_space && len > 0 && buffer[len - 1] != ' ' && ps->cur.kind == TK_LETTER) {
                    if (len + 1 < sizeof(buffer)) {
                        buffer[len] = ' ';
                        buffer[len + 1] = '\0';
                        len++;
                    }
                    pending_space = false;
                }

                if (ps->cur.kind == TK_LETTER) {
                    if (len + 1 < sizeof(buffer)) {
                        buffer[len] = ps->cur.ch;
                        buffer[len + 1] = '\0';
                    }
                } else if (ps->cur.kind == TK_SLASH || ps->cur.kind == TK_NEWLINE) {
                    pending_space = buffer[0] != '\0';
                } else {
                    // 예상치 못한 토큰이 나오면 중단 (이후에서 오류 처리)
                    break;
                }
                advance(ps);
            }

            if (ps->cur.kind != TK_SEMI) {
                diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "missing ';' after PRINT");
                return false;
            }

            // 문자열 출력으로 전환
            out->kind = STMT_PRINT_STR;
            strncpy(out->printStrStmt.text, buffer, sizeof(out->printStrStmt.text));
            out->printStrStmt.text[sizeof(out->printStrStmt.text) - 1] = '\0';
        }
    }

    skip_separators(ps);
    if (ps->cur.kind != TK_SEMI) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "missing ';' after PRINT");
        return false;
    }

    advance(ps);
    return true;
}

/**
 * @brief VAR name (= expr)? ; 문장을 파싱
 */
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

    // 변수 이름 파싱
    if (!parse_word(ps, out->varStmt.name, sizeof(out->varStmt.name))) return false;

    skip_separators(ps);

    // 할당 연산자 '=' 파싱(선택)
    if (ps->cur.kind == TK_EQ) {
        advance(ps);
        if (!parse_expr(ps, &out->varStmt.value_expr)) return false;
        out->varStmt.has_value = true;
    }

    skip_separators(ps);

    // 구문 종결자 ';' 파싱
    if (ps->cur.kind != TK_SEMI) {
        diag_error(ps->lx->filename, ps->cur.line, ps->cur.col, "missing ';' after VAR statement");
        return false;
    }
    advance(ps);
    return true;
}


//========================================
// Main Parsing Loop
//========================================

/**
 * @brief 다음 문장을 파싱하고 해당 Stmt 구조체 삽입
 * @return EOF가 아니면 true, 파일 끝이면 false.
 */
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
