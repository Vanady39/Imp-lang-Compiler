#ifndef AST_H
#define AST_H

#include "lexer/lexer.h"

#include <memory>
#include <string>
#include <vector>

using namespace std;

enum class NodeType {
    IntegerLiteral,
    RealLiteral,
    BooleanLiteral,
    UnaryExpression,
    BinaryExpression,
    NameReference,
    MemberAccess,
    IndexAccess,
    RoutineCall,

    PrimitiveType,
    NamedType,
    ArrayType,
    RecordType,

    VariableDeclaration,
    TypeDeclaration,
    ParameterDeclaration,
    RoutineDeclaration,

    Body,

    Assignment,
    CallStatement,
    WhileLoop,
    ForLoop,
    IfStatement,
    PrintStatement,
    ReturnStatement,

    Program
};

class Node {
public:
    NodeType nodeType;
    Span span;

    Node(NodeType nodeType, Span span) : nodeType(nodeType), span(span) {}
    virtual ~Node() = default;
};

using NodePtr = unique_ptr<Node>;

class Expression : public Node {
public:
    using Node::Node;
};

using ExpressionPtr = unique_ptr<Expression>;

class IntegerLiteral : public Expression {
public:
    int value;

    IntegerLiteral(Span span, int value)
        : Expression(NodeType::IntegerLiteral, span), value(value) {}
};

class RealLiteral : public Expression {
public:
    double value;

    RealLiteral(Span span, double value)
        : Expression(NodeType::RealLiteral, span), value(value) {}
};

class BooleanLiteral : public Expression {
public:
    bool value;

    BooleanLiteral(Span span, bool value)
        : Expression(NodeType::BooleanLiteral, span), value(value) {}
};

class UnaryExpression : public Expression {
public:
    TokenCode op;
    ExpressionPtr operand;

    UnaryExpression(Span span, TokenCode op, ExpressionPtr operand)
        : Expression(NodeType::UnaryExpression, span), op(op), operand(std::move(operand)) {}
};

class BinaryExpression : public Expression {
public:
    TokenCode op;
    ExpressionPtr left;
    ExpressionPtr right;

    BinaryExpression(Span span, TokenCode op, ExpressionPtr left, ExpressionPtr right)
        : Expression(NodeType::BinaryExpression, span), op(op), left(std::move(left)), right(std::move(right)) {}
};

class NameReference : public Expression {
public:
    string name;

    NameReference(Span span, string name)
        : Expression(NodeType::NameReference, span), name(name) {}
};

class MemberAccess : public Expression {
public:
    ExpressionPtr object;
    string member;

    MemberAccess(Span span, ExpressionPtr object, string member)
        : Expression(NodeType::MemberAccess, span), object(std::move(object)), member(member) {}
};

class IndexAccess : public Expression {
public:
    ExpressionPtr array;
    ExpressionPtr index;

    IndexAccess(Span span, ExpressionPtr array, ExpressionPtr index)
        : Expression(NodeType::IndexAccess, span), array(std::move(array)), index(std::move(index)) {}
};

class RoutineCall : public Expression {
public:
    string name;
    vector<ExpressionPtr> arguments;

    RoutineCall(Span span, string name, vector<ExpressionPtr> arguments)
        : Expression(NodeType::RoutineCall, span), name(name), arguments(std::move(arguments)) {}
};

class TypeNode : public Node {
public:
    using Node::Node;
};

using TypePtr = unique_ptr<TypeNode>;

class PrimitiveType : public TypeNode {
public:
    TokenCode kind;

    PrimitiveType(Span span, TokenCode kind)
        : TypeNode(NodeType::PrimitiveType, span), kind(kind) {}
};

class NamedType : public TypeNode {
public:
    string name;

    NamedType(Span span, string name)
        : TypeNode(NodeType::NamedType, span), name(name) {}
};

class ArrayType : public TypeNode {
public:
    ExpressionPtr size;
    TypePtr elementType;

    ArrayType(Span span, ExpressionPtr size, TypePtr elementType)
        : TypeNode(NodeType::ArrayType, span), size(std::move(size)), elementType(std::move(elementType)) {}
};

class VariableDeclaration;

class RecordType : public TypeNode {
public:
    vector<unique_ptr<VariableDeclaration>> fields;

    RecordType(Span span, vector<unique_ptr<VariableDeclaration>> fields)
        : TypeNode(NodeType::RecordType, span), fields(std::move(fields)) {}
};

