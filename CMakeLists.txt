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

add_executable(lexer_build
    main.cpp
)

target_link_libraries(lexer_build
    PRIVATE
        lexer
)
