#include "checker.h"

#include "semantic/checker/condition/condition.h"
#include "semantic/checker/for-loop/for.h"
#include "semantic/checker/routine/routine.h"
#include "semantic/checker/return/return.h"
#include "semantic/checker/variable/variable.h"
#include "semantic/checker/while/while.h"
#include "semantic/common/helpers/expression_type.h"
#include "semantic/traversal/ast_traversal.h"

using namespace std;

class Checker final : public IChecker, public IAstTraversalObserver {
private:
    vector<string> errors;
    ScopeStack* scopeStack = nullptr;
    CheckResult result;
    unique_ptr<ICheckerVariable> variableChecker;
    unique_ptr<ICheckerRoutine> routineChecker;
    unique_ptr<ICheckerFor> forChecker;
    unique_ptr<ICheckerReturn> returnChecker;
    unique_ptr<ICheckerCondition> conditionChecker;
    unique_ptr<ICheckerWhile> whileChecker;
    unique_ptr<IExpressionTypeResolver> typeResolver;
    vector<const RoutineDeclaration*> routineStack;
    vector<unique_ptr<TypeInfo>> ownedTypes;
    unordered_map<const ForLoop*, const TypeInfo*> loopTypes;

    // Резолвер хранит ссылки на стек и список ошибок, поэтому обновляем его
    // после создания анализатора и используем один контекст на весь проход.
    void resetResolvers() {
        typeResolver = makeExpressionTypeResolver(*scopeStack, errors);
        conditionChecker = makeCheckerCondition(*scopeStack, errors);
        whileChecker = makeCheckerWhile(*scopeStack, errors);
    }

    const TypeInfo* resolveType(const TypeNode* type) {
        if (type == nullptr) {
            return nullptr;
        }
        return typeResolver->resolveType(*type);
    }

    void registerSymbol(
        const string& name,
        SymbolKind kind,
        const Span& span,
        const TypeInfo* type,
        bool readOnly = false
    ) {
        // Проверка конфликта выполняется атомарным checker-сервисом до
        // добавления символа. Здесь остаётся только сама регистрация.
        if (scopeStack->empty()) {
            return;
        }
        Symbol symbol{name, kind, span, type, readOnly, {}};
        scopeStack->declare(symbol);
    }

    bool registerRoutine(
        const RoutineDeclaration& declaration,
        const vector<const TypeInfo*>& parameterTypes,
        const TypeInfo* returnType
    ) {
        if (!routineChecker->checkDeclareRoutine(
                declaration,
                parameterTypes,
                returnType,
                *scopeStack
            )) {
            return false;
        }

        Symbol* existing = scopeStack->currentScopeFind(declaration.name);
        bool definition = declaration.body != nullptr ||
            declaration.expressionBody != nullptr;

        if (existing == nullptr) {
            // Регистрируем routine сразу после проверки её сигнатуры, чтобы
            // последующие вызовы и рекурсивные обращения её видели.
            Symbol symbol{
                declaration.name,
                SymbolKind::Routine,
                declaration.span,
                returnType,
                false,
                {const_cast<RoutineDeclaration*>(&declaration)},
                parameterTypes,
                definition
            };
            scopeStack->declare(symbol);
            return true;
        }

        existing->routineDeclarations.push_back(
            const_cast<RoutineDeclaration*>(&declaration)
        );
        existing->hasDefinition = existing->hasDefinition || definition;
        return true;
    }

    // Выражение разрешается в текущем scope до проверки родительской
    // конструкции, поэтому ошибки имени появляются в исходной позиции.
    void checkExpression(const Expression& expression) {
        const TypeInfo* type = typeResolver->resolve(expression);
        if (type != nullptr) {
            result.expressionTypes[const_cast<Expression*>(&expression)] = *type;
        }

        if (expression.nodeType != NodeType::RoutineCall) {
            return;
        }

        const RoutineCall& call = static_cast<const RoutineCall&>(expression);
        vector<const TypeInfo*> argumentTypes;
        for (const auto& argument : call.arguments) {
            const TypeInfo* argumentType = typeResolver->resolve(*argument);
            argumentTypes.push_back(argumentType);
        }
        routineChecker->checkCallRoutine(call, argumentTypes, *scopeStack);
    }

