#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

// 공백(스페이스/탭/개행 등) 런 길이
int run_spaces(const char** pp);
// 점('.') 런 길이
int run_dots(const char** pp);
// 대시('-') 런 길이
int run_dashes(const char** pp);

// 파일 전체를 읽어 NUL-종료 버퍼로 반환 (성공: buf, 실패: NULL)
// *size_out이 있으면 파일 크기 반환
char* slurp_file(const char* path, size_t* size_out);

#endif
