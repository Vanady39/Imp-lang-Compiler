cmake_minimum_required(VERSION 3.20)

project(compilers LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)
if(LLVM_PACKAGE_VERSION VERSION_LESS 15)
    message(FATAL_ERROR "LLVM 15 or newer is required for opaque pointers")
endif()

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
    semantic/traversal/ast_traversal.cpp
    semantic/optimizer/optimizer.cpp
    semantic/optimizer/expression_optimizer/expression_optimizer.cpp
    semantic/optimizer/body_optimizer/body_optimizer.cpp
    semantic/optimizer/node_optimizer/node_optimizer.cpp
    semantic/checker/checker.cpp
    semantic/checker/condition/condition.cpp
    semantic/checker/for-loop/for.cpp
    semantic/checker/routine/routine.cpp
    semantic/checker/return/return.cpp
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
    codegen/bitcode.cpp
    codegen/entry.cpp
    codegen/expression.cpp
    codegen/function.cpp
    codegen/generator.cpp
    codegen/statement.cpp
    runners/runner.cpp
    runners/ast_printers/ast_printer.cpp
)

target_link_libraries(compiler
    PRIVATE
        parser
        semantic
)
target_include_directories(compiler SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
if(TARGET LLVM)
    target_link_libraries(compiler PRIVATE LLVM)
elseif(TARGET LLVM-C)
    target_link_libraries(compiler PRIVATE LLVM-C)
else()
    llvm_map_components_to_libnames(LLVM_LIBRARIES analysis bitwriter core irreader)
    target_link_libraries(compiler PRIVATE ${LLVM_LIBRARIES})
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(semantic_base_tests tests/semantic_base.cpp)
    target_link_libraries(semantic_base_tests PRIVATE semantic)
    add_test(NAME semantic_base COMMAND semantic_base_tests)

    add_test(
        NAME golden_tests
        COMMAND "${CMAKE_COMMAND}" "-DCOMPILER=$<TARGET_FILE:compiler>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_golden.cmake"
    )

    find_program(LLVM_LLC NAMES llc-${LLVM_VERSION_MAJOR} llc HINTS "${LLVM_TOOLS_BINARY_DIR}")
    if(LLVM_LLC AND NOT MSVC AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_test(
            NAME llvm_codegen
            COMMAND "${CMAKE_COMMAND}" "-DCOMPILER=$<TARGET_FILE:compiler>"
                    "-DLLC=${LLVM_LLC}" "-DLINKER=${CMAKE_CXX_COMPILER}"
                    "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/native-tests"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/check_codegen.cmake"
        )
    endif()
endif()
