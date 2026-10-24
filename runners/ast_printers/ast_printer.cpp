#include "runners/ast_printers/ast_printer.h"

#include "lexer/lexer.h"

#include <format>
#include <iostream>
#include <string>

using namespace std;

namespace {

void printNode(const Node* node, int indent);

void printLine(int indent, const string& text) {
    cout << string(indent * 2, ' ') << text << endl;
}

void printChild(int indent, const string& label, const Node* node) {
    printLine(indent, label);
    printNode(node, indent + 1);
}

void printNode(const Node* node, int indent) {
    switch (node->nodeType) {
    case NodeType::IntegerLiteral: {
        auto literal = static_cast<const IntegerLiteral*>(node);
        printLine(indent, format("IntegerLiteral {}", literal->value));
        break;
    }
    case NodeType::RealLiteral: {
        auto literal = static_cast<const RealLiteral*>(node);
        printLine(indent, format("RealLiteral {}", literal->value));
        break;
    }
    case NodeType::BooleanLiteral: {
        auto literal = static_cast<const BooleanLiteral*>(node);
        printLine(indent, format("BooleanLiteral {}", literal->value ? "true" : "false"));
        break;
    }
    case NodeType::UnaryExpression: {
        auto unary = static_cast<const UnaryExpression*>(node);
        printLine(indent, format("UnaryExpression {}", tokenName(unary->op)));
        printNode(unary->operand.get(), indent + 1);
        break;
    }
    case NodeType::BinaryExpression: {
        auto binary = static_cast<const BinaryExpression*>(node);
        printLine(indent, format("BinaryExpression {}", tokenName(binary->op)));
        printNode(binary->left.get(), indent + 1);
        printNode(binary->right.get(), indent + 1);
        break;
    }
    case NodeType::NameReference: {
        auto name = static_cast<const NameReference*>(node);
        printLine(indent, format("Name {}", name->name));
        break;
    }
    case NodeType::MemberAccess: {
        auto access = static_cast<const MemberAccess*>(node);
        printLine(indent, format("MemberAccess .{}", access->member));
        printNode(access->object.get(), indent + 1);
        break;
    }
    case NodeType::IndexAccess: {
        auto access = static_cast<const IndexAccess*>(node);
        printLine(indent, "IndexAccess");
        printNode(access->array.get(), indent + 1);
        printChild(indent + 1, "Index", access->index.get());
        break;
    }
    case NodeType::RoutineCall: {
        auto call = static_cast<const RoutineCall*>(node);
        printLine(indent, format("RoutineCall {}", call->name));
        for (const auto& argument : call->arguments) {
            printNode(argument.get(), indent + 1);
        }
        break;
    }
    case NodeType::PrimitiveType: {
        auto primitive = static_cast<const PrimitiveType*>(node);
        printLine(indent, format("PrimitiveType {}", tokenName(primitive->kind)));
        break;
    }
    case NodeType::NamedType: {
        auto named = static_cast<const NamedType*>(node);
        printLine(indent, format("NamedType {}", named->name));
        break;
    }
    case NodeType::ArrayType: {
        auto array = static_cast<const ArrayType*>(node);
        printLine(indent, array->size ? "ArrayType" : "ArrayType (sizeless)");
        if (array->size) {
            printChild(indent + 1, "Size", array->size.get());
        }
        printChild(indent + 1, "ElementType", array->elementType.get());
        break;
    }
    case NodeType::RecordType: {
        auto record = static_cast<const RecordType*>(node);
        printLine(indent, "RecordType");
        for (const auto& field : record->fields) {
            printNode(field.get(), indent + 1);
        }
        break;
    }
    case NodeType::VariableDeclaration: {
        auto variable = static_cast<const VariableDeclaration*>(node);
        printLine(indent, format("VariableDeclaration {}", variable->name));
        if (variable->type) {
            printChild(indent + 1, "Type", variable->type.get());
        }
        if (variable->initializer) {
            printChild(indent + 1, "Initializer", variable->initializer.get());
        }
        break;
    }
    case NodeType::TypeDeclaration: {
        auto type = static_cast<const TypeDeclaration*>(node);
        printLine(indent, format("TypeDeclaration {}", type->name));
        printNode(type->type.get(), indent + 1);
        break;
    }
    case NodeType::ParameterDeclaration: {
        auto parameter = static_cast<const ParameterDeclaration*>(node);
        printLine(indent, format("Parameter {}", parameter->name));
        printNode(parameter->type.get(), indent + 1);
        break;
    }
    case NodeType::RoutineDeclaration: {
        auto routine = static_cast<const RoutineDeclaration*>(node);
        bool forward = !routine->body && !routine->expressionBody;
        printLine(indent, format("RoutineDeclaration {}{}", routine->name, forward ? " (forward)" : ""));
        for (const auto& parameter : routine->parameters) {
            printNode(parameter.get(), indent + 1);
        }
        if (routine->returnType) {
            printChild(indent + 1, "ReturnType", routine->returnType.get());
        }
        if (routine->body) {
            printNode(routine->body.get(), indent + 1);
        }
        if (routine->expressionBody) {
            printChild(indent + 1, "ExpressionBody", routine->expressionBody.get());
        }
        break;
    }
    case NodeType::Body: {
        auto body = static_cast<const Body*>(node);
        printLine(indent, "Body");
        for (const auto& item : body->items) {
            printNode(item.get(), indent + 1);
        }
        break;
    }
    case NodeType::Assignment: {
        auto assignment = static_cast<const Assignment*>(node);
        printLine(indent, "Assignment");
        printChild(indent + 1, "Target", assignment->target.get());
        printChild(indent + 1, "Value", assignment->value.get());
        break;
    }
    case NodeType::CallStatement: {
        auto call = static_cast<const CallStatement*>(node);
        printLine(indent, "CallStatement");
        printNode(call->call.get(), indent + 1);
        break;
    }
    case NodeType::WhileLoop: {
        auto loop = static_cast<const WhileLoop*>(node);
        printLine(indent, "WhileLoop");
        printChild(indent + 1, "Condition", loop->condition.get());
        printNode(loop->body.get(), indent + 1);
        break;
    }
    case NodeType::ForLoop: {
        auto loop = static_cast<const ForLoop*>(node);
        printLine(indent, format("ForLoop {}{}", loop->variable, loop->reverse ? " (reverse)" : ""));
        if (loop->rangeEnd) {
            printChild(indent + 1, "From", loop->rangeStart.get());
            printChild(indent + 1, "To", loop->rangeEnd.get());
        } else {
            printChild(indent + 1, "Array", loop->rangeStart.get());
        }
        printNode(loop->body.get(), indent + 1);
        break;
    }
    case NodeType::IfStatement: {
        auto ifStatement = static_cast<const IfStatement*>(node);
        printLine(indent, "IfStatement");
        printChild(indent + 1, "Condition", ifStatement->condition.get());
        printChild(indent + 1, "Then", ifStatement->thenBody.get());
        if (ifStatement->elseBody) {
            printChild(indent + 1, "Else", ifStatement->elseBody.get());
        }
        break;
    }
    case NodeType::PrintStatement: {
        auto print = static_cast<const PrintStatement*>(node);
        printLine(indent, "PrintStatement");
        for (const auto& value : print->values) {
            printNode(value.get(), indent + 1);
        }
        break;
    }
    case NodeType::ReturnStatement: {
        auto ret = static_cast<const ReturnStatement*>(node);
        printLine(indent, "ReturnStatement");
        printNode(ret->value.get(), indent + 1);
        break;
    }
    case NodeType::Program: {
        auto program = static_cast<const Program*>(node);
        printLine(indent, "Program");
        for (const auto& declaration : program->declarations) {
            printNode(declaration.get(), indent + 1);
        }
        break;
    }
    }
}

} // namespace

void printProgram(const Program* program) {
    printNode(program, 0);
}
