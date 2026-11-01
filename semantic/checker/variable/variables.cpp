#include "semantic/checker/variable/variable.h"

using namespace std;

class CheckerVariable : ICheckerVariable {
    CheckerVariable(vector<string>& errors) : ICheckerVariable(errors) {}

    bool CheckerVariable::declareVariable(
        const VariableDeclaration& declaration,
        const TypeInfo* type,
        ScopeStack& scopeStack
    ) {
        if (scopeStack.empty()) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): cannot declare variable '" + declaration.name +
                "' without an active scope"
            );
            return false;
        }

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

        if (!scopeStack.declare(symbol)) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): failed to declare variable '" + declaration.name +
                "'"
            );
            return false;
        }

        return true;
    }

    TypeInfo CheckerVariable::checkNameReference(
        const NameReference& expression,
        ScopeStack& scopeStack
    ) {
        Symbol* symbol = scopeStack.lookup(expression.name);

        if (symbol == nullptr) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): undeclared name '" + expression.name + "'"
            );
            return errorType;
        }

        if (symbol->kind != SymbolKind::Variable &&
            symbol->kind != SymbolKind::Parameter) {
            this->errors.push_back(
                "Semantic error (line " + to_string(expression.span.lineNum) +
                "): '" + expression.name +
                "' is not a variable or parameter"
            );
            return errorType;
        }

        if (symbol->type == nullptr) {
            return errorType;
        }

        return *symbol->type;
    }
};