# Decisions for the simple test suite

This document applies to the 15 active test files (the original 7 groups plus 8 additional cases) in
[tests/](../tests/). It distinguishes the supplied **Project I**
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
  the grammar. Aggregate assignment copies references; tests 08–10 exercise aliasing and
  mutations through shared references.
- **Routines (pp. 4-6):** calls must have exactly as many arguments as declared
  parameters, with conforming types. Typed routines produce values usable in
  expressions. The routine-body grammar explicitly includes `=> Expression`.
- **Entry and printing (pp. 1, 5):** execution starts at a routine selected at
  launch; `main` is not a mandated name. A print statement accepts expressions.

## B. Minimal team assumptions

These are provisional and **not confirmed official rules**.

| ID | Interpretation needed by the active suite | Reason and scope |
| --- | --- | --- |
| A1 | A routine declaration may have an empty parameter list, as in `routine test()`. | The p. 5 `Parameters` production requires at least one parameter. All 15 files use an entry with no arguments to avoid unspecified entry-argument conversion. No test makes a zero-argument call in source code. |
| A2 | Declaring an array or record variable creates an object whose elements or fields can be assigned. | Project I describes reference types and access but does not explicitly define object creation. Tests 05, 08–11, and 14 need writable objects. Nested aggregate references are explicitly assigned existing objects before chained access. Tests assign every accessed member before reading it; no default zero initialization is assumed. |
| A3 | In a typed routine, `return Expression` evaluates the expression, exits that routine invocation, and returns its value. For an aggregate of the declared result type, the returned value is its reference, without cloning the object. | The requested test sources use this syntax in tests 06, 07, 10, 13, and 14. Project I mentions return on p. 6 but omits its syntax from the Statement grammar; this remains a provisional interpretation. Since p. 6 ties `return` to routines with a result type, the parser accepts it only in the body of such a routine and reports it as a syntax error anywhere else. |
| A4 | The only comment form is the single-line `//`, running to the end of the line and skipped as whitespace. There are **no** multi-line comments: `/*` is not a comment opener and simply scans as the two operators `/` and `*`. The newline that ends a `//` comment is not consumed and stays a significant `Newline` token. | Project I gives no lexical rule for comments at all (see section C). The scanner needs a fixed answer, and the single-line form is the smallest one that is useful. Leaving out `/* */` also removes the unterminated-comment error case entirely. Tests use these comments to annotate expected results. |
| A5 | A sized integer array can be passed to `array [] integer`; the parameter keeps the same object and actual size. | Test 11 needs this interpretation. Page 3 explicitly permits sizeless array parameters, but p. 4 requires aggregate argument and parameter types to originate from the same type declaration. Their compatibility exception is not specified. This is a provisional resolution, not an unambiguous rule from the PDF. |

A1-A5 are the assumptions used by the test suite. They should be
confirmed with the lecturer or TA. Removing them entirely would require changing
the entry convention or dropping aggregate access / returned-value coverage.

The parser adds the following interpretations of the grammar:

| ID | Interpretation used by the parser | Reason and scope |
| --- | --- | --- |
| A6 | Conventional operator priorities: `* / %` bind tighter than `+ -`, so `a + b * c` is parsed as `a + (b * c)`. The parser swaps the operator sets of the p. 6 `Simple` and `Factor` productions: `Simple : Factor { ( + \| - ) Factor }`, `Factor : Summand { ( * \| / \| % ) Summand }`. Arithmetic operators are left-associative. `and`, `or`, `xor` share one level and are left-associative; a `Relation` allows at most one comparison. | p. 6 asks for "conventional priorities", and Lecture 3 (slides 18, 23) gives multiplication the higher priority, but the published `Simple` / `Factor` productions say the opposite. The text and the lecture are followed instead of the literal productions. Test 12 uses parentheses and is unaffected. |
| A7 | Calls follow the p. 4 `RoutineCall` grammar: a call without arguments is written `f`, and `f()` is a syntax error. Inside an expression a bare identifier is always parsed as a `ModifiablePrimary`; whether it denotes a variable or a call without arguments is left to semantic analysis. As a statement, a bare identifier is a call. | A1 still allows `()` in a declaration, but calls need no matching exception. A bare name cannot be classified without the symbol table. |

## C. Still open questions

The following gaps are recorded, not resolved by additional team rules.

