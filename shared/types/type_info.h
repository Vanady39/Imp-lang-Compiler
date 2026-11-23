#ifndef SHARED_TYPES_TYPE_INFO_H
#define SHARED_TYPES_TYPE_INFO_H

#include "lexer/lexer.h"
#include "parser/ast.h"

#include <optional>

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
    const TypeDeclaration* arrayDeclaration = nullptr;
    std::optional<int> arraySize{};
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
inline TypeInfo arrayType(const TypeInfo& elementType, std::optional<int> size = std::nullopt,
                          const TypeDeclaration* declaration = nullptr) {
    return TypeInfo{TypeKind::Array, nullptr, &elementType, declaration, size};
}

// Named aggregates compare by the identity of their type declaration.
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
               a.arrayDeclaration == b.arrayDeclaration && a.arraySize == b.arraySize &&
               sameType(*a.element, *b.element);
    default:
        return true;
    }
}

inline bool isAssignmentCompatible(const TypeInfo& target, const TypeInfo& value) {
    if (target.kind == TypeKind::Error || value.kind == TypeKind::Error) return true;
    if (sameType(target, value)) return true;

    switch (target.kind) {
    case TypeKind::Integer:
        return value.kind == TypeKind::Real || value.kind == TypeKind::Boolean;
    case TypeKind::Real:
        return value.kind == TypeKind::Integer || value.kind == TypeKind::Boolean;
    case TypeKind::Boolean:
        return value.kind == TypeKind::Integer;
    case TypeKind::Array:
        // A sizeless parameter retains the caller's array and its actual length.
        return !target.arraySize && value.kind == TypeKind::Array &&
               target.element && value.element && sameType(*target.element, *value.element);
    default:
        return false;
    }
}

#endif