    // Объявление проверяется до регистрации: так сохраняется правило
    // объявления до использования и обнаруживаются дубликаты текущего scope.
    void checkVariable(const VariableDeclaration& declaration) {
        const TypeInfo* declaredType = resolveType(declaration.type.get());
        const TypeInfo* initializerType = nullptr;
        if (declaration.initializer != nullptr) {
            initializerType = typeResolver->resolve(*declaration.initializer);
        }

        TypeInfo inferredType = variableChecker->checkInitializer(
            declaration,
            declaredType,
            initializerType
        );
        if (inferredType.kind != TypeKind::Error) {
            // Символ хранит указатель, поэтому сохраняем тип до конца анализа.
            ownedTypes.push_back(make_unique<TypeInfo>(inferredType));
            registerSymbol(
                declaration.name,
                SymbolKind::Variable,
                declaration.span,
                ownedTypes.back().get()
            );
        }
    }

    // Именованный тип должен попасть в таблицу до обхода следующих
    // деклараций, иначе NamedType не сможет его разрешить.
    void checkType(const TypeDeclaration& declaration) {
        const TypeInfo* type = typeResolver->resolveType(
            *declaration.type,
            &declaration
        );
        if (type->kind != TypeKind::Error) {
            registerSymbol(
                declaration.name,
                SymbolKind::Type,
                declaration.span,
                type
            );
        }
    }

    void checkAssignment(const Assignment& assignment) {
        const TypeInfo* valueType = typeResolver->resolve(*assignment.value);
        if (assignment.target->nodeType != NodeType::NameReference) {
            return;
        }
        const NameReference* name = dynamic_cast<const NameReference*>(
            assignment.target.get()
        );
        if (name != nullptr) {
            variableChecker->canAssign(*name, *valueType, *scopeStack);
        }
        // Проверки составных целей будут подключены отдельным checker-сервисом.
    }

    void checkCallStatement(const CallStatement& statement) {
        checkExpression(*statement.call);
    }

public:
    Checker()
        : scopeStack(nullptr),
          variableChecker(makeCheckerVariable(errors)),
          routineChecker(makeCheckerRoutine(errors)),
          forChecker(makeCheckerFor(errors)),
          returnChecker(makeCheckerReturn(errors)) {}

    CheckResult check(Program& program) override {
        errors.clear();
        result = {};
        // ScopeStack живёт дольше traversal, потому что checker-сервисы
        // хранят ссылки на него во время всего анализа.
        ScopeStack analysisScopes;
        scopeStack = &analysisScopes;
        routineStack.clear();
        ownedTypes.clear();
        loopTypes.clear();
        resetResolvers();

        unique_ptr<IAstTraversal> traversal = makeAstTraversal(*scopeStack);
        traversal->traverse(program, *this);
        return result;
    }

    const vector<string>& get_errors() const override {
        return errors;
    }

