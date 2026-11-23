cmake_minimum_required(VERSION 3.20)

file(MAKE_DIRECTORY "${WORK_DIR}")

function(build_native source name)
    set(bitcode "${WORK_DIR}/${name}.bc")
    set(object "${WORK_DIR}/${name}.o")
    set(native "${WORK_DIR}/${name}.exe")
    execute_process(COMMAND "${COMPILER}" "${CMAKE_CURRENT_LIST_DIR}/${source}" -o "${bitcode}"
                    COMMAND_ERROR_IS_FATAL ANY)
    file(READ "${bitcode}" magic LIMIT 4 HEX)
    if(NOT magic STREQUAL "4243c0de")
        message(FATAL_ERROR "${bitcode} is not LLVM bitcode")
    endif()
    execute_process(COMMAND "${LLC}" -filetype=obj -relocation-model=pic "${bitcode}" -o "${object}"
                    COMMAND_ERROR_IS_FATAL ANY)
    execute_process(COMMAND "${LINKER}" "${object}" -o "${native}" -lm
                    COMMAND_ERROR_IS_FATAL ANY)
endfunction()

function(check_output name expected)
    execute_process(COMMAND "${WORK_DIR}/${name}.exe" ${ARGN}
                    OUTPUT_VARIABLE actual OUTPUT_STRIP_TRAILING_WHITESPACE
                    COMMAND_ERROR_IS_FATAL ANY)
    string(REPLACE "\r\n" "\n" actual "${actual}")
    if(NOT actual STREQUAL expected)
        message(FATAL_ERROR "${name}\nExpected:\n${expected}\nActual:\n${actual}")
    endif()
endfunction()

function(check_program name expected)
    build_native("positive/${name}.txt" "${name}")
    check_output("${name}" "${expected}")
endfunction()

check_program(01_variables "2\n5")
check_program(02_conversion "4")
check_program(03_if_else "1")
check_program(04_loops "0\n1\n1\n2\n3")
check_program(05_aggregates "5\n7")
check_program(06_routine_result "5")
check_program(08_nested_records "7 7")
check_program(09_nested_arrays_records "7\n9")
check_program(10_aggregate_routines "2 3\n8 9")
check_program(11_array_parameters_and_ranges "2\n4\n4\n2\n2\n1")
check_program(12_scope_and_boolean "1\n0\n2\n10\n20 1")
check_program(13_forward_recursion "6")

build_native(codegen.imp codegen)
check_output(codegen "30\n10\n3 3\n8 9\n1 1 1\n1\n42 3\n2\n1.75")
check_output(codegen "3 2.5 1" demo 3 2.5 1)

execute_process(
    COMMAND "${COMPILER}" "${CMAKE_CURRENT_LIST_DIR}/codegen.imp" --emit-llvm -o "${WORK_DIR}/codegen.ll"
    COMMAND_ERROR_IS_FATAL ANY
)
file(READ "${WORK_DIR}/codegen.ll" ir)
if(NOT ir MATCHES "define i32 @main")
    message(FATAL_ERROR "Textual LLVM IR is missing its entry point")
endif()

function(check_rejected source diagnostic_pattern)
    set(output "${WORK_DIR}/invalid.bc")
    execute_process(COMMAND "${COMPILER}" "${source}" -o "${output}"
                    RESULT_VARIABLE status OUTPUT_VARIABLE diagnostic)
    if(NOT status STREQUAL "1" OR EXISTS "${output}" OR NOT diagnostic MATCHES "${diagnostic_pattern}")
        message(FATAL_ERROR "Expected a semantic error and no output for ${source}\n${diagnostic}")
    endif()
endfunction()

file(GLOB invalid_sources "${CMAKE_CURRENT_LIST_DIR}/negative/*.txt")
foreach(source IN LISTS invalid_sources)
    check_rejected("${source}" "Semantic error")
endforeach()

set(dynamic_array "${WORK_DIR}/dynamic-array.imp")
file(WRITE "${dynamic_array}" [=[
routine test(n : integer) is
    var values : array [n] integer
end
]=])
check_rejected("${dynamic_array}" "array size must be a non-negative integer constant")

set(missing_definition "${WORK_DIR}/missing-definition.imp")
file(WRITE "${missing_definition}" [=[
routine pending(x : integer) : integer
]=])
check_rejected("${missing_definition}" "routine 'pending' has no definition")

set(wrong_return "${WORK_DIR}/wrong-return.imp")
file(WRITE "${wrong_return}" [=[
routine bad(value : real) : boolean => value
]=])
check_rejected("${wrong_return}" "return value has incompatible type")

set(array_type_mismatch "${WORK_DIR}/array-type-mismatch.imp")
file(WRITE "${array_type_mismatch}" [=[
type First is array [2] integer
type Second is array [2] integer
routine test() is
    var first : First
    var second : Second
    first := second
end
]=])
check_rejected("${array_type_mismatch}" "cannot assign value of type 'array'")

set(array_length_assignment "${WORK_DIR}/array-length-assignment.imp")
file(WRITE "${array_length_assignment}" [=[
routine test() is
    var values : array [1] integer
    values.length := 2
end
]=])
check_rejected("${array_length_assignment}" "array length is read-only")
