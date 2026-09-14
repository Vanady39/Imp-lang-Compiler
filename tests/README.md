# Simple test suite

Exactly **15 source files**: 12 positive tests and 3 negative semantic tests.
The first 7 files preserve all 10 original scenarios; 8 additional files extend coverage. Each file is
compiled independently. Reference PDFs are in `documents/`; page numbers below
are PDF page numbers.

## Running and reading the expectations

For every positive file, select entry routine `test` with **no arguments**.
Project I p. 1 allows any routine to be selected at launch; the name `test`
is only our convention. Helper routines are declared before their calls.

The expected outputs below are ordered numeric values across executed
`print` statements (a statement may print several expressions). For example, `1; 2; 3` means three successive prints, not literal
semicolons. Project I p. 5 does not fix output whitespace or line formatting.

The tests use the provisional interpretations in
[DECISIONS.md](../DECISIONS.md): **A1** allows empty declaration parameter lists,
**A2** creates writable array/record objects at variable declaration, and **A3**
returns a value using `return Expression` in a typed routine.
**A4** defines the `//` comments used for expected results.
**A5** provisionally allows sized arrays as arguments to sizeless parameters (test 11).
A1 applies to all files; A2 to tests 05, 08–11, and 14; A3 to tests 06, 07,
10, 13, and 14. Nested aggregate members are assigned explicit objects before access.
These are team assumptions awaiting lecturer/TA confirmation, not additional
official rules. **No file is strictly justified by the PDF alone**, because
all use the empty parameter list under A1. Test 11 additionally depends on the
unresolved sized/sizeless type-compatibility exception. See [AUDIT.md](AUDIT.md)
for a strict per-file review and the limits of validation.

## The original 7 test groups

| File | Original scenarios | Main aspect | Entry routine | Expected output / error |
| --- | --- | --- | --- | --- |
| [01_variables.txt](positive/01_variables.txt) | 1 + 2 | Explicit declaration, assignment, and type inference | `test` | `2; 5` |
| [02_conversion.txt](positive/02_conversion.txt) | 3 | Real-to-integer conversion | `test` | `4` |
| [03_if_else.txt](positive/03_if_else.txt) | 4 | If with both branches; true selects then | `test` | `1` |
| [04_loops.txt](positive/04_loops.txt) | 5 + 6 | While loop followed by inclusive for loop | `test` | `0; 1; 1; 2; 3` |
| [05_aggregates.txt](positive/05_aggregates.txt) | 7 + 8 | Array element and record field access | `test` | `5; 7` |
| [06_routine_result.txt](positive/06_routine_result.txt) | 9 | Parameter passing and returned value | `test` | `5` |
| [07_wrong_argument_count.txt](negative/07_wrong_argument_count.txt) | 10 | Too few arguments | `test` (do not execute) | Semantic error at `add(1)`: expected 2 arguments, received 1 |

The merged scenarios execute sequentially in one entry routine. In group 01,
the inferred variable is named `inferred` to avoid redeclaring `x` in the same
scope. All original declarations, assignments, branches, loops, accesses, calls,
return expressions, and expected-result comments are retained, with that rename.
The negative scenario stays separate so its error does not prevent positive
scenarios from executing.

## Additional coverage

All files use entry `test` without arguments. Commas in source comments and
semicolons below both describe ordered printed values, not required formatting.

| File | Coverage | Expected output / error | Project I basis |
| --- | --- | --- | --- |
| [08_nested_records.txt](positive/08_nested_records.txt) | Named records inside records, chained field access, reference assignment and aliasing | `7; 7` | pp. 2–4, 6 |
| [09_nested_arrays_records.txt](positive/09_nested_arrays_records.txt) | Array of records, record containing an array, array of arrays, shared nested objects | `7; 9` | pp. 2–4, 6 |
| [10_aggregate_routines.txt](positive/10_aggregate_routines.txt) | Record and array parameters, mutation visible to caller, aggregate return values, discarded return value | `2; 3; 8; 9` | pp. 3–6; A3 |
| [11_array_parameters_and_ranges.txt](positive/11_array_parameters_and_ranges.txt) | Sizeless array parameter, array iteration in both directions, reverse numeric loop, bounds evaluated once, empty range, zero-iteration while | `2; 4; 4; 2; 2; 1` | pp. 3–5; A5 (conditional) |
| [12_scope_and_boolean.txt](positive/12_scope_and_boolean.txt) | Global/local/block shadowing, local type alias, false branch, integer/boolean conversions, and/or/xor, parenthesized arithmetic and remainder | `1; 0; 2; 10; 20; 1` | pp. 1–4, 6 |
| [13_forward_recursion.txt](positive/13_forward_recursion.txt) | Forward declaration followed by matching definition, recursive function, return in conditional branches | `6` | pp. 5–6; A3 |
| [14_record_type_mismatch.txt](negative/14_record_type_mismatch.txt) | Nominal record compatibility in function arguments; identical fields do not imply the same type | Semantic error at `readPoint(other)`: expected `Point`, received `OtherPoint` | p. 4 |
| [15_readonly_loop_variable.txt](negative/15_readonly_loop_variable.txt) | Prohibition on assigning the loop variable | Semantic error at `i := 0`: loop variable is read-only | p. 5 |

Each negative file must be rejected during semantic analysis. A syntax error
does not count as the intended result. Exact diagnostic wording is not required.
Each contains one intended semantic violation once its prerequisite syntax
assumptions are accepted; without A1/A3/A4, a strict grammar may reject it earlier.

## Manual consistency review

Expected results are derived by hand. In the while test, `x` is printed as
0 and 1, then becomes 2 and the loop stops. The for test includes both endpoints.
The function `add1` computes `4 + 1`. In test 07, the supplied argument
is an integer; the intended error is the missing second argument.

Every entity is declared before use, including the forward declaration of
`factorial`. Every value read is explicitly initialized or assigned first. All
array indices are within bounds. Nested record/array references are explicitly
assigned before chained access; recursive automatic allocation is not assumed.
Aggregate-return tests return existing caller-owned objects, so they do not rely
on unspecified lifetime rules for locally created returned objects.

In test 11, changing `upper` inside the loop must not change its saved bounds.
The array iteration uses integer elements, avoiding the unclear loop-variable
type for non-integer arrays. No unspecified array-length member is used. Passing the sized array to `scan`
requires the provisional compatibility rule A5.
In test 12, the parameter `x` shadows the global, then a block-local `x`
shadows the parameter. The prints show each scope separately. Parentheses make
mixed operator priorities explicit. In test 13, `factorial(3)` reaches the
base case and returns 6; the call precedes the full definition.

Sources use `//` comments and `return` under the documented assumptions. They
avoid division, mixed arithmetic, halfway rounding and external entry arguments.
This suite is broader but does not cover the whole language: expression-bodied
routines, all conversions/operators, lexical errors and runtime bounds behavior
remain outside its coverage.

The repository has a lexer runner, but no parser, semantic analyzer, or program
execution. Lexing these files does not verify the expected program results or
reject the negative semantic tests.

## Course requirement

**Project Outline p. 11 asks for approximately 15 test cases.** The suite now
contains 15 independent source files. The Outline also asks students to discuss
specification inconsistencies with the lecturer or TA; unresolved assumptions
remain documented in DECISIONS.md.
