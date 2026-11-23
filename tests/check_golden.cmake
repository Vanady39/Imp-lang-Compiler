cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE sources RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
     "${CMAKE_CURRENT_LIST_DIR}/positive/*.txt" "${CMAKE_CURRENT_LIST_DIR}/negative/*.txt")
foreach(source IN LISTS sources)
    execute_process(
        COMMAND "${COMPILER}" "${CMAKE_CURRENT_LIST_DIR}/${source}" --dump-ast
        RESULT_VARIABLE status
        OUTPUT_VARIABLE actual
        ERROR_VARIABLE actual
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX REPLACE "\\.txt$" ".golden" golden "${source}")
    file(READ "${CMAKE_CURRENT_LIST_DIR}/golden/${golden}" expected)
    string(REGEX REPLACE "[\r\n]+$" "" expected "${expected}")
    string(REPLACE "\r\n" "\n" expected "${expected}")
    string(REPLACE "\r\n" "\n" actual "${actual}")
    if(NOT "=== exit: ${status} ===\n${actual}" STREQUAL expected)
        message(FATAL_ERROR "Golden mismatch for ${source}\nExpected:\n${expected}\nActual:\n=== exit: ${status} ===\n${actual}")
    endif()
endforeach()
list(LENGTH sources count)
message(STATUS "${count} golden tests passed")
