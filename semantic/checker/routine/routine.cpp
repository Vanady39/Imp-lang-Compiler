#include "semantic/checker/routine/routine.h"

using namespace std;

class CheckerRoutine final : public ICheckerRoutine {
private:
    // Прототип и определение одной routine должны иметь одинаковые типы.
    bool sameSignature(
        const Symbol& symbol,
        const vector<const TypeInfo*>& parameterTypes,
        const TypeInfo* returnType
    ) const {
        if (symbol.parameterTypes.size() != parameterTypes.size()) {
            return false;
        }

        // nullptr означает routine без возвращаемого значения.
        if (returnType == nullptr || symbol.type == nullptr) {
            if (returnType != symbol.type) {
                return false;
            }
        } else if (!sameType(*symbol.type, *returnType)) {
            return false;
        }

        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (parameterTypes[i] == nullptr ||
                symbol.parameterTypes[i] == nullptr ||
                !sameType(*symbol.parameterTypes[i], *parameterTypes[i])) {
                return false;
            }
        }

        return true;
    }

    // Объявление без обоих вариантов тела является прототипом.
    bool isDefinition(const RoutineDeclaration& declaration) const {
        return declaration.body != nullptr ||
            declaration.expressionBody != nullptr;
    }

    // Имена параметров уникальны внутри одного объявления.
    bool checkNonEqualParameterNames(
        const RoutineDeclaration& declaration
    ) {
        vector<string> names;
        for (const auto& parameter : declaration.parameters) {
            for (const string& name : names) {
                if (name == parameter->name) {
                    errors.push_back(
                        "Semantic error (line " +
                        to_string(parameter->span.lineNum) +
                        "): duplicate parameter '" + parameter->name + "'"
                    );
                    return false;
                }
            }
            names.push_back(parameter->name);
        }
        return true;
    }

public:
    explicit CheckerRoutine(vector<string>& errors) : ICheckerRoutine(errors) {}

    bool declareRoutine(
        const RoutineDeclaration& declaration,
        const vector<const TypeInfo*>& parameterTypes,
        const TypeInfo* returnType,
        ScopeStack& scopeStack
    ) override {
        if (scopeStack.empty()) {
            this->errors.push_back(       
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): cannot declare routine '" + declaration.name +
                "' without an active scope"
            );

            return false;
        }

        if (parameterTypes.size() != declaration.parameters.size()) {
            errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): routine '" + declaration.name +
                "' parameter type count does not match declaration"
            );
            return false;
        }

        if (!this->checkNonEqualParameterNames(declaration)) {
            return false;
        }

        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (parameterTypes[i] == nullptr) {
                errors.push_back(
                    "Semantic error (line " +
                    to_string(declaration.parameters[i]->span.lineNum) +
                    "): cannot determine type of parameter '" +
                    declaration.parameters[i]->name + "'"
                );
                return false;
            }
        }

        // Проверяем текущую область: внешнее имя можно затенить.
        Symbol* symbol = scopeStack.currentScopeFind(declaration.name);
        if (symbol == nullptr) {
            Symbol routine{
                declaration.name,
                SymbolKind::Routine,
                declaration.span,
                returnType,
                false,
                {const_cast<RoutineDeclaration*>(&declaration)},
                parameterTypes,
                isDefinition(declaration)
            };
            return scopeStack.declare(routine);
        }

        if (symbol->kind != SymbolKind::Routine) {
            errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): duplicate declaration of '" + declaration.name + "'"
            );
            return false;
        }

        // Повторное имя связываем с той же routine; другую сигнатуру отклоняем.
        if (!sameSignature(
                *symbol,
                parameterTypes,
                returnType
            )) {
            errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): routine '" + declaration.name +
                "' has incompatible signature"
            );
            return false;
        }

        // Прототипы могут повторяться, тело разрешено только одно.
        if (isDefinition(declaration) && symbol->hasDefinition) {
            errors.push_back(
                "Semantic error (line " +
                to_string(declaration.span.lineNum) +
                "): duplicate definition of routine '" +
                declaration.name + "'"
            );
            return false;
        }

        symbol->routineDeclarations.push_back(
            const_cast<RoutineDeclaration*>(&declaration)
        );
        symbol->hasDefinition = symbol->hasDefinition ||
            isDefinition(declaration);
        return true;
    }
};

unique_ptr<ICheckerRoutine> makeCheckerRoutine(vector<string>& errors) {
    return make_unique<CheckerRoutine>(errors);
}
