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

    // по названию и так очевидно
    bool areCompatible(
        const TypeInfo& targetType,
        const TypeInfo& valueType
    ) const {
        if (targetType.kind == TypeKind::Error || valueType.kind == TypeKind::Error) {
            return true;
        }

        return sameType(targetType, valueType) ||
            (targetType.kind == TypeKind::Integer &&
             valueType.kind == TypeKind::Real) ||
            (targetType.kind == TypeKind::Real &&
             valueType.kind == TypeKind::Integer);
    }

public:
    explicit CheckerRoutine(vector<string>& errors) : ICheckerRoutine(errors) {}

    bool checkDeclareRoutine(
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
            // Сам runner зарегистрирует routine после прохождения проверки.
            // Здесь проверяем только отсутствие конфликта в текущем scope.
            return true;
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

        // Состояние prototype/definition обновит runner при регистрации declaration.
        return true;
    }

    virtual bool checkCallRoutine(
        const RoutineCall& routineCall,
        const vector<const TypeInfo*>& argumentTypes,
        ScopeStack& scopeStack
    ) override {
        Symbol* routine = scopeStack.lookup(routineCall.name);
        if (routine == nullptr) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(routineCall.span.lineNum) +
                "): undeclared routine '" + routineCall.name + "'"
            );
            return false;
        }

        if (routine->kind != SymbolKind::Routine) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(routineCall.span.lineNum) +
                "): '" + routineCall.name + "' is not a routine"
            );
            return false;
        }

        if (routine->parameterTypes.size() != argumentTypes.size()) {
            this->errors.push_back(
                "Semantic error (line " +
                to_string(routineCall.span.lineNum) +
                "): routine '" + routineCall.name +
                "' expects " + to_string(routine->parameterTypes.size()) +
                " argument(s), but " + to_string(argumentTypes.size()) +
                " provided"
            );
            return false;
        }

        for (size_t i = 0; i < argumentTypes.size(); i++) {
            if (routine->parameterTypes[i] == nullptr ||
                argumentTypes[i] == nullptr) {
                this->errors.push_back(
                    "Semantic error (line " +
                    to_string(routineCall.span.lineNum) +
                    "): cannot determine type of argument " +
                    to_string(i + 1) + " in call to routine '" +
                    routineCall.name + "'"
                );
                return false;
            }

            if (!areCompatible(
                    *routine->parameterTypes[i],
                    *argumentTypes[i]
                )) {
                this->errors.push_back(
                    "Semantic error (line " +
                    to_string(routineCall.span.lineNum) +
                    "): argument " + to_string(i + 1) +
                    " of routine '" + routineCall.name +
                    "' has incompatible type"
                );
                return false;
            }
        }

        return true;
    };
};

unique_ptr<ICheckerRoutine> makeCheckerRoutine(vector<string>& errors) {
    return make_unique<CheckerRoutine>(errors);
}
