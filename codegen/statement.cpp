#include "codegen/emitter.h"

#include <utility>

namespace codegen {

void Function::body(const Body& node) {
    scopes.emplace_back();
    for (const auto& item : node.items) {
        if (terminated) break;
        emitNode(*item);
    }
    scopes.pop_back();
}

void Function::emitNode(const Node& node) {
    switch (node.nodeType) {
    case NodeType::Body:
        body(static_cast<const Body&>(node));
        break;
    case NodeType::VariableDeclaration:
        variable(static_cast<const VariableDeclaration&>(node));
        break;
    case NodeType::TypeDeclaration:
        typeDeclaration(static_cast<const TypeDeclaration&>(node));
        break;
    case NodeType::Assignment: {
        const auto& assignment = static_cast<const Assignment&>(node);
        const auto target = address(*assignment.target);
        store(convert(expression(*assignment.value), target.type, node), target);
        break;
    }
    case NodeType::CallStatement:
        call(*static_cast<const CallStatement&>(node).call);
        break;
    case NodeType::IfStatement:
        emitIf(static_cast<const IfStatement&>(node));
        break;
    case NodeType::WhileLoop:
        emitWhile(static_cast<const WhileLoop&>(node));
        break;
    case NodeType::ForLoop:
        emitFor(static_cast<const ForLoop&>(node));
        break;
    case NodeType::PrintStatement:
        emitPrint(static_cast<const PrintStatement&>(node));
        break;
    case NodeType::ReturnStatement:
        emitReturn(*static_cast<const ReturnStatement&>(node).value);
        break;
    default:
        fail(node, "unsupported statement");
    }
}

void Function::emitIf(const IfStatement& node) {
    const auto yes = label("if.then");
    const auto done = label("if.end");
    auto no = done;
    if (node.elseBody) {
        no = label("if.else");
    }
    const auto condition = convert(expression(*node.condition), &module.boolean, *node.condition);
    branch(condition.ir, yes, no);
    block(yes);
    body(*node.thenBody);
    const bool thenReturns = terminated;
    if (!terminated) jump(done);

    if (node.elseBody) {
        block(no);
        body(*node.elseBody);
        if (terminated && thenReturns) return;
        if (!terminated) jump(done);
    }
    block(done);
}

void Function::emitWhile(const WhileLoop& node) {
    const auto test = label("while.test");
    const auto loop = label("while.body");
    const auto done = label("while.end");
    jump(test);
    block(test);
    const auto condition = convert(expression(*node.condition), &module.boolean, *node.condition);
    branch(condition.ir, loop, done);
    block(loop);
    body(*node.body);
    if (!terminated) jump(test);
    block(done);
}

void Function::emitFor(const ForLoop& node) {
    // Read the range once, before introducing the loop variable.
    Value first = expression(*node.rangeStart);
    const bool arrayRange = !node.rangeEnd;
    const auto array = first;
    Value last;
    if (arrayRange) {
        last = arrayLength(array);
        first = {&module.integer, "1"};
    } else {
        last = expression(*node.rangeEnd);
        first = convert(first, &module.integer, *node.rangeStart);
        last = convert(last, &module.integer, *node.rangeEnd);
    }
    if (node.reverse) std::swap(first, last);

    const auto index = slot(&module.integer);
    auto variable = index;
    if (arrayRange) {
        variable = slot(array.type->element);
    }
    store(first, index);
    const auto test = label("for.test");
    const auto loop = label("for.body");
    const auto increment = label("for.increment");
    const auto done = label("for.end");
    jump(test);
    block(test);
    const auto iteration = load(index);
    std::string compare = "icmp sle i32 ";
    if (node.reverse) {
        compare = "icmp sge i32 ";
    }
    branch(temporary(compare + iteration.ir + ", " + last.ir), loop, done);
    block(loop);
    // A numeric loop exposes its index. An array loop exposes the element.
    if (arrayRange) store(load(arrayElement(array, iteration)), variable);
    scopes.push_back({{node.variable, Binding{variable}}});
    body(*node.body);
    scopes.pop_back();
    if (!terminated) {
        // Exit before stepping beyond INT_MAX or INT_MIN on the last iteration.
        branch(temporary("icmp eq i32 " + iteration.ir + ", " + last.ir), done, increment);
        block(increment);
        std::string step = "add i32 ";
        if (node.reverse) {
            step = "sub i32 ";
        }
        store({&module.integer, temporary(step + iteration.ir + ", 1")}, index);
        jump(test);
    }
    block(done);
}

void Function::emitPrint(const PrintStatement& node) {
    for (size_t i = 0; i < node.values.size(); ++i) {
        auto value = expression(*node.values[i]);
        if (i != 0) emit("call i32 @putchar(i32 32)");
        const char* format = "%d";
        switch (value.type->kind) {
        case TypeKind::Boolean:
            value = convert(value, &module.integer, *node.values[i]);
            break;
        case TypeKind::Real:
            format = "%.17g";
            break;
        case TypeKind::Integer:
            break;
        case TypeKind::Array:
        case TypeKind::Record:
            format = "%p";
            break;
        default:
            fail(*node.values[i], "cannot print a procedure result");
        }
        emit("call i32 (ptr, ...) @printf(ptr " + module.stringConstant(format) +
             ", " + value.typed() + ")");
    }
    emit("call i32 @putchar(i32 10)");
}

void Function::emitReturn(const Expression& expr) {
    const auto value = convert(expression(expr), result, expr);
    terminate("ret " + value.typed());
}

} // namespace codegen
