# Test cases

Each `.txt` file contains source code in the project language. Expected values
are recorded in comments; commas in those comments separate values and do not
prescribe literal output punctuation. There is currently no compiler or test
runner in this repository.

## Entry routines

Compile each file independently. Run the following routines independently for
positive tests; helper routines are not additional entry points.

| File in `positive/` | Entry routines |
| --- | --- |
| `01_conversions.txt` | `conversion_test` |
| `02_reference_semantics.txt` | `record_alias_test`, `array_alias_test`, `nested_record_test` |
| `02b_anonymous_array.txt` | `anonymous_array_test` |
| `03_loops.txt` | `loops_test` |
| `03b_loop_over_real_array.txt` | `real_loop_test` |
| `03c_precedence.txt` | `precedence_test` (pending precedence decision) |
| `04_scope_inference.txt` | `shadow_test`, `type_shadow_test`, `synonym_test`, `inference_test` |
| `04e_arith_inference.txt` | `arith_inference_test` |
| `05_forward_early_return.txt` | `forward_return_test`: prints 10, then 8 |
| `06_array_parameter_alias.txt` | `array_parameter_test`: prints 11, then 17 |

## Negative tests

Most files in `negative/` expect a compilation error at the intentionally invalid
operation. A syntax error elsewhere must not count as the expected semantic error.

- `01c_bool_from_int_param.txt` must compile. Run `driver`: it prints `true`,
  then `false`, and fails at runtime on `to_bool(5)`.
- `01b_bool_from_int_const.txt` assigns the literal `5` to a boolean and must
  fail at compile time under DECISIONS.md #10. It does not require tracking the
  value of a mutable variable.
- `05_wrong_argument_count.txt` expects a semantic error at `add(1)` because
  `add` requires two arguments. Do not run this file.

## Basis in the supplied documents

Reviewed against Project I.pdf, the supplied DECISIONS.md, the 2026 Project
Outline, and the Introduction lecture. Page references below use PDF page numbers.
Those reference documents are supplied separately and are not bundled here.

The Project Outline p. 11 asks for a small suite of approximately 15 cases and
for specification inconsistencies to be discussed with the lecturer or TA.
The Introduction p. 14 lists the test suite as part of Lab 2. Neither document
prescribes these exact three additional programs. There are now 20 source files.

| Added test | Specification basis | What a failure detects |
| --- | --- | --- |
| `positive/05_forward_early_return.txt` | Project I pp. 5-6 permits a routine header without a body, followed by a matching definition; DECISIONS.md #3 and #4 allows empty parameter lists and `return Expression` anywhere in a routine body | Unresolved forward calls, incorrect returned values, or execution continuing after an early return |
| `positive/06_array_parameter_alias.txt` | Project I p. 3 permits sizeless array parameters; pp. 4 and 6 give parameter passing the semantics of assignment, including reference copying | Rejecting different array lengths for a sizeless parameter or copying array contents instead of sharing the object |
| `negative/05_wrong_argument_count.txt` | Project I p. 4 requires equal argument and parameter counts | Accepting a call with a missing required argument |

The array-parameter test uses explicitly initialized elements and a no-argument
entry routine. It does not depend on the undefined array-length member syntax or
on converting command-line text to aggregate arguments (DECISIONS.md #15, #16).
The sizeless parameter intentionally accepts different lengths, as provided for
in Project I; this does not relax same-length compatibility for ordinary sized
anonymous arrays under DECISIONS.md #11.

## Adopted decisions and remaining questions

The supplied DECISIONS.md is the team's record of assumptions where Project I
is incomplete. These are already adopted for the suite:

| Decision | Suite behavior |
| --- | --- |
| #1, #3, #4 | `//` comments, empty parameter lists, and `return Expression` are allowed |
| #6 | Real-to-integer rounding uses `floor(x + 0.5)`, including `-2.5 -> -2` |
| #7 | Record/array declarations allocate objects with zero/false initial fields and elements |
| #8 | An array-loop variable has the array's element type |
| #10 | Invalid constant integer-to-boolean conversions fail at compile time; other values are checked at runtime |
| #11 | Sized anonymous arrays conform by element type and length; named aggregate types retain name identity |
| #12, #13 | Real operands promote mixed arithmetic; `/` always produces a real result |

Only the precedence questions affecting these tests remain open in DECISIONS.md
(#5, #5a). `03c_precedence.txt` is a decision probe, not an unconditional pass/fail
test until those questions are settled:

- The current arithmetic expectations use conventional precedence: 14 and 4
  for the first two prints. Reading the grammar literally gives 20 and 24.
- The current boolean assumption is equal precedence with left associativity,
  giving `false` for `true or false and false`. If `and` binds tighter, the result
  is `true`. Unlike the old expression, this one distinguishes those rules.

The three new tests do not mix operators of disputed precedence.
