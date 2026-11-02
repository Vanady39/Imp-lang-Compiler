#ifndef SEMANTIC_TYPES_SYMBOL_H
#define SEMANTIC_TYPES_SYMBOL_H

#include "lexer/lexer.h"
#include "parser/ast.h"
#include "shared/types/type_info.h"

#include <string>
#include <vector>

using namespace std;

enum class SymbolKind {
    Variable,
    Parameter,
    Routine,
    Type
};

struct Symbol {
    string name; // <- имя декларируемой сущности
    SymbolKind kind; // <- тип символа
    Span span; // <- позиция (строка, столбец) в source code
    const TypeInfo* type; // <- тип сущности
    bool readOnly; // <- имутебелен ли? 
    vector<RoutineDeclaration*> routineDeclarations; // <- список объявлений
    vector<const TypeInfo*> parameterTypes;
    bool hasDefinition = false;
};

#endif
