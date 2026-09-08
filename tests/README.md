# Simple test suite

Exactly **10 source files**: 9 positive tests and 1 negative semantic test.
Each file demonstrates one main language feature and is compiled independently.
The supplied reference PDFs are not bundled in this repository; page numbers
below are PDF page numbers.

## Running and reading the expectations

For every positive file, select entry routine `main` with **no arguments**.
Project I p. 1 allows any routine to be selected at launch; the name `main`
is only our convention. Helper routines are declared before their calls.

The expected outputs below are ordered numeric values, one per executed
`print`. For example, `1; 2; 3` means three successive prints, not literal
semicolons. Project I p. 5 does not fix output whitespace or line formatting.

The tests use the three explicitly provisional interpretations in
[DECISIONS.md](../DECISIONS.md): **A1** allows empty declaration parameter lists,
**A2** creates writable array/record objects at variable declaration, and **A3**
returns the expression's value from an expression-bodied routine.
A1 applies to all files, A2 only to tests 07 and 08, and A3 only to test 09.
These are team assumptions awaiting lecturer/TA confirmation, not additional
official rules.

## The 10 tests

| File | Main aspect | Entry routine | Expected output / error | Basis in Project I |
| --- | --- | --- | --- | --- |
| [01_assignment.txt](positive/01_assignment.txt) | Declare an integer, then replace its value | `main` | `2` | pp. 1-2: variable declaration; p. 3: assignment |
| [02_inference.txt](positive/02_inference.txt) | Infer `integer` from literal `7` | `main` | `7` | pp. 1-2: omitted type is inferred from the initializer |
| [03_real_to_integer.txt](positive/03_real_to_integer.txt) | Assign a real value to an integer | `main` | `4` | p. 3: round to the nearest integer; `3.7` has no tie |
| [04_if_else.txt](positive/04_if_else.txt) | Select the then branch for true and the else branch for false | `main` | `1; 2` | p. 5: conditional execution |
| [05_while.txt](positive/05_while.txt) | Repeat while the condition holds | `main` | `1; 2; 3` | p. 4: condition checked before each iteration |
| [06_for.txt](positive/06_for.txt) | Iterate over a small inclusive integer range | `main` | `1; 2; 3` | pp. 4-5: inclusive range with step 1 |
| [07_array_access.txt](positive/07_array_access.txt) | Write and read one array element | `main` | `7` | p. 3: sized arrays and indexing from 1; p. 6: indexed access; A2 |
| [08_record_access.txt](positive/08_record_access.txt) | Write and read one record field | `main` | `7` | p. 3: record fields; p. 6: dotted access; A2 |
| [09_routine_result.txt](positive/09_routine_result.txt) | Pass an integer parameter and use the returned value | `main` | `5` | pp. 5-6: typed routines, `=> Expression`, calls in expressions; A3 |
| [10_wrong_argument_count.txt](negative/10_wrong_argument_count.txt) | Reject too many arguments | `main` (do not execute) | Semantic error at `show(1, 2)`: expected 1 argument, received 2 | p. 4: argument count must equal parameter count |

The negative file must be rejected during semantic analysis. A syntax error
does not count as the intended result. Exact diagnostic wording is not required.

## Manual consistency review

All 10 files were read against Project I and traced by hand. In the while test,
`i` becomes 4 after the third print, so the next condition is false.
The for test includes both endpoints. The function computes `4 + 1`.
In the negative test, both supplied arguments are integers; the single intended
error is their count.

Every declaration precedes its use. Every value read is explicitly initialized
or assigned first, and array index 1 is within the declared size 2.
The sources contain no comments, `return` statements, mixed arithmetic,
division, ambiguous operator combinations, sizeless arrays, or entry arguments.
The conversion test avoids halfway rounding. Remaining specification questions
and the limited dependencies above are recorded in DECISIONS.md.

There is currently no compiler or test runner in this repository. This review
checks source syntax under the stated assumptions and manually derived results;
it is **not an executed compiler test run**.

## Course requirement

**Project Outline p. 11 asks for approximately 15 test cases.** This suite is
deliberately reduced to 10 to keep the defense simple. That is a potential
deviation from the assignment until the lecturer or TA confirms it is acceptable.
No such confirmation is recorded here. The Outline also asks students to discuss
specification inconsistencies with the lecturer or TA.
