#include "semantic/checker/variable/variable.h"

using namespace std;

static string typeName(const TypeInfo& type) {
    switch (type.kind) {
    case TypeKind::Integer:
        return "integer";
    case TypeKind::Real:
        return "real";
    case TypeKind::Boolean:
        return "boolean";
    case TypeKind::Record:
        return "record";
    case TypeKind::Array:
        return "array";
    case TypeKind::Error:
        return "error";
    }

    return "unknown";
}

class CheckerVariable final : public ICheckerVariable {
private:
    bool checkAssignmentTarget(
        const NameReference& expression,
        ScopeStack& scopeStack
    ) {
        Symbol* symbol = scopeStack.lookup(expression.name);

        if (symbol == nullptr) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): undeclared name '" + expression.name + "'"
            );

            return false;
        }

        if (symbol->kind != SymbolKind::Variable &&
            symbol->kind != SymbolKind::Parameter) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): assignment target '" + expression.name +
                "' is not a variable or parameter"
            );
            return false;
        }

        if (symbol->readOnly) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): assignment target '" + expression.name +
                "' is read-only"
            );
            return false;
        }

        return true;
    }

public:
    explicit CheckerVariable(vector<string>& errors)
        : ICheckerVariable(errors) {}

    bool declareVariable(
        const VariableDeclaration& declaration,
        const TypeInfo* type,
        ScopeStack& scopeStack
    ) override {
        // проверяем что стек не пустой
        if (scopeStack.empty()) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): cannot declare variable '" + declaration.name +
                "' without an active scope"
            );
            return false;
        }

        // проверем что в текущем стеке нет сущности с этим именем
        if (scopeStack.currentScopeFind(declaration.name) != nullptr) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): duplicate declaration of '" + declaration.name + "'"
            );
            return false;
        }

        Symbol symbol{
            declaration.name,
            SymbolKind::Variable,
            declaration.span,
            type,
            false,
            {}
        };

        // деклариурем перемнную. при ошибке кидаем эррор
        if (!scopeStack.declare(symbol)) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): failed to declare variable '" + declaration.name + "'"
            );
            return false;
        }

        return true;
    }

    TypeInfo checkNameReference(
        const NameReference& expression,
        ScopeStack& scopeStack
    ) override {
        Symbol* symbol = scopeStack.lookup(expression.name);

        // есть переменной не существует, то дропаем ошибку 
        if (symbol == nullptr) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): undeclared name '" + expression.name + "'"
            );
            return errorType;
        }

        // очев
        if (symbol->kind != SymbolKind::Variable && symbol->kind != SymbolKind::Parameter) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): '" + expression.name +
                "' is not a variable or parameter"
            );
            return errorType;
        }

        // тоже очевидно
        if (symbol->type == nullptr) {
            return errorType;
        }

        return *symbol->type;
    }

    bool canAssign(
        const NameReference& expression,
        const TypeInfo& valueType,
        ScopeStack& scopeStack
    ) override {
        if (!checkAssignmentTarget(expression, scopeStack)) {
            return false;
        }

        Symbol* symbol = scopeStack.lookup(expression.name);

        if (symbol->type == nullptr) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): cannot determine the type of assignment target '" +
                expression.name + "'"
            );
            return false;
        }

        if (valueType.kind == TypeKind::Error ||
            symbol->type->kind == TypeKind::Error) {
            return true;
        }

        const TypeInfo& targetType = *symbol->type;

        if (sameType(targetType, valueType)) {
            return true;
        }

        if ((targetType.kind == TypeKind::Integer &&
             valueType.kind == TypeKind::Real) ||
            (targetType.kind == TypeKind::Real &&
             valueType.kind == TypeKind::Integer)) {
            return true;
        }

        this->errors.push_back(
            "Semantic error (line " + to_string(expression.span.lineNum) +
            "): cannot assign value of type '" + typeName(valueType) +
            "' to target of type '" + typeName(targetType) + "'"
        );

        return false;
    }
};