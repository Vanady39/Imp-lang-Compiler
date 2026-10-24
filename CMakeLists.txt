cmake_minimum_required(VERSION 3.20)

project(compilers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lexer
    lexer/lexer.cpp
    lexer/reader.cpp
    lexer/token_name_map.cpp
)

target_include_directories(lexer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(parser
    parser/parser.cpp
    parser/parser_stmt.cpp
    parser/parser_expr.cpp
)

target_link_libraries(parser
    PUBLIC
        lexer
)

add_executable(compiler
    main.cpp
    runners/runner.cpp
    runners/ast_printers/ast_printer.cpp
)

target_link_libraries(compiler
    PRIVATE
        parser
)
