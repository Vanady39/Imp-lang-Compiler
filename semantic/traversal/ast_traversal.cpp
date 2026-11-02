#include "semantic/traversal/ast_traversal.h"

using namespace std;

class AstTraversal final : public IAstTraversal {
private:
    ScopeStack& scopes;
    IAstTraversalObserver* observer = nullptr;

    void enterNode(const Node& node) {
        observer->enter(node, scopes);
    }

    void leaveNode(const Node& node) {
        observer->leave(node, scopes);
    }

    void enterScope(const Node& node) {
        scopes.push();
        observer->enterScope(node, scopes);
    }

    void leaveScope(const Node& node) {
        observer->leaveScope(node, scopes);
        scopes.pop();
    }

    void traverseExpression(const Expression& expression) {
        switch (expression.nodeType) {
        case NodeType::IntegerLiteral:
        case NodeType::RealLiteral:
        case NodeType::BooleanLiteral:
        case NodeType::NameReference:
            break;
        case NodeType::UnaryExpression: {
            const UnaryExpression& unary = static_cast<const UnaryExpression&>(expression);
            traverseNode(unary.operand.get());
            break;
        }
        case NodeType::BinaryExpression: {
            const BinaryExpression& binary = static_cast<const BinaryExpression&>(expression);
            traverseNode(binary.left.get());
            traverseNode(binary.right.get());
            break;
        }
        case NodeType::MemberAccess: {
            const MemberAccess& member = static_cast<const MemberAccess&>(expression);
            traverseNode(member.object.get());
            break;
        }
        case NodeType::IndexAccess: {
            const IndexAccess& index = static_cast<const IndexAccess&>(expression);
            traverseNode(index.array.get());
            traverseNode(index.index.get());
            break;
        }
        case NodeType::RoutineCall: {
            const RoutineCall& call = static_cast<const RoutineCall&>(expression);
            for (const auto& argument : call.arguments) {
                traverseNode(argument.get());
            }
            break;
        }
        case NodeType::PrimitiveType:
        case NodeType::NamedType:
        case NodeType::ArrayType:
        case NodeType::RecordType:
        case NodeType::VariableDeclaration:
        case NodeType::TypeDeclaration:
        case NodeType::ParameterDeclaration:
        case NodeType::RoutineDeclaration:
        case NodeType::Body:
        case NodeType::Assignment:
        case NodeType::CallStatement:
        case NodeType::WhileLoop:
        case NodeType::ForLoop:
        case NodeType::IfStatement:
        case NodeType::PrintStatement:
        case NodeType::ReturnStatement:
        case NodeType::Program:
            break;
        }
    }

    void traverseType(const TypeNode& type) {
        switch (type.nodeType) {
        case NodeType::PrimitiveType:
        case NodeType::NamedType:
            break;
        case NodeType::ArrayType: {
            const ArrayType& array = static_cast<const ArrayType&>(type);
            traverseNode(array.size.get());
            traverseNode(array.elementType.get());
            break;
        }
        case NodeType::RecordType: {
            const RecordType& record = static_cast<const RecordType&>(type);
            for (const auto& field : record.fields) {
                traverseNode(field.get());
            }
            break;
        }
        case NodeType::IntegerLiteral:
        case NodeType::RealLiteral:
        case NodeType::BooleanLiteral:
        case NodeType::UnaryExpression:
        case NodeType::BinaryExpression:
        case NodeType::NameReference:
        case NodeType::MemberAccess:
        case NodeType::IndexAccess:
        case NodeType::RoutineCall:
        case NodeType::VariableDeclaration:
        case NodeType::TypeDeclaration:
        case NodeType::ParameterDeclaration:
        case NodeType::RoutineDeclaration:
        case NodeType::Body:
        case NodeType::Assignment:
        case NodeType::CallStatement:
        case NodeType::WhileLoop:
        case NodeType::ForLoop:
        case NodeType::IfStatement:
        case NodeType::PrintStatement:
        case NodeType::ReturnStatement:
        case NodeType::Program:
            break;
        }
    }

    void traverseDeclaration(const Declaration& declaration) {
        switch (declaration.nodeType) {
        case NodeType::VariableDeclaration: {
            const VariableDeclaration& variable = static_cast<const VariableDeclaration&>(declaration);
            traverseNode(variable.type.get());
            traverseNode(variable.initializer.get());
            break;
        }
        case NodeType::TypeDeclaration: {
            const TypeDeclaration& type = static_cast<const TypeDeclaration&>(declaration);
            traverseNode(type.type.get());
            break;
        }
        case NodeType::ParameterDeclaration: {
            const ParameterDeclaration& parameter = static_cast<const ParameterDeclaration&>(declaration);
            traverseNode(parameter.type.get());
            break;
        }
        case NodeType::RoutineDeclaration: {
            const RoutineDeclaration& routine = static_cast<const RoutineDeclaration&>(declaration);
            for (const auto& parameter : routine.parameters) {
                traverseNode(parameter.get());
            }
            traverseNode(routine.returnType.get());
            traverseNode(routine.body.get());
            traverseNode(routine.expressionBody.get());
            break;
        }
        case NodeType::IntegerLiteral:
        case NodeType::RealLiteral:
        case NodeType::BooleanLiteral:
        case NodeType::UnaryExpression:
        case NodeType::BinaryExpression:
        case NodeType::NameReference:
        case NodeType::MemberAccess:
        case NodeType::IndexAccess:
        case NodeType::RoutineCall:
        case NodeType::PrimitiveType:
        case NodeType::NamedType:
        case NodeType::ArrayType:
        case NodeType::RecordType:
        case NodeType::Body:
        case NodeType::Assignment:
        case NodeType::CallStatement:
        case NodeType::WhileLoop:
        case NodeType::ForLoop:
        case NodeType::IfStatement:
        case NodeType::PrintStatement:
        case NodeType::ReturnStatement:
        case NodeType::Program:
            break;
        }
    }

