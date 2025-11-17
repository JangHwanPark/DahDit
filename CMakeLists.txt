cmake_minimum_required(VERSION 3.26)
project(DahDit C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

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

# include 폴더 등록
# include_directories(${CMAKE_SOURCE_DIR}/include)

# 타깃 기준으로 include 경로 지정 (추천)
target_include_directories(dahdit PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 기본 실행 인자 지정
set(DHDIT_DEFAULT_ARGS "${CMAKE_SOURCE_DIR}/src/tests/hello.dit")
set_target_properties(dahdit PROPERTIES
        VS_DEBUGGER_COMMAND_ARGUMENTS "${DHDIT_DEFAULT_ARGS}"  # CLion도 이 값 사용
)

# (선택) 경고 옵션
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(dahdit PRIVATE -Wall -Wextra -Wpedantic)
endif()