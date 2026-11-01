#include "semantic/checker/for-loop/for.h"
#include "semantic/common/helpers/expression_type.h"

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

class CheckerFor final : public ICheckerFor {
public:
    explicit CheckerFor(vector<string>& errors) : ICheckerFor(errors) {}

    bool checkNumberRange(
        const ForLoop& forLoop,
        ScopeStack& scopeStack
    ) override {
        if (scopeStack.empty()) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): cannot declare loop variable '" + forLoop.variable +
                "' without an active scope"
            );
            return false;
        }

        if (scopeStack.currentScopeFind(forLoop.variable) != nullptr) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): duplicate declaration of loop variable '" +
                forLoop.variable + "'"
            );
            return false;
        }

        if (forLoop.rangeStart == nullptr) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): for-loop range must have a start expression"
            );
            return false;
        }

        ExpressionTypeResolver resolver(scopeStack, errors);
        const TypeInfo* startType = resolver.resolve(*forLoop.rangeStart);
        if (startType->kind == TypeKind::Error) {
            return false;
        }
        if (startType->kind != TypeKind::Integer) {
            errors.push_back(
                "Semantic error (line " +
                to_string(forLoop.rangeStart->span.lineNum) +
                "): for-loop start expression must have type 'integer'"
            );
            return false;
        }

        if (forLoop.rangeEnd == nullptr) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): numeric for-loop requires an end expression"
            );
            return false;
        }

        const TypeInfo* endType = resolver.resolve(*forLoop.rangeEnd);
        if (endType->kind == TypeKind::Error) {
            return false;
        }
        if (endType->kind != TypeKind::Integer) {
            errors.push_back(
                "Semantic error (line " +
                to_string(forLoop.rangeEnd->span.lineNum) +
                "): for-loop end expression must have type 'integer'"
            );
            return false;
        }

        Symbol loopVariable{
            forLoop.variable,
            SymbolKind::Variable,
            forLoop.span,
            &integerType,
            true,
            {}
        };

        if (!scopeStack.declare(loopVariable)) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): failed to declare loop variable '" +
                forLoop.variable + "'"
            );
            return false;
        }

        return true;
    }

    bool checkArrayRange(
        const ForLoop& forLoop,
        ScopeStack& scopeStack
    ) override {
        if (scopeStack.empty()) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): cannot declare loop variable '" + forLoop.variable +
                "' without an active scope"
            );
            return false;
        }

        if (scopeStack.currentScopeFind(forLoop.variable) != nullptr) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): duplicate declaration of loop variable '" +
                forLoop.variable + "'"
            );
            return false;
        }

        if (forLoop.rangeStart == nullptr) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): array iteration must have a source expression"
            );
            return false;
        }

        if (forLoop.rangeEnd != nullptr) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): array iteration cannot have a range end"
            );
            return false;
        }

        ExpressionTypeResolver resolver(scopeStack, errors);
        const TypeInfo* sourceType = resolver.resolve(*forLoop.rangeStart);
        if (sourceType->kind == TypeKind::Error) {
            return false;
        }
        if (sourceType->kind != TypeKind::Array || sourceType->element == nullptr) {
            errors.push_back(
                "Semantic error (line " +
                to_string(forLoop.rangeStart->span.lineNum) +
                "): for-loop source has type '" + typeName(*sourceType) +
                "'; expected an array"
            );
            return false;
        }

        Symbol loopVariable{
            forLoop.variable,
            SymbolKind::Variable,
            forLoop.span,
            sourceType->element,
            true,
            {}
        };

        if (!scopeStack.declare(loopVariable)) {
            errors.push_back(
                "Semantic error (line " + to_string(forLoop.span.lineNum) +
                "): failed to declare loop variable '" +
                forLoop.variable + "'"
            );
            return false;
        }

        return true;
    }
};

unique_ptr<ICheckerFor> makeCheckerFor(vector<string>& errors) {
    return make_unique<CheckerFor>(errors);
}
