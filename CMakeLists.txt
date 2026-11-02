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

add_library(semantic
    semantic/common/helpers/expression_type.cpp
    semantic/semantic-analyzer.cpp
    semantic/traversal/ast_traversal.cpp
    semantic/checker/checker.cpp
    semantic/checker/condition/condition.cpp
    semantic/checker/for-loop/for.cpp
    semantic/checker/routine/routine.cpp
    semantic/checker/variable/variables.cpp
    semantic/checker/while/while.cpp
)

target_link_libraries(semantic
    PRIVATE
        parser
)

target_include_directories(semantic
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
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
        semantic
)

include(CTest)
if(BUILD_TESTING)
    add_executable(semantic_base_tests tests/semantic_base.cpp)
    target_link_libraries(semantic_base_tests PRIVATE semantic)
    add_test(NAME semantic_base COMMAND semantic_base_tests)
endif()