    void traverseBody(const Body& body) {
        // Любой body отделяет локальные имена от внешнего блока.
        enterScope(body);
        enterNode(body);
        for (const auto& item : body.items) {
            traverseNode(item.get());
        }
        leaveNode(body);
        leaveScope(body);
    }

    void traverseStatement(const Statement& statement) {
        switch (statement.nodeType) {
        case NodeType::Assignment: {
            const Assignment& assignment = static_cast<const Assignment&>(statement);
            traverseNode(assignment.target.get());
            traverseNode(assignment.value.get());
            break;
        }
        case NodeType::CallStatement: {
            const CallStatement& call = static_cast<const CallStatement&>(statement);
            traverseNode(call.call.get());
            break;
        }
        case NodeType::WhileLoop: {
            const WhileLoop& loop = static_cast<const WhileLoop&>(statement);
            traverseNode(loop.condition.get());
            traverseNode(loop.body.get());
            break;
        }
        case NodeType::ForLoop: {
            const ForLoop& loop = static_cast<const ForLoop&>(statement);
            traverseNode(loop.rangeStart.get());
            traverseNode(loop.rangeEnd.get());
            // Loop scope охватывает переменную цикла и его body.
            traverseNode(loop.body.get());
            break;
        }
        case NodeType::IfStatement: {
            const IfStatement& statementNode = static_cast<const IfStatement&>(statement);
            traverseNode(statementNode.condition.get());
            traverseNode(statementNode.thenBody.get());
            traverseNode(statementNode.elseBody.get());
            break;
        }
        case NodeType::PrintStatement: {
            const PrintStatement& print = static_cast<const PrintStatement&>(statement);
            for (const auto& value : print.values) {
                traverseNode(value.get());
            }
            break;
        }
        case NodeType::ReturnStatement: {
            const ReturnStatement& returnStatement = static_cast<const ReturnStatement&>(statement);
            traverseNode(returnStatement.value.get());
            break;
        }
        case NodeType::IntegerLiteral:
        case NodeType::RealLiteral:
        case NodeType::BooleanLiteral:
        case NodeType::UnaryExpression:
        case NodeType::BinaryExpression:
        case NodeType::NameReference:
        case NodeType::MemberAccess:
        case NodeType::IndexAccess:
        case NodeType::RoutineCall:
        case NodeType::PrimitiveType:
        case NodeType::NamedType:
        case NodeType::ArrayType:
        case NodeType::RecordType:
        case NodeType::VariableDeclaration:
        case NodeType::TypeDeclaration:
        case NodeType::ParameterDeclaration:
        case NodeType::RoutineDeclaration:
        case NodeType::Body:
        case NodeType::Program:
            break;
        }
    }

    void traverseNode(const Node* node) {
        if (node == nullptr) {
            return;
        }

        if (node->nodeType == NodeType::Program) {
            const Program& program = static_cast<const Program&>(*node);
            // Глобальная область доступна всем верхнеуровневым декларациям.
            scopes.push();
            enterNode(program);
            for (const auto& declaration : program.declarations) {
                traverseNode(declaration.get());
            }
            leaveNode(program);
            scopes.pop();
            return;
        }

        if (node->nodeType == NodeType::Body) {
            traverseBody(static_cast<const Body&>(*node));
            return;
        }

        // Сначала проверяется сама конструкция во внешней области.
        enterNode(*node);
        if (node->nodeType == NodeType::RoutineDeclaration ||
            node->nodeType == NodeType::ForLoop) {
            // Локальная область нужна только для дочерних узлов.
            enterScope(*node);
        }
        switch (node->nodeType) {
        case NodeType::IntegerLiteral:
        case NodeType::RealLiteral:
        case NodeType::BooleanLiteral:
        case NodeType::UnaryExpression:
        case NodeType::BinaryExpression:
        case NodeType::NameReference:
        case NodeType::MemberAccess:
        case NodeType::IndexAccess:
        case NodeType::RoutineCall:
            traverseExpression(static_cast<const Expression&>(*node));
            break;
        case NodeType::PrimitiveType:
        case NodeType::NamedType:
        case NodeType::ArrayType:
        case NodeType::RecordType:
            traverseType(static_cast<const TypeNode&>(*node));
            break;
        case NodeType::VariableDeclaration:
        case NodeType::TypeDeclaration:
        case NodeType::ParameterDeclaration:
        case NodeType::RoutineDeclaration:
            traverseDeclaration(static_cast<const Declaration&>(*node));
            break;
        case NodeType::Assignment:
        case NodeType::CallStatement:
        case NodeType::WhileLoop:
        case NodeType::ForLoop:
        case NodeType::IfStatement:
        case NodeType::PrintStatement:
        case NodeType::ReturnStatement:
            traverseStatement(static_cast<const Statement&>(*node));
            break;
        case NodeType::Body:
        case NodeType::Program:
            break;
        }
        leaveNode(*node);
        if (node->nodeType == NodeType::RoutineDeclaration ||
            node->nodeType == NodeType::ForLoop) {
            leaveScope(*node);
        }
    }

public:
    explicit AstTraversal(ScopeStack& scopes) : scopes(scopes) {}

    void traverse(
        const Program& program,
        IAstTraversalObserver& traversalObserver
    ) override {
        observer = &traversalObserver;
        traverseNode(&program);
        observer = nullptr;
    }

    const ScopeStack& get_scopes() const override {
        return scopes;
    }
};

unique_ptr<IAstTraversal> makeAstTraversal(ScopeStack& scopes) {
    return make_unique<AstTraversal>(scopes);
}
