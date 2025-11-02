#ifndef CONFIG_H
#define CONFIG_H

// 제어 토큰 (NORMAL 모드에서만 의미)
#define CTRL_PRINT_ON_DOTS   5   // "....."
#define CTRL_PRINT_OFF_DASH  6   // "------"
#define CTRL_EXIT_DOTS       7   // "......."

// 모스 간격(공백 수)
#define GAP_ELEM   1  // 요소 간
#define GAP_LETTER 3  // 글자 간
#define GAP_WORD   7  // 단어 간

// 버퍼 크기(필요시 조절)
#define LETTER_MAX  16
#define OUTBUF_MAX  8192

#endif