    void enter(const Node& node, ScopeStack& currentScopes) override {
        (void)currentScopes;
        switch (node.nodeType) {
        case NodeType::VariableDeclaration:
            checkVariable(static_cast<const VariableDeclaration&>(node));
            break;
        case NodeType::TypeDeclaration:
            checkType(static_cast<const TypeDeclaration&>(node));
            break;
        case NodeType::ParameterDeclaration:
            break;
        case NodeType::RoutineDeclaration: {
            const RoutineDeclaration& declaration =
                static_cast<const RoutineDeclaration&>(node);
            vector<const TypeInfo*> parameterTypes;
            for (const auto& parameter : declaration.parameters) {
                parameterTypes.push_back(resolveType(parameter->type.get()));
            }
            const TypeInfo* returnType = resolveType(declaration.returnType.get());
            registerRoutine(declaration, parameterTypes, returnType);
            routineStack.push_back(&declaration);
            break;
        }
        case NodeType::NameReference: {
            const NameReference& reference =
                static_cast<const NameReference&>(node);
            variableChecker->checkNameReference(reference, *scopeStack);
            break;
        }
        case NodeType::Assignment:
            checkAssignment(static_cast<const Assignment&>(node));
            break;
        case NodeType::CallStatement:
            checkCallStatement(static_cast<const CallStatement&>(node));
            break;
        case NodeType::IfStatement:
            conditionChecker->checkCondition(
                static_cast<const IfStatement&>(node)
            );
            break;
        case NodeType::WhileLoop:
            whileChecker->checkCondition(
                static_cast<const WhileLoop&>(node)
            );
            break;
        case NodeType::ForLoop: {
            const ForLoop& loop = static_cast<const ForLoop&>(node);
            bool valid = loop.rangeEnd != nullptr
                ? forChecker->checkNumberRange(loop, *scopeStack)
                : forChecker->checkArrayRange(loop, *scopeStack);
            if (valid) {
                const TypeInfo* loopType = &integerType;
                if (loop.rangeEnd == nullptr) {
                    const TypeInfo* sourceType = typeResolver->resolve(
                        *loop.rangeStart
                    );
                    loopType = sourceType->element;
                }
                loopTypes[&loop] = loopType;
            }
            break;
        }
        case NodeType::IntegerLiteral:
        case NodeType::RealLiteral:
        case NodeType::BooleanLiteral:
        case NodeType::UnaryExpression:
        case NodeType::BinaryExpression:
        case NodeType::MemberAccess:
        case NodeType::IndexAccess:
        case NodeType::RoutineCall:
            checkExpression(static_cast<const Expression&>(node));
            break;
        case NodeType::PrintStatement:
            // Для print отдельного семантического правила пока нет.
            break;
        case NodeType::ReturnStatement:
            returnChecker->checkReturn(
                static_cast<const ReturnStatement&>(node),
                routineStack.empty() ? nullptr : routineStack.back(),
                *scopeStack
            );
            break;
        case NodeType::PrimitiveType:
        case NodeType::NamedType:
        case NodeType::ArrayType:
        case NodeType::RecordType:
        case NodeType::Body:
        case NodeType::Program:
            break;
        }
    }

    void leave(const Node& node, ScopeStack& currentScopes) override {
        (void)currentScopes;
        if (node.nodeType == NodeType::RoutineDeclaration &&
            !routineStack.empty()) {
            routineStack.pop_back();
        }
    }

    void enterScope(const Node& node, ScopeStack& currentScopes) override {
        (void)currentScopes;
        if (node.nodeType == NodeType::ForLoop) {
            // Диапазон проверен до входа в scope, loop variable уже можно
            // сделать read-only для тела цикла.
            const ForLoop& loop = static_cast<const ForLoop&>(node);
            const TypeInfo* loopType = loopTypes[&loop];
            registerSymbol(
                loop.variable,
                SymbolKind::Variable,
                loop.span,
                loopType,
                true
            );
            return;
        }
        if (node.nodeType != NodeType::RoutineDeclaration) {
            return;
        }

        const RoutineDeclaration& routine =
            static_cast<const RoutineDeclaration&>(node);
        for (const auto& parameter : routine.parameters) {
            const TypeInfo* type = resolveType(parameter->type.get());
            if (type != nullptr && type->kind != TypeKind::Error) {
                registerSymbol(
                    parameter->name,
                    SymbolKind::Parameter,
                    parameter->span,
                    type
                );
            }
        }
    }

    void leaveScope(const Node& node, ScopeStack& currentScopes) override {
        (void)node;
        (void)currentScopes;
    }
};

unique_ptr<IChecker> makeChecker() {
    return make_unique<Checker>();
}