| Topic | What Project I leaves unclear | Effect on the active tests |
| --- | --- | --- |
| `return` | p. 6 mentions a return statement, but p. 3 omits it from `Statement`; its syntax is missing. | Tests 06, 07, 10, 13, and 14 use `return Expression` under A3, always inside a routine with a result type. |
| Empty parameter list | p. 5 requires a first parameter in `Parameters`; it does not explicitly permit `()` in declarations. | All entries depend on A1; lecturer/TA confirmation remains open. |
| Comment syntax | No lexical rule for comments is provided. Resolved provisionally by A4, which is a team choice and not an official rule. | Source files annotate expected results with `//` comments. The scanner implements A4. |
| Array/record allocation | Creation, default initialization, and lifetime are not explicitly defined for these reference types (pp. 2-3). | Tests 05, 08–11, and 14 depend on A2 and explicit member assignments. Nested references are explicitly assigned; test 10 returns caller-owned objects. Default values and escaping locally created objects are not tested. |
| Operator precedence | p. 6 says conventional priorities, but the `Simple` / `Factor` productions give addition tighter binding than multiplication. Boolean operators also share one grammar level. | Test 12 uses explicit parentheses when combining different operators, avoiding reliance on the disputed priorities. The parser uses conventional priorities (A6). |
| Unary operators | p. 6 allows a sign only before numeric literals and `not` only before an integer literal, so `-x` and `not flag` are not expressible, although p. 2 lists boolean negation. | Not used by the tests. The parser follows the grammar literally and reports `not true` / `-x` as syntax errors. |
| Real-to-integer rounding | p. 3 specifies nearest-integer rounding but gives no halfway tie rule. | Test 02 uses `3.7 -> 4`, whose nearest integer is unique. No `floor(x + 0.5)` rule is adopted. |
| Integer-to-boolean | p. 4 maps 0 to false and 1 to true; other integers are erroneous. It does not prescribe when the error is reported. | Test 12 checks 1 -> true and 0 -> false, then converts back to integer for printing. Invalid integer-to-boolean conversion is not tested. |
| `/` semantics | p. 6 lists division without a full operand/result type rule, including whether integer operands produce an integer or real result. | No division is used. |
| Mixed arithmetic | Assignment conversions (pp. 3-4) do not fully define arithmetic promotion for mixed operand types. | All arithmetic operands are integers. Real-to-integer assignment is a separate, explicitly specified conversion. |
| Sized/sizeless array compatibility | p. 3 permits sizeless parameters; p. 4 requires the same aggregate type declaration without stating an exception. | Test 11 is conditional on A5; it is not a strict conformance test until this conflict is resolved. |
| Sizeless array length | p. 3 promises a special dotted way to obtain array size without giving the member name. | Test 11 uses a sizeless integer-array parameter and iterates over its elements without a length query. |
| Entry arguments | p. 1 allows launch arguments without specifying their external representation or conversion. The treatment of an entry's returned value is also unspecified. | Entries have no parameters under A1 and no return type. |
| Array iteration variable | p. 5 calls the loop variable integer but also permits iteration over array elements of arbitrary type. | Test 11 iterates only over integer arrays. |
| Array bounds | Indexing starts at 1 (p. 3), but out-of-bounds behavior and checking are not defined. | All indices in the positive tests are within the declared sizes (1 or 2). |

The compiler's exact print formatting is also unspecified (p. 5). Expected-result
comments in the tests describe numeric values, not mandatory whitespace or punctuation. Multiple expressions in one `print`
are listed in source order as a display convention; the PDF does not explicitly
define output formatting or evaluation order. All such expressions in this suite
are side-effect-free, so evaluation order does not change their values.

The suite is conditionally valid under the assumptions above, not proven fully
defined by the PDF alone. Conversions
and aggregate reference copying in tests 08, 10, and 12 now use explicit `:=`,
so those checks do not require extending assignment conversion rules to typed
initializers. Basic same-type initializers and type inference still use `is`.

## D. Current build and runner

The repository has a minimal CMake build with three artifacts:

| Artifact | Source | Purpose |
| --- | --- | --- |
| `liblexer.a` | `lexer/lexer.cpp`, `lexer/reader.cpp`, `lexer/token_name_map.cpp` | Static library containing the lexer and reader implementation. |
| `libparser.a` | `parser/parser.cpp` (tree classes in `parser/ast.h`) | Static library containing the syntax analyzer. |
| `compiler` | `main.cpp` linked with `parser` and `lexer` | Reads one source file and prints its program tree, or the lexical and syntax errors. |

`compiler` currently stops after syntax analysis. It does not type-check,
execute, or generate code, so its output is the program tree, not the program
output annotated in the test sources.

Configure and build from the repository root:

```bash
cmake -S . -B build
cmake --build build
```

Run it on one test file:

```bash
./build/compiler tests/positive/13_forward_recursion.txt
```

Run it on all test files:

```bash
for f in tests/*/*.txt; do
  echo "=== $f ==="
  ./build/compiler "$f"
done
```

The negative tests contain semantic errors only, so they are syntactically
valid and also produce a tree.

If the executable name changes later, update the `add_executable(...)` target in
`CMakeLists.txt` and use the matching path under `build/`.

## E. Syntax analyzer

The parser is a hand-written top-down recursive descent parser (Lectures 3-4):

- The lexer produces the whole token buffer first; the parser reads it with
  unlimited lookahead (Lecture 3, "interaction via temporary storage").
- Each grammar rule has its own parsing function; `{ ... }` repetitions are
  loops, and left-associative operator chains are built as in Lecture 3,
  slide 24.
- The program tree is a class hierarchy (Lecture 4): `Expression`,
  `Statement`, `TypeNode` and `Declaration` derive from `Node`. The classes
  hold only data. Processing follows the CCI approach (Lecture 4, slides
  18-19): every node stores its `NodeType` tag, and each pass (for now, the
  tree printer in `main.cpp`) is a `switch` over the tag followed by a
  `static_cast`. The switches have no `default`, so `-Wswitch` (part of
  `-Wall`) warns when a node kind is not handled. A `Body` keeps declarations
  and statements in one list because they can be mixed.
- New lines and `;` separate declarations and statements. Two items on one
  line without `;` are a syntax error.
- Errors use panic-mode recovery: the parser reports the error, skips to the
  next separator, `end` or `else` and continues with the next declaration or
  statement. Only the first error on each line is reported, and tokens the
  lexer already rejected are not reported again. A nested `routine` is
  reported and then parsed completely, so its `end` does not close the
  enclosing body.
