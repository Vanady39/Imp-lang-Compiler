#ifndef SHARED_TYPES_TYPE_INFO_H
#define SHARED_TYPES_TYPE_INFO_H

#include "lexer/lexer.h"
#include "parser/ast.h"

// Semantic type representation shared by compiler stages (the analyzer now,
// optimizations and code generation later). TypeInfo does not copy the AST
// type declarations: record identity is the TypeDeclaration node itself,
// which gives nominal typing (two identically shaped records stay distinct).
//
// Array size is deliberately not part of the type: DECISIONS.md A5 lets a
// sizeless array parameter receive a sized argument, so sizes are checked
// separately from compatibility.

enum class TypeKind {
    Integer,
    Real,
    Boolean,
    Record,
    Array,
    Error // poison type: suppresses cascading derived errors
};

struct TypeInfo {
    TypeKind kind = TypeKind::Error;
    // kind == Record: the AST node that owns the record type.
    // Named records use TypeDeclaration; anonymous records use RecordType.
    const Node* record = nullptr;
    // kind == Array: the element type.
    const TypeInfo* element = nullptr;
};

inline const TypeInfo integerType{TypeKind::Integer, nullptr, nullptr};
inline const TypeInfo realType{TypeKind::Real, nullptr, nullptr};
inline const TypeInfo booleanType{TypeKind::Boolean, nullptr, nullptr};
inline const TypeInfo errorType{TypeKind::Error, nullptr, nullptr};

inline TypeInfo recordType(const TypeDeclaration& declaration) {
    return TypeInfo{TypeKind::Record, &declaration, nullptr};
}

inline TypeInfo recordType(const RecordType& type) {
    return TypeInfo{TypeKind::Record, &type, nullptr};
}

// elementType must outlive the returned value's use; the analyzer keeps the
// element TypeInfo alive in its cache of declaration types.
inline TypeInfo arrayType(const TypeInfo& elementType) {
    return TypeInfo{TypeKind::Array, nullptr, &elementType};
}

// Structural equality except records, which compare by declaration identity
// (nominal typing, DECISIONS.md section A; test 14 depends on this).
inline bool sameType(const TypeInfo& a, const TypeInfo& b) {
    if (a.kind == TypeKind::Error || b.kind == TypeKind::Error) {
        return false;
    }
    if (a.kind != b.kind) {
        return false;
    }
    switch (a.kind) {
    case TypeKind::Record:
        return a.record != nullptr && a.record == b.record;
    case TypeKind::Array:
        return a.element != nullptr && b.element != nullptr &&
               sameType(*a.element, *b.element);
    default:
        return true;
    }
}

#endif
