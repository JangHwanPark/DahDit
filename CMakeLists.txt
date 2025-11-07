cmake_minimum_required(VERSION 3.26)
project(DahDit C)
set(CMAKE_C_STANDARD 11)

# include 폴더 등록
include_directories(${CMAKE_SOURCE_DIR}/include)

# 실행 파일 생성 (모든 c 파일 포함)
add_executable(dahdit
        src/main.c
        src/diag.c
        include/lexer.h
        include/parser.h
        include/interp.h
        include/symtab.h
        src/interp.c
        src/parser.c
        src/symtab.c
        src/lexer.c
)
