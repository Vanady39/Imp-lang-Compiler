# Design

## Context

The compiler uses a tagged AST and separate semantic helper classes. The current implementation stage is atomic validation: checker methods receive already selected AST nodes, types, and scopes. They collect semantic errors and return a type or boolean result. A complete AST traversal will be added later.

The project requires at least two non-mutating semantic checks and at least two AST-modifying optimizations. The current change implements the non-mutating foundation only.

## Current architecture

### Type representation

`TypeInfo` represents:

- `integer`;
- `real`;
- `boolean`;
- nominal records, identified by their owning AST node;
- arrays, identified by their element type;
- `errorType`, which suppresses derived errors.

Array size is currently not stored in `TypeInfo`. Array-size validation is a separate pending check.

### Type resolver

`IExpressionTypeResolver` is the public interface. It exposes:

- expression type resolution;
- type-node resolution.

`ExpressionTypeResolver` is the concrete implementation in `expression_type.cpp`. It is created through `makeExpressionTypeResolver`. The header exposes the interface and factory; the concrete implementation is private to the implementation file.

The resolver currently handles:

- primitive types;
- named types through `ScopeStack` and `SymbolKind::Type`;
- array element types;
- named and anonymous records;
- literals;
- name references;
- unary and binary operators;
- routine-call result types;
- member access;
- array indexing.

The resolver does not register type declarations and does not perform a program traversal.

### Variable checker

`ICheckerVariable` exposes independent checks for:

- variable declaration;
- name reference;
- initializer typing and inference;
- type compatibility;
- assignment to a simple name target.

The shared compatibility check accepts exact types, integer/real numeric conversions, and nominal aggregate compatibility. `errorType` prevents duplicate derived errors.

### For-loop checker

`ICheckerFor` exposes independent checks for:

- integer numeric ranges;
- array iteration ranges.

It validates the range/source types and checks loop-variable name conflicts. It does not register the loop variable; the future runner will manage its read-only symbol and lifetime.

### Scope and symbol structures

`ScopeStack` stores nested maps of names to `Symbol` entries. `Symbol` currently supports variables, parameters, routines, and types. The scope helper supports lookup, current-scope duplicate detection, shadowing of outer names, and push/pop behavior.

Atomic checkers do not own scope state. They may inspect the prepared stack through lookup and current-scope queries, but they do not push or pop scopes and do not register declarations. The future analyzer runner owns scope creation, declaration registration, routine prototype/definition state, loop-variable lifetime, and cleanup.

## Completed checks

The following checks are implemented and tested at helper level:

- primitive and named type resolution;
- array and record type construction;
- expression typing for the supported expression nodes;
- variable declaration and duplicate detection;
- variable/parameter name resolution;
- initializer inference and compatibility;
- assignment compatibility for simple name targets;
- numeric and array for-loop validation;
- read-only loop variables;
- boolean `if` condition validation;
- boolean `while` condition validation;
- scope stack and `TypeInfo` behavior.

## Routine declaration and call checks

Routine declaration and call checks are atomic. They receive a prepared `ScopeStack` and already resolved `TypeInfo` values. They do not populate scopes, mutate routine symbol state, or traverse the AST. Scope population, declaration ordering, prototype/definition state, and invocation from the complete program traversal belong to the future analyzer runner.

The routine checker currently validates:

- parameter uniqueness and resolved parameter types;
- routine signature compatibility;
- prototype/definition linking;
- duplicate definitions;
- visible routine lookup;
- routine kind and definition presence;
- argument count and argument compatibility.

### Condition checker

`ICheckerCondition` validates an `IfStatement` condition. It checks that the condition exists, resolves its type through `IExpressionTypeResolver`, accepts only `boolean`, and avoids duplicating errors for `errorType`.

`ICheckerWhile` performs the same atomic validation for `WhileLoop` conditions. Both checkers receive a prepared `ScopeStack`; they do not traverse statement bodies or create scopes.

## Pending checks

The following remain atomic tasks before traversal:

- return statement rules;
- member and index assignment targets;
- array-size rules, constant-expression evaluation, and static bounds where possible;
- registration of type declarations and other declaration kinds through a complete symbol-table API.

## Optimizations

Optimizations are a separate AST-mutating stage. The first two planned optimizations are:

1. constant-expression simplification;
2. constant `if`/`while` simplification.

They must be implemented after the semantic foundation and must preserve AST ownership and source spans where practical.

## Verification

The current implementation is verified with:

```text
cmake --build build --clean-first -j2
ctest --test-dir build --output-on-failure
```

The `semantic_base` test passes. The complete analyzer is not yet wired into `runners/runner.cpp`.