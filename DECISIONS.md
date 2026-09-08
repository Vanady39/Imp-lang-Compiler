# Decisions for the simple test suite

This document applies to the 10 active tests listed in
[tests/README.md](tests/README.md). It distinguishes the supplied **Project I**
rules from the team's limited interpretations. It does not attempt to complete
the entire language specification. Page numbers refer to Project I.pdf.

## A. Official Project I rules used here

- **Declarations and inference (pp. 1-2):** a variable can have an explicit type
  and optional initializer, or infer its type from an initializer. Entities
  must be declared before use. Integer, real, and boolean are primitive types.
- **Assignment and conversion (pp. 3-4):** assignment changes the target value;
  a real assigned to an integer is rounded to the nearest integer.
- **Control flow (pp. 4-5):** while checks its condition before the body; a
  numeric for range includes both bounds and advances by 1 when not reversed;
  if selects a branch using its condition.
- **Aggregates (pp. 2-3, 6):** records have named fields; arrays have an element
  type and a size, with indexing from 1. Dotted and indexed access appear in
  the grammar. Aggregate assignment copies references, although these tests
  do not exercise aliasing.
- **Routines (pp. 4-6):** calls must have exactly as many arguments as declared
  parameters, with conforming types. Typed routines produce values usable in
  expressions. The routine-body grammar explicitly includes `=> Expression`.
- **Entry and printing (pp. 1, 5):** execution starts at a routine selected at
  launch; `main` is not a mandated name. A print statement accepts expressions.

## B. Minimal team assumptions

These are provisional and **not confirmed official rules**.

| ID | Interpretation needed by the active suite | Reason and scope |
| --- | --- | --- |
| A1 | A routine declaration may have an empty parameter list, as in `routine main()`. | The p. 5 `Parameters` production requires at least one parameter. All 10 files use an entry with no arguments to avoid unspecified entry-argument conversion. No test makes a zero-argument call in source code. |
| A2 | Declaring an array or record variable creates an object whose elements or fields can be assigned. | Project I describes reference types and access but does not explicitly define object creation. Tests 07 and 08 need writable objects. They assign every accessed member before reading it; no default zero initialization is assumed. |
| A3 | In a typed routine, `=> Expression` evaluates and returns that expression's value. | This body syntax is official on p. 5, but its return behavior is not separately explained in prose. Test 09 uses this interpretation instead of extending the Statement grammar with `return`. |

These are the only adopted assumptions needed by this suite. They should be
confirmed with the lecturer or TA. Removing them entirely would require changing
the entry convention or dropping aggregate access / returned-value coverage.

## C. Still open questions

The following gaps are recorded, not resolved by additional team rules.

| Topic | What Project I leaves unclear | Effect on the active tests |
| --- | --- | --- |
| `return` | p. 6 mentions a return statement, but p. 3 omits it from `Statement`; its syntax is missing. | No `return` statement is used. Test 09 uses the listed expression-body syntax with A3. |
| Empty parameter list | p. 5 requires a first parameter in `Parameters`; it does not explicitly permit `()` in declarations. | All entries depend on A1; lecturer/TA confirmation remains open. |
| Comment syntax | No lexical rule for comments is provided. | Source files contain no comments. |
| Array/record allocation | Creation, default initialization, and lifetime are not explicitly defined for these reference types (pp. 2-3). | Tests 07 and 08 depend only on A2 and explicit member assignments. Default values and lifetime beyond the entry routine are not tested. |
| Operator precedence | p. 6 says conventional priorities, but the `Simple` / `Factor` productions give addition tighter binding than multiplication. Boolean operators also share one grammar level. | No test combines competing arithmetic or boolean operators. The while comparison contains no arithmetic subexpression. |
| Real-to-integer rounding | p. 3 specifies nearest-integer rounding but gives no halfway tie rule. | Test 03 uses `3.7 -> 4`, whose nearest integer is unique. No `floor(x + 0.5)` rule is adopted. |
| Integer-to-boolean | p. 4 maps 0 to false and 1 to true; other integers are erroneous. It does not prescribe when the error is reported. | Not exercised. Conditions use boolean literals or an integer comparison. |
| `/` semantics | p. 6 lists division without a full operand/result type rule, including whether integer operands produce an integer or real result. | No division is used. |
| Mixed arithmetic | Assignment conversions (pp. 3-4) do not fully define arithmetic promotion for mixed operand types. | All arithmetic operands are integers. Real-to-integer assignment is a separate, explicitly specified conversion. |
| Sizeless array length | p. 3 promises a special dotted way to obtain array size without giving the member name. | No sizeless arrays or length queries are used. |
| Entry arguments | p. 1 allows launch arguments without specifying their external representation or conversion. The treatment of an entry's returned value is also unspecified. | Entries have no parameters under A1 and no return type. |
| Array bounds | Indexing starts at 1 (p. 3), but out-of-bounds behavior and checking are not defined. | The only index is 1 in an array of size 2. |

The compiler's exact print formatting is also unspecified (p. 5). The README
therefore records ordered numeric values, not mandatory whitespace or punctuation.
