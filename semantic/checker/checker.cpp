#include "checker.h"

#include "semantic/checker/condition/condition.h"
#include "semantic/checker/for-loop/for.h"
#include "semantic/checker/routine/routine.h"
#include "semantic/checker/return/return.h"
#include "semantic/checker/variable/variable.h"
#include "semantic/checker/while/while.h"
#include "semantic/common/helpers/expression_type.h"
#include "semantic/traversal/ast_traversal.h"
#include "shared/types/constant_integer.h"

using namespace std;

class Checker final : public IChecker, public IAstTraversalObserver {
private:
    struct RoutineContext {
        const RoutineDeclaration& declaration;
        const TypeInfo* result;
        vector<const TypeInfo*> parameters;
    };

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
    vector<RoutineContext> routineStack;
    vector<unique_ptr<TypeInfo>> ownedTypes;
    unordered_map<const ForLoop*, const TypeInfo*> loopTypes;

    // Резолвер хранит ссылки на стек и список ошибок, поэтому обновляем его
    // после создания анализатора и используем один контекст на весь проход.
    void resetResolvers() {
        typeResolver = makeExpressionTypeResolver(*scopeStack, errors);
        conditionChecker = makeCheckerCondition(*scopeStack, errors);
        whileChecker = makeCheckerWhile(*scopeStack, errors);
    }

    void checkArraySizes(const TypeNode& node, bool allowUnsized) {
        if (node.nodeType != NodeType::ArrayType) return;
        const auto& array = static_cast<const ArrayType&>(node);
        const string location = "Semantic error (line " + to_string(node.span.lineNum) + "): ";
        if (array.size) {
            const auto size = constantInteger(*array.size);
            if (!size || *size < 0) {
                errors.push_back(location + "array size must be a non-negative integer constant");
            }
        } else if (!allowUnsized) {
            errors.push_back(location + "only array parameters may omit their size");
        }
        checkArraySizes(*array.elementType, false);
    }

    const TypeInfo* resolveType(const TypeNode* type, bool allowUnsized = false) {
        if (type == nullptr) {
            return nullptr;
        }
        checkArraySizes(*type, allowUnsized);
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
        if (inferredType.kind != TypeKind::Error &&
            variableChecker->declareVariable(declaration, &inferredType, *scopeStack)) {
            // Символ хранит указатель, поэтому сохраняем тип до конца анализа.
            ownedTypes.push_back(make_unique<TypeInfo>(inferredType));
            scopeStack->rememberDeclarationType(declaration, *ownedTypes.back());
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
        checkArraySizes(*declaration.type, false);
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
        if (assignment.target->nodeType == NodeType::MemberAccess) {
            const auto& member = static_cast<const MemberAccess&>(*assignment.target);
            const auto* object = typeResolver->resolve(*member.object);
            if (object->kind == TypeKind::Array && member.member == "length") {
                errors.push_back("Semantic error (line " + to_string(member.span.lineNum) +
                                 "): array length is read-only");
                return;
            }
        }
        if (assignment.target->nodeType != NodeType::NameReference) {
            const TypeInfo* targetType = typeResolver->resolve(*assignment.target);
            if (!isAssignmentCompatible(*targetType, *valueType)) {
                errors.push_back("Semantic error (line " + to_string(assignment.span.lineNum) +
                                 "): assignment target and value have incompatible types");
            }
            return;
        }
        const NameReference* name = dynamic_cast<const NameReference*>(
            assignment.target.get()
        );
        if (name != nullptr) {
            variableChecker->canAssign(*name, *valueType, *scopeStack);
        }
    }

    void checkCallStatement(const CallStatement& statement) {
        checkExpression(*statement.call);
    }

    void checkReturn(const Expression& value) {
        const auto* routine = routineStack.empty() ? nullptr : &routineStack.back();
        returnChecker->checkReturn(value, routine ? &routine->declaration : nullptr,
                                   routine ? routine->result : nullptr, *scopeStack);
    }

    void checkRoutineDefinitions(const Program& program) {
        for (const auto& declaration : program.declarations) {
            if (declaration->nodeType != NodeType::RoutineDeclaration) continue;
            const auto* symbol = scopeStack->currentScopeFind(declaration->name);
            if (symbol && symbol->kind == SymbolKind::Routine && !symbol->hasDefinition &&
                symbol->routineDeclarations.front() == declaration.get()) {
                errors.push_back("Semantic error (line " + to_string(declaration->span.lineNum) +
                                 "): routine '" + declaration->name + "' has no definition");
            }
        }
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
                parameterTypes.push_back(resolveType(parameter->type.get(), true));
            }
            const TypeInfo* returnType = resolveType(declaration.returnType.get());
            registerRoutine(declaration, parameterTypes, returnType);
            routineStack.push_back({declaration, returnType, std::move(parameterTypes)});
            break;
        }
        case NodeType::NameReference: {
            const NameReference& reference =
                static_cast<const NameReference&>(node);
            const auto* symbol = scopeStack->lookup(reference.name);
            if (symbol && symbol->kind == SymbolKind::Routine) {
                checkExpression(reference);
            } else {
                variableChecker->checkNameReference(reference, *scopeStack);
            }
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
        case NodeType::ReturnStatement: {
            const auto& statement = static_cast<const ReturnStatement&>(node);
            if (statement.value) {
                checkReturn(*statement.value);
            } else {
                errors.push_back("Semantic error (line " + to_string(node.span.lineNum) +
                                 "): return value is required");
            }
            break;
        }
        case NodeType::PrimitiveType:
        case NodeType::NamedType:
        case NodeType::ArrayType:
        case NodeType::RecordType:
        case NodeType::VariableDeclaration:
        case NodeType::Body:
        case NodeType::Program:
            break;
        }
    }

    void leave(const Node& node, ScopeStack& currentScopes) override {
        (void)currentScopes;
        if (node.nodeType == NodeType::VariableDeclaration) {
            checkVariable(static_cast<const VariableDeclaration&>(node));
        } else if (node.nodeType == NodeType::RoutineDeclaration) {
            const auto& routine = routineStack.back().declaration;
            if (routine.expressionBody) checkReturn(*routine.expressionBody);
            routineStack.pop_back();
        } else if (node.nodeType == NodeType::Program) {
            checkRoutineDefinitions(static_cast<const Program&>(node));
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

        const auto& context = routineStack.back();
        const auto& parameters = context.declaration.parameters;
        for (size_t i = 0; i < parameters.size(); ++i) {
            const auto& parameter = parameters[i];
            const TypeInfo* type = context.parameters[i];
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
