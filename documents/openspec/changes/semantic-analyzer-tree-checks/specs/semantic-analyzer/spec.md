# Semantic Analyzer Specification

## Purpose

This change records the current atomic semantic-checking foundation and the remaining work required by the project assignment. The checks do not modify the AST. Complete AST traversal and AST-modifying optimizations are planned separately.

## ADDED Requirements

### Requirement: Atomic type resolution

The semantic helper layer SHALL resolve supported type nodes and expressions without modifying the AST.

#### Scenario: Resolve primitive and named types

- **WHEN** the resolver receives a primitive type or a named type registered as `SymbolKind::Type`
- **THEN** it returns the corresponding `TypeInfo`

#### Scenario: Resolve aggregate types

- **WHEN** the resolver receives an array or record type
- **THEN** it recursively resolves array elements and represents records nominally

#### Scenario: Resolve supported expressions

- **WHEN** the resolver receives a literal, name reference, unary/binary expression, routine call, member access, or index access
- **THEN** it returns the expression type or `errorType` and records an error when the expression is invalid

#### Current implementation status

- [x] Primitive, named, array, and record type resolution.
- [x] Expression resolution for the supported expression nodes.
- [x] Resolver exposed through `IExpressionTypeResolver` and a factory.
- [ ] Type declaration registration through a complete analyzer pass.
- [ ] Array-size and compile-time constant validation.
- [ ] Static array-bound checking.

### Requirement: Variable and name checks

The variable checker SHALL validate declarations, name references, initializers, and simple name assignment targets independently of AST traversal.

#### Scenario: Validate variable declaration and reference

- **WHEN** a variable is declared in an active scope and later referenced through a visible variable or parameter
- **THEN** declaration and reference checks succeed

#### Scenario: Reject invalid variable operations

- **WHEN** a variable is duplicated in the current scope, a name is missing, or a non-variable symbol is referenced as a variable
- **THEN** the checker records a semantic error

#### Scenario: Infer and validate initializer type

- **WHEN** a variable has an initializer without an explicit type
- **THEN** the checker infers its type; when an explicit type exists, it validates compatibility

#### Current implementation status

- [x] Active-scope and duplicate-variable checks.
- [x] Nested name lookup and shadowing behavior.
- [x] Initializer inference and validation.
- [x] Simple assignment-target checks.
- [x] Declaration validation without mutating `ScopeStack`; the runner owns symbol registration.
- [x] Shared `areCompatible` check for initializers and assignments.
- [ ] Complete AST traversal and declaration-before-use enforcement.
- [ ] Member and index assignment targets.

### Requirement: Type compatibility

The checker SHALL provide one reusable compatibility rule for initializers, assignments, and future routine calls.

#### Scenario: Accept compatible values

- **WHEN** target and value have equal types, compatible nominal aggregate types, or integer/real numeric conversion
- **THEN** compatibility succeeds

#### Scenario: Reject incompatible values

- **WHEN** target and value have incompatible primitive or nominal aggregate types
- **THEN** compatibility fails

#### Current implementation status

- [x] Exact types.
- [x] Integer-to-real and real-to-integer compatibility.
- [x] Nominal records and recursive array compatibility through `sameType`.
- [x] Error-type suppression for derived checks.
- [ ] Routine-call and return-site integration.

### Requirement: For-loop checks

The checker SHALL validate numeric and array iteration ranges independently of a complete AST traversal.

#### Scenario: Validate numeric range

- **WHEN** a numeric loop has integer start and end expressions
- **THEN** the loop variable is registered as an integer read-only symbol

#### Scenario: Validate array iteration

- **WHEN** a loop iterates over an array expression
- **THEN** the loop variable is registered as a read-only symbol with the array element type

#### Current implementation status

- [x] Numeric range type checks.
- [x] Array source and element type checks.
- [x] Loop-variable conflict checks without registering the variable.
- [ ] Complete loop-scope traversal, registration, and cleanup.

### Requirement: Routine and contextual checks

The atomic routine checker SHALL validate routine declarations and calls. Scope population and ordering of these checks SHALL be performed later by the analyzer runner.

#### Scenario: Validate a routine call

- **WHEN** a call is checked against a routine declaration
- **THEN** the analyzer checks definition presence, argument count, and argument compatibility

#### Scenario: Validate a return statement

- **WHEN** a return is checked in a routine context
- **THEN** the analyzer checks result-value presence and result-type compatibility

#### Current implementation status

- [x] Parameter duplicate checks and parameter type validation.
- [x] Routine signature comparison.
- [x] Prototype/definition linking and duplicate-definition detection.
- [x] Routine lookup through visible scopes.
- [x] Routine-kind and definition-presence checks for calls.
- [x] Argument count and argument type compatibility checks.
- [x] Routine declaration validation without registering symbols; the runner owns prototype/definition state.
- [ ] Return placement and return-type checks.
- [x] Boolean `if` and `while` condition checks.
- [ ] Member/index assignment targets.

#### Scope-population boundary

The routine checker expects its `ScopeStack` to be prepared by the caller. It does not traverse `Program`, create routine scopes, register declarations, or decide declaration order. The analyzer runner will later populate the global and nested scopes and invoke the checker with resolved parameter and argument types. This keeps routine validation atomic and independent of AST traversal.

### Requirement: Analyzer traversal and compiler integration

The compiler SHALL eventually run the atomic checks over the parsed `Program`, collect all semantic errors, and report a non-zero exit status when errors exist.

#### Scenario: Analyze a valid program

- **WHEN** the complete analyzer runs on a valid parsed program
- **THEN** it reports no semantic errors

#### Scenario: Analyze an invalid program

- **WHEN** the complete analyzer runs on a parsed program with semantic errors
- **THEN** it reports all collected semantic errors and returns a non-zero result

#### Current implementation status

- [x] Atomic `if` and `while` condition checkers.
- [ ] Complete AST traversal.
- [ ] Analyzer runner integration.
- [ ] End-to-end positive and negative source tests.

### Requirement: AST-modifying optimizations

The project SHALL implement at least two optimizations that modify the AST.

#### Scenario: Simplify constant expressions

- **WHEN** an expression contains only compile-time constants
- **THEN** the optimizer replaces it with the computed literal node

#### Scenario: Simplify constant control flow

- **WHEN** an `if` or `while` condition is a compile-time boolean constant
- **THEN** the optimizer keeps the selected branch or removes the false loop

#### Current implementation status

- [ ] Constant-expression simplification.
- [ ] Constant-control-flow simplification.
- [ ] Optimizer tests.

### Requirement: Report and laboratory demonstration

The project SHALL provide a 2–3 page report and a laboratory demonstration describing implemented checks and optimizations with examples.

#### Scenario: Demonstrate implemented work

- **WHEN** the project is presented for laboratory evaluation
- **THEN** the report and demonstration identify completed checks, pending checks, and completed AST-modifying optimizations

#### Current implementation status

- [ ] Report.
- [ ] Laboratory demonstration.
