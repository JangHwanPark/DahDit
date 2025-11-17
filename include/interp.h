#ifndef INTERP_H
#define INTERP_H
#pragma once
//========================================
// System Includes
//========================================
#include <stdbool.h>

//========================================
// 파일에서 Dashdit 프로그램을 로드, 파싱 및 실행
//========================================
bool run_program(const char* filename);

#endif