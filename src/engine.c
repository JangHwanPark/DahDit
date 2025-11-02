#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "engine.h"
#include "morse.h"
#include "util.h"
#include "config.h"

typedef enum { MODE_NORMAL, MODE_PRINT } Mode;

static void push_letter(char* out, int* olen, const char* letter) {
    if (letter[0] == '\0') return;
    char ch = morse_decode(letter);
    size_t n = strlen(out);
    (void)n; // not used
    if (*olen < OUTBUF_MAX - 1) {
        out[(*olen)++] = ch;
    }
}

static void flush_out(char* out, int* olen, char* letter, int* llen) {
    if (*llen > 0) {
        letter[*llen] = '\0';
        push_letter(out, olen, letter);
        *llen = 0;
    }
    if (*olen > 0) {
        out[*olen] = '\0';
        printf("%s\n", out);
        *olen = 0;
    }
}

int engine_run_file(const char* path) {
    size_t sz = 0;
    char* buf = slurp_file(path, &sz);
    if (!buf) {
        fprintf(stderr, "failed to read: %s\n", path);
        return 1;
    }

    const char* p = buf;
    Mode mode = MODE_NORMAL;

    char letter[LETTER_MAX]; int llen = 0;    // 현재 글자의 모스 패턴
    char out[OUTBUF_MAX];    int olen = 0;    // 출력 버퍼
    letter[0] = '\0'; out[0] = '\0';

    while (*p) {
        // 1) 공백 런
        int s = run_spaces(&p);
        if (s > 0) {
            if (mode == MODE_PRINT) {
                if (s == GAP_ELEM) {
                    // 요소 간격: noop
                } else if (s == GAP_LETTER) {
                    // 글자 마감
                    if (llen > 0) {
                        letter[llen] = '\0';
                        push_letter(out, &olen, letter);
                        llen = 0;
                    }
                } else if (s == GAP_WORD) {
                    // 단어 마감
                    if (llen > 0) {
                        letter[llen] = '\0';
                        push_letter(out, &olen, letter);
                        llen = 0;
                    }
                    if (olen < OUTBUF_MAX - 1) out[olen++] = ' ';
                } else {
                    // 비표준 간격은 무시(원하면 경고/스냅)
                }
            }
            continue;
        }

        // 2) 점 런
        if (*p == '.') {
            int d = run_dots(&p);

            if (mode == MODE_NORMAL) {
                if (d == CTRL_PRINT_ON_DOTS) {
                    mode = MODE_PRINT;
                } else if (d == CTRL_EXIT_DOTS) {
                    // 종료 (남은 것 정리)
                    if (mode == MODE_PRINT) {
                        flush_out(out, &olen, letter, &llen);
                    }
                    free(buf);
                    return 0;
                } else {
                    // 다른 점 런은 NORMAL에선 의미 없음
                }
            } else { // MODE_PRINT
                // 데이터: dot = '.' 하나
                // 연속 점(d>1)은 요소 사이 공백 없이 쓴 꼴 -> d번 반복으로 관대하게 해석
                for (int i = 0; i < d; ++i) {
                    if (llen < LETTER_MAX - 1) letter[llen++] = '.';
                }
            }
            continue;
        }

        // 3) 대시 런
        if (*p == '-') {
            int d = run_dashes(&p);

            if (mode == MODE_NORMAL) {
                // NORMAL에서 ------(6)은 PRINT_OFF 정의지만,
                // 실사용에선 PRINT 모드에서만 의미 있게 쓰자. 여기선 무시.
            } else { // MODE_PRINT
                if (d == CTRL_PRINT_OFF_DASH) {
                    // 출력 종료: 버퍼 flush, NORMAL 복귀
                    flush_out(out, &olen, letter, &llen);
                    mode = MODE_NORMAL;
                } else {
                    // 데이터: dash = '-' 하나
                    for (int i = 0; i < d; ++i) {
                        if (llen < LETTER_MAX - 1) letter[llen++] = '-';
                    }
                }
            }
            continue;
        }

        // 4) 그 외 문자는 무시
        ++p;
    }

    // 파일 끝 처리
    if (mode == MODE_PRINT) {
        flush_out(out, &olen, letter, &llen);
    }

    free(buf);
    return 0;
}
