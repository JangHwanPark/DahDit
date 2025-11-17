//========================================
// System Includes
//========================================
#include "interp.h"
#include "lexer.h"
#include "diag.h"
#include <stdio.h>
#include <string.h>

#include "parser.h"
#include "symtab.h"


/**
 * @brief 표현식(Expr)을 스택 기반으로 평가하고 최종 정수 값을 반환.(스택을 사용하여 계산)
 *
 * Dashdit 파서가 생성한 표현식 항목(ExprItem)은 피연산자가 연산자보다 앞서는 형태로
 * 암묵적인 후위 표기법(RPN)처럼 동작.
 *
 * @param expr 평가할 표현식 구조체.
 * @param st 변수 조회를 위한 심볼 테이블.
 * @param filename 오류 보고를 위한 파일 이름.
 * @param line, col 오류 보고를 위한 위치 정보.
 * @param out 최종 평가된 정수 값을 저장할 포인터.
 * @return 평가 성공 시 true, 오류 발생 시 false (단, 호출자가 오류를 처리하고 진행할 수 있음).
 */
static bool eval_expr(const Expr* expr, SymTab* st, const char* filename, int line, int col, int32_t* out) {
    int32_t stack[MAX_EXPR_ITEMS];
    int sp = 0;

    for (int i = 0; i < expr->count; ++i) {
        ExprItem item = expr->items[i];
        switch (item.kind) {
            case EXPR_ITEM_NUMBER:
                if (sp >= MAX_EXPR_ITEMS) {
                    diag_error(filename, line, col, "Expression stack overflow (number)");
                    return false;
                }
                stack[sp++] = item.as.number;
                break;
            case EXPR_ITEM_VAR: {
                int32_t value;
                if (!st_get(st, item.as.var, &value)) {
                    char msg[96];
                    snprintf(msg, sizeof(msg), "undefined variable '%s'", item.as.var);
                    diag_error(filename, line, col, msg);
                    return false;
                }
                if (sp >= MAX_EXPR_ITEMS) {
                    diag_error(filename, line, col, "Expression stack overflow (variable)");
                    return false;
                }
                stack[sp++] = value;
                break;
            }
            case EXPR_ITEM_OP: {
                if (sp < 2) {
                    diag_error(filename, line, col, "not enough operands for operator");
                    return false;
                }
                int32_t rhs = stack[--sp];
                int32_t lhs = stack[--sp];
                int32_t result = 0;

                switch (item.as.op) {
                    case EXPR_OP_ADD:
                        result = lhs + rhs;
                        break;
                    case EXPR_OP_SUB:
                        result = lhs - rhs;
                        break;
                    case EXPR_OP_MUL: // Added Multiplication
                        result = lhs * rhs;
                        break;
                    case EXPR_OP_DIV: // Added Division
                        if (rhs == 0) {
                            diag_error(filename, line, col, "Division by zero");
                            return false;
                        }
                        result = lhs / rhs;
                        break;
                    case EXPR_OP_MOD: // Added Modulo
                        if (rhs == 0) {
                            diag_error(filename, line, col, "Modulo by zero");
                            return false;
                        }
                        result = lhs % rhs;
                        break;
                    default:
                        diag_error(filename, line, col, "Unknown operator in expression");
                        return false;
                }
                stack[sp++] = result;
                break;
            }
        }
    }

    if (sp != 1) {
        diag_error(filename, line, col, "expression did not reduce to a value");
        return false;
    }

    *out = stack[0];
    return true;
}

/**
 * @brief 단일 문장(Statement)의 실행을 처리.
 * run_program 함수의 중첩 깊이를 줄이기 위해 분리.
 *
 * @param s 실행할 문장 구조체.
 * @param st 심볼 테이블.
 * @param filename 오류 보고를 위한 파일 이름.
 * @return 치명적인 오류가 발생하지 않았으면 true (다음 문장으로 계속 진행).
 */
static bool handle_statement(const Stmt* s, SymTab* st, const char* filename) {
    switch (s->kind) {
        case STMT_PRINT: {
            int32_t value;
            if (!eval_expr(&s->printStmt.expr, st, filename, s->line, s->col, &value)) {
                return true;
            }
            printf("%d\n", value);
            break;
        }

        case STMT_PRINT_STR: {
            printf("%s\n", s->printStrStmt.text);
            // (미구현) 향후 문자열 출력 시 사용될 예정
            // 현재는 TK_STRING 토큰을 Lexer/Parser에서 건네주지 않으므로 실행되지 않음.
            // diag_error(filename, s->line, s->col, "String output is not supported yet");
            break;
        }

        case STMT_VAR: {
            if (!s->varStmt.has_value) {
                diag_error(filename, s->line, s->col, "VAR without initializer is not supported yet");
                return true;
            }
            int32_t value;
            if (!eval_expr(&s->varStmt.value_expr, st, filename, s->line, s->col, &value)) {
                return true;
            }
            if (!st_set(st, s->varStmt.name, value)) {
                diag_error(filename, s->line, s->col, "Symbol table full");
                return true;
            }
            break;
        }

        default:
            diag_error(filename, s->line, s->col, "Internal error: Unknown statement kind");
            break;
    }
    return true;
}

/**
 * @brief Dashdit 프로그램을 로드, 파싱 및 실행
 *
 * @param filename .dit 파일 경로.
 * @return 프로그램이 성공적으로 실행되었거나 (비치명적인 오류 포함), 파일을 열 수 없으면 false.
 */
bool run_program(const char* filename) {
    Lexer lx;
    if (!lx_open(&lx, filename)) {
        fprintf(stderr, "Cannot open: %s\n", filename);
        return false;
    }

    Parser ps; ps_init(&ps, &lx);
    SymTab st; st_init(&st);
    Stmt s;

    // 다음 문장을 파싱하고, handle_statement를 호출
    while (ps_next_stmt(&ps, &s)) {
        handle_statement(&s, &st, lx.filename);
    }

    lx_close(&lx);
    return true;
}