class Declaration : public Node {
public:
    string name;

    Declaration(NodeType nodeType, Span span, string name)
        : Node(nodeType, span), name(name) {}
};

using DeclarationPtr = unique_ptr<Declaration>;

class VariableDeclaration : public Declaration {
public:
    TypePtr type;
    ExpressionPtr initializer;

    VariableDeclaration(Span span, string name, TypePtr type, ExpressionPtr initializer)
        : Declaration(NodeType::VariableDeclaration, span, name), type(std::move(type)),
          initializer(std::move(initializer)) {}
};

class TypeDeclaration : public Declaration {
public:
    TypePtr type;

    TypeDeclaration(Span span, string name, TypePtr type)
        : Declaration(NodeType::TypeDeclaration, span, name), type(std::move(type)) {}
};

class ParameterDeclaration : public Declaration {
public:
    TypePtr type;

    ParameterDeclaration(Span span, string name, TypePtr type)
        : Declaration(NodeType::ParameterDeclaration, span, name), type(std::move(type)) {}
};

class Body : public Node {
public:
    vector<NodePtr> items;

    Body(Span span, vector<NodePtr> items)
        : Node(NodeType::Body, span), items(std::move(items)) {}
};

using BodyPtr = unique_ptr<Body>;

class RoutineDeclaration : public Declaration {
public:
    vector<unique_ptr<ParameterDeclaration>> parameters;
    TypePtr returnType;
    BodyPtr body;
    ExpressionPtr expressionBody;

    RoutineDeclaration(Span span, string name, vector<unique_ptr<ParameterDeclaration>> parameters,
                       TypePtr returnType, BodyPtr body, ExpressionPtr expressionBody)
        : Declaration(NodeType::RoutineDeclaration, span, name), parameters(std::move(parameters)),
          returnType(std::move(returnType)), body(std::move(body)), expressionBody(std::move(expressionBody)) {}
};

class Statement : public Node {
public:
    using Node::Node;
};

using StatementPtr = unique_ptr<Statement>;

class Assignment : public Statement {
public:
    ExpressionPtr target;
    ExpressionPtr value;

    Assignment(Span span, ExpressionPtr target, ExpressionPtr value)
        : Statement(NodeType::Assignment, span), target(std::move(target)), value(std::move(value)) {}
};

class CallStatement : public Statement {
public:
    unique_ptr<RoutineCall> call;

    CallStatement(Span span, unique_ptr<RoutineCall> call)
        : Statement(NodeType::CallStatement, span), call(std::move(call)) {}
};

class WhileLoop : public Statement {
public:
    ExpressionPtr condition;
    BodyPtr body;

    WhileLoop(Span span, ExpressionPtr condition, BodyPtr body)
        : Statement(NodeType::WhileLoop, span), condition(std::move(condition)), body(std::move(body)) {}
};

class ForLoop : public Statement {
public:
    string variable;
    ExpressionPtr rangeStart;
    ExpressionPtr rangeEnd;
    bool reverse;
    BodyPtr body;

    ForLoop(Span span, string variable, ExpressionPtr rangeStart, ExpressionPtr rangeEnd,
            bool reverse, BodyPtr body)
        : Statement(NodeType::ForLoop, span), variable(variable), rangeStart(std::move(rangeStart)),
          rangeEnd(std::move(rangeEnd)), reverse(reverse), body(std::move(body)) {}
};

class IfStatement : public Statement {
public:
    ExpressionPtr condition;
    BodyPtr thenBody;
    BodyPtr elseBody;

    IfStatement(Span span, ExpressionPtr condition, BodyPtr thenBody, BodyPtr elseBody)
        : Statement(NodeType::IfStatement, span), condition(std::move(condition)),
          thenBody(std::move(thenBody)), elseBody(std::move(elseBody)) {}
};

class PrintStatement : public Statement {
public:
    vector<ExpressionPtr> values;

    PrintStatement(Span span, vector<ExpressionPtr> values)
        : Statement(NodeType::PrintStatement, span), values(std::move(values)) {}
};

class ReturnStatement : public Statement {
public:
    ExpressionPtr value;

    ReturnStatement(Span span, ExpressionPtr value)
        : Statement(NodeType::ReturnStatement, span), value(std::move(value)) {}
};

class Program : public Node {
public:
    vector<DeclarationPtr> declarations;

    Program(Span span, vector<DeclarationPtr> declarations)
        : Node(NodeType::Program, span), declarations(std::move(declarations)) {}
};

#endif
