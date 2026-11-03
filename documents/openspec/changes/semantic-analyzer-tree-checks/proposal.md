# Proposal

## Why

The project is implementing the semantic-analysis stage incrementally. The current work focuses on reusable, atomic semantic checks that can later be connected to an AST traversal. The analyzer traversal and command-line integration are not part of the current implementation state.

The project requirements for this stage are:

- at least two semantic checks that do not modify the AST;
- at least two optimizations that modify the AST;
- a short report and laboratory demonstration describing the implemented checks and optimizations.

## Current scope

This change records the atomic semantic-checking foundation. The checks are implemented as independent helpers and checker classes. They validate supplied AST nodes, types, and scopes without traversing the whole program and without modifying the AST.

Implemented atomic checks include:

- primitive, named, array, and record type resolution;
- expression type resolution for literals, names, unary and binary expressions, routine calls, member access, and array indexing;
- variable declaration and duplicate-name checks;
- variable and parameter name-reference checks;
- initializer type inference and compatibility;
- assignment compatibility, including numeric conversions and nominal aggregate types;
- numeric and array `for`-loop checks;
- read-only loop-variable checks;
- scope-stack and type-representation behavior.

The type resolver is exposed through `IExpressionTypeResolver` and created through a factory. Its concrete implementation remains private to the `.cpp` file.

## Not in current scope

The following work remains for later tasks:

- complete AST traversal and analyzer pipeline;
- routine declaration and prototype/definition checks;
- routine call argument checking;
- return and condition statement checks;
- member/index assignment-target checks;
- array-size constant validation and bounds checking;
- AST-modifying optimizations;
- execution, code generation, and optimizer integration.

## Planned optimizations

To satisfy the project minimum, the later optimization stage will implement at least two AST-modifying optimizations:

1. constant-expression simplification;
2. constant-control-flow simplification for `if` and `while`.

## Impact

Current changes are limited to semantic helper interfaces and implementations, type representation, and atomic checker code. The compiler still does not run semantic analysis over the complete program tree.

The implementation depends on the language decisions recorded in `documents/DECISIONS.md`, especially numeric conversion, nominal aggregate typing, and loop-variable rules.