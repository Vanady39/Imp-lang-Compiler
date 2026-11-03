# Tasks

## 1. Atomic semantic checks — completed

- [x] 1.1 Define `TypeInfo`, primitive types, nominal record types, array types, and `errorType`.
- [x] 1.2 Implement `IExpressionTypeResolver` and the factory `makeExpressionTypeResolver`.
- [x] 1.3 Implement type resolution for primitive, named, array, and record type nodes.
- [x] 1.4 Implement expression type resolution for literals, names, unary/binary expressions, routine-call results, member access, and array indexing.
- [x] 1.5 Implement variable declaration validation, duplicate-name, name-reference, initializer inference, and simple assignment checks. Scope registration remains the runner's responsibility.
- [x] 1.6 Implement shared assignment/initializer compatibility for exact types, numeric conversions, and nominal aggregates.
- [x] 1.7 Implement numeric and array `for`-loop checks, including element typing and loop-variable conflict validation. Scope registration and read-only lifetime remain the runner's responsibility.
- [x] 1.8 Verify the helper layer with `semantic_base`, a clean build, and `ctest`.

## 2. Remaining atomic semantic checks

- [x] 2.1 Add atomic routine-signature checks: parameter types, result type, prototype/definition compatibility, and duplicate definitions.
- [x] 2.2 Add atomic routine-call checks: routine lookup, definition presence, argument count, and per-argument compatibility. The checker receives a prepared `ScopeStack`; scope population remains the runner's responsibility.
- [x] 2.3 Add atomic return checks: routine context, required/missing return value, unexpected return value, and result compatibility.
- [x] 2.4 Add atomic condition checks for boolean `if` and `while` expressions.
- [ ] 2.5 Extend assignment-target checks to record members and array elements.
- [ ] 2.6 Add array-size checks: integer type, positive value, parameter-only sizeless arrays, and compile-time constant requirement.
- [ ] 2.7 Add constant-expression evaluation needed for array sizes and possible static array-bound checks.
- [ ] 2.8 Add focused unit tests for each atomic checker, including positive and negative cases.
- [x] 2.9 Add `if` and `while` condition checker implementations to the semantic build.

## 3. AST traversal and integration

- [x] 3.1 Implement a read-only AST traversal that invokes the atomic checkers.
- [x] 3.2 Add declaration registration and scope management during traversal.
- [x] 3.3 Connect the analyzer to the compiler runner after successful lexing and parsing.
- [x] 3.4 Verify positive and negative source programs through the complete compiler pipeline.

## 4. AST-modifying optimizations — completed for the selected scope

- [x] 4.1 Implement arithmetic constant-expression folding.
- [x] 4.2 Implement constant `if true` and `if false` simplification.
- [x] 4.3 Remove statements after `return` from the same `Body`.
- [ ] 4.4 Add focused optimizer tests that verify AST modification and source behavior preservation.

## 5. Report and demonstration

- [ ] 5.1 Prepare a 2–3 page report describing implemented semantic checks, planned/completed optimizations, and examples.
- [ ] 5.2 Prepare a laboratory demonstration of the checker and optimizer behavior.
- [ ] 5.3 Confirm the report states clearly which checks and optimizations are implemented and which remain pending.
