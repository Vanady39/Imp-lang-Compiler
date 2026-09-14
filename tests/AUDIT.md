# Strict source and specification audit

Reviewed on 2026-09-14 against all six pages of
[Project I.pdf](../documents/Project%20I.pdf), the 15 test sources,
[README.md](README.md), and [DECISIONS.md](../DECISIONS.md).

This is a manual grammar/type/value-flow review, plus a lexer smoke run.
It is not execution by an implemented compiler or a proof of language conformance.
The PDF has gaps; behavior not specified there is distinguished from an actual
error. No positive test can be called fully defined by the PDF alone.

## Shared conditions

- A1: every file declares `test()`; the published `Parameters` rule requires at
  least one parameter. Selecting `test` as entry is allowed, but the empty list
  itself needs the documented extension.
- A2: aggregate declarations are assumed to allocate writable objects. The PDF
  specifies reference semantics, but no construction mechanism. No test reads
  an uninitialized scalar or an unassigned nested reference. Copying an object
  reference does not read its uninitialized fields.
- A3: `return Expression` is a provisional completion of the missing statement
  grammar. It exits the current invocation; an aggregate result of the declared
  type is its reference. Test 10 relies on that latter interpretation.
- A4: `//` is the team's comment syntax, not a rule in Project I. Tests 03 and
  04 have no comments; the other files do.
- A5: test 11 needs a compatibility exception for a sized actual array and a
  sizeless parameter. Page 3 intends sizeless parameters to be usable, but p. 4
  requires the same aggregate type declaration and supplies no exception.

The PDF also omits precise identifier/numeric literal patterns and print
formatting. These sources use ordinary ASCII names and decimal numbers supported
by the current lexer. Printed expectations denote numeric values in source order,
not an exact byte-for-byte output contract. Multi-expression prints have no side
effects, so expression evaluation order cannot affect their values. Numeric
values are small and fit the current implementation; Project I leaves numeric
representation and limits to the implementation.

## Every test

All positive results below are conditional on the listed assumptions.
Negative tests must fail semantically after the relevant syntax is accepted.

| Test | Result | Strict check and assumptions |
| --- | --- | --- |
| 01 variables | `2; 5` | Explicit integer initialization, replacement, then independent inferred integer. No duplicate declaration. A1, A4. |
| 02 conversion | `4` | `x` is assigned before its first read. The assignment table explicitly rounds real to nearest integer; 3.7 has no tie. A1, A4. |
| 03 if/else | `1` | Literal true selects only the then branch. The else branch is present but is not executed by this test. A1. |
| 04 loops | `0; 1; 1; 2; 3` | While exits at x = 2. For includes both bounds; its integer variable is implicit and never assigned in the body. A1. |
| 05 aggregates | `5; 7` | Array index 1 is within size 2. The accessed element and record field are assigned before use; unused a[2] is never read. A1, A2, A4. |
| 06 routine result | `5` | One integer argument for one integer parameter. Integer addition returns 5. A1, A3, A4. |
| 07 wrong argument count | Semantic error at `add(1)` | Exactly one intended violation: two parameters, one argument. The provided integer has a conforming type. The routine must not execute. A1, A3, A4. |
| 08 nested records | `7; 7` | box and alias share one Box; its point field refers to p. Writing alias.point.x changes the same Point read through both expressions. Explicit reference assignments use matching named types. A1, A2, A4. |
| 09 nested arrays/records | `7; 9` | row.values and matrix[1] both refer to a; rows[1] refers to row. Writes through either chain reach a[1]. All indices are 1 in size-1 arrays, and every nested reference is assigned. A1, A2, A4. |
| 10 aggregate routines | `2; 3; 8; 9` | First update changes caller objects to 2 and 3. The returned Cell reference is assigned to result, which sets c.x to 7. The returned array reference sets a[1] to 8. Second update yields 8 and 9; discarding its result is explicitly allowed on p. 4. Returned objects belong to the caller, so no local-object escape is needed. A1, A2, A3, A4. |
| 11 array parameters/ranges | `2; 4; 4; 2; 2; 1` | Both array elements are initialized. Array traversal is forward then reverse. Numeric reverse loop saves bounds 1 and 2 once, despite upper becoming 0. Empty range and false while print nothing. Neither loop variable is modified. A1, A2, A4, **A5: unresolved compatibility conflict**. |
| 12 scopes/boolean | `1; 0; 2; 10; 20; 1` | Block-local boolean x shadows integer parameter x, which shadows global x. Local Count shadows its global type synonym. Conversions use explicit assignments: 1 -> true -> 1 and 0 -> false -> 0. Parentheses remove precedence ambiguity; boolean condition is true. Positive integer arithmetic has no overflow or zero divisor. A1, A4. |
| 13 forward/recursion | `6` | Forward signature and definition match. The call appears after the forward declaration. Recursion uses 3, 2, 1; each branch returns. Parenthesized n - 1 removes any precedence issue. A1, A3, A4. |
| 14 record mismatch | Semantic error at `readPoint(other)` | Point and OtherPoint come from distinct record declarations, even though their fields match. Page 4 requires nominal identity for arguments. The actual argument is initialized and the argument count is correct. No execution is intended. A1, A3, A4; object construction follows A2 if evaluated. |
| 15 readonly loop variable | Semantic error at `i := 0` | Page 5 expressly prohibits assigning the loop variable. The assignment is otherwise type-correct and the range nonempty. No execution is intended. A1, A4. |

## Corrections from this review

- Tests 08 and 10 now copy aggregate references using `:=` rather than typed
  initializers. Test 12 uses `:=` for cross-type conversions. The PDF gives an
  explicit assignment table but does not separately give initializer conversion
  rules. Expected values and intended coverage are unchanged.
- A3 now states the return/control-flow/reference interpretation required by
  tests 10 and 13. This documents a dependency, not lecturer approval.
- A5 records the previously unlisted conflict exercised by test 11. Keeping this
  test preserves sizeless-array coverage, but its positive status is conditional.
- README now accounts for multiple expressions in one print. DECISIONS lists
  the actual array sizes (1 and 2) and all three lexer library source files.

No additional value-flow or type error was found in the positive tests under
these assumptions. The three negative tests have the intended semantic errors.
Accepting A1–A5 or replacing their dependent cases requires a language-specification
decision; a successful lexer run cannot settle these questions.
