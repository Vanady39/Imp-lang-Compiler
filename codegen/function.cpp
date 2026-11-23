#include "codegen/emitter.h"
#include "shared/types/constant_integer.h"

#include <utility>

namespace codegen {

Function::Function(Module& module, const Type* result)
    : module(module), result(result), scopes(1) {}

void Function::emit(const std::string& line) {
    instructions << "  " << line << '\n';
}

std::string Function::label(const std::string& name) {
    return name + "." + std::to_string(nextId++);
}

std::string Function::temporary(const std::string& instruction) {
    const auto name = "%" + label("v");
    emit(name + " = " + instruction);
    return name;
}

void Function::block(const std::string& name) {
    instructions << name << ":\n";
    terminated = false;
}

void Function::terminate(const std::string& instruction) {
    emit(instruction);
    terminated = true;
}

void Function::jump(const std::string& target) {
    terminate("br label %" + target);
}

void Function::branch(const std::string& condition, const std::string& yes,
                      const std::string& no) {
    terminate("br i1 " + condition + ", label %" + yes + ", label %" + no);
}

std::string Function::finish(const std::string& signature) {
    return "define " + signature + " {\nentry:\n" + allocations.str() +
           instructions.str() + "}\n\n";
}

const Binding* Function::findBinding(const std::string& name) const {
    if (initializerScope) {
        auto found = initializerScope->find(name);
        return found == initializerScope->end() ? nullptr : &found->second;
    }
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end()) return &found->second;
    }
    auto found = module.scope.find(name);
    return found == module.scope.end() ? nullptr : &found->second;
}

const Binding& Function::lookup(const std::string& name, const Node& node) const {
    if (const auto* binding = findBinding(name)) return *binding;
    fail(node, "unknown name '" + name + "'");
}

Scope Function::visibleBindings() const {
    Scope visible = module.scope;
    for (const auto& scope : scopes) {
        for (const auto& [name, binding] : scope) {
            visible.insert_or_assign(name, binding);
        }
    }
    return visible;
}

Value Function::typeInitializer(const Expression& expr, const Scope& scope) {
    // When allocating a named type, resolve its expressions where the type was declared.
    const auto previous = std::exchange(initializerScope, &scope);
    auto value = expression(expr);
    initializerScope = previous;
    return value;
}

const Type* Function::resolveType(const TypeNode& node) {
    switch (node.nodeType) {
    case NodeType::PrimitiveType:
        switch (static_cast<const PrimitiveType&>(node).kind) {
        case tokenInteger: return &module.integer;
        case tokenReal: return &module.real;
        case tokenBoolean: return &module.boolean;
        default: fail(node, "unknown primitive type");
        }
    case NodeType::NamedType: {
        const auto& named = static_cast<const NamedType&>(node);
        const auto& binding = lookup(named.name, node);
        if (!binding.isType) fail(node, "'" + named.name + "' is not a type");
        return binding.value.type;
    }
    case NodeType::ArrayType: {
        const auto& array = static_cast<const ArrayType&>(node);
        const auto size = array.size ? constantInteger(*array.size) : std::nullopt;
        return module.addType({TypeKind::Array, "ptr", resolveType(*array.elementType),
                               size});
    }
    case NodeType::RecordType: {
        const auto& record = static_cast<const RecordType&>(node);
        Type type{TypeKind::Record, "ptr", nullptr, {}, "%imp.record.", {}};
        type.initializerScope = visibleBindings();
        scopes.emplace_back();
        for (const auto& field : record.fields) {
            const Type* fieldType = field->type ? resolveType(*field->type)
                                                : typeOf(*field->initializer);
            type.fields.push_back({field.get(), fieldType});
            scopes.back().emplace(field->name, Binding{{fieldType, {}}});
        }
        scopes.pop_back();
        return module.addType(std::move(type));
    }
    default:
        fail(node, "unsupported type");
    }
}

Value Function::slot(const Type* type) {
    const auto name = "%" + label("slot");
    // A loop reuses its scalar slots. Aggregate objects still allocate on each declaration execution.
    allocations << "  " << name << " = alloca " << type->ir << '\n';
    return {type, name};
}

Value Function::load(const Value& address) {
    return {address.type, temporary("load " + address.type->ir + ", ptr " + address.ir)};
}

void Function::store(const Value& value, const Value& address) {
    emit("store " + value.typed() + ", ptr " + address.ir);
}

void Function::guard(const std::string& condition) {
    const auto good = label("valid");
    const auto bad = label("trap");
    branch(condition, good, bad);
    block(bad);
    emit("call void @llvm.trap()");
    terminate("unreachable");
    block(good);
}

std::string Function::allocate(const std::string& type, const std::string& count) {
    const auto end = temporary("getelementptr " + type + ", ptr null, i64 1");
    const auto size = temporary("ptrtoint ptr " + end + " to i64");
    const auto bytes = temporary("mul i64 " + size + ", " + count);
    const auto empty = temporary("icmp eq i64 " + bytes + ", 0");
    const auto capacity = temporary("select i1 " + empty + ", i64 1, i64 " + bytes);
    const auto pointer = temporary("call ptr @calloc(i64 1, i64 " + capacity + ")");
    guard(temporary("icmp ne ptr " + pointer + ", null"));
    return pointer;
}

Value Function::recordField(const Value& record, size_t index) {
    guard(temporary("icmp ne ptr " + record.ir + ", null"));
    return {record.type->fields.at(index).type,
            temporary("getelementptr " + record.type->layout + ", ptr " + record.ir +
                      ", i32 0, i32 " + std::to_string(index))};
}

Value Function::arrayLength(const Value& array) {
    guard(temporary("icmp ne ptr " + array.ir + ", null"));
    return load({&module.integer, array.ir});
}

Value Function::arrayElement(const Value& array, const Value& index) {
    const auto length = arrayLength(array);
    const auto above = temporary("icmp sge i32 " + index.ir + ", 1");
    const auto below = temporary("icmp sle i32 " + index.ir + ", " + length.ir);
    guard(temporary("and i1 " + above + ", " + below));
    const auto dataSlot = temporary("getelementptr %imp.array, ptr " + array.ir + ", i32 0, i32 1");
    const auto data = temporary("load ptr, ptr " + dataSlot);
    const auto offset = temporary("sub i32 " + index.ir + ", 1");
    const Type* element = array.type->element;
    return {element, temporary("getelementptr " + element->ir + ", ptr " + data +
                               ", i32 " + offset)};
}

Value Function::defaultValue(const Type* type, const Node& node) {
    if (type->kind == TypeKind::Record) {
        const auto record = Value{type, allocate(type->layout, "1")};
        auto scope = type->initializerScope;
        for (size_t i = 0; i < type->fields.size(); ++i) {
            const auto& field = type->fields[i];
            const auto address = recordField(record, i);
            if (field.declaration->initializer) {
                const auto initial = typeInitializer(*field.declaration->initializer, scope);
                store(convert(initial, field.type, *field.declaration), address);
            }
            scope.insert_or_assign(field.declaration->name, Binding{address});
        }
        return record;
    }
    if (type->kind == TypeKind::Array) {
        if (!type->extent || *type->extent < 0) fail(node, "a new array requires a constant size");
        const auto count = std::to_string(*type->extent);
        const auto array = allocate("%imp.array", "1");
        const auto data = allocate(type->element->ir, count);
        store({&module.integer, count}, {&module.integer, array});
        const auto dataSlot = temporary("getelementptr %imp.array, ptr " + array + ", i32 0, i32 1");
        emit("store ptr " + data + ", ptr " + dataSlot);
        return {type, array};
    }
    return {type, "zeroinitializer"};
}

void Function::variable(const VariableDeclaration& node, bool global) {
    const auto* declared = node.type ? resolveType(*node.type) : nullptr;
    const auto initial = node.initializer ? expression(*node.initializer) : defaultValue(declared, node);
    const auto* type = declared ? declared : initial.type;
    if (type == &module.voidType) fail(node, "variable cannot store a procedure result");
    const auto storage = global ? Value{type, module.addGlobal(type)} : slot(type);
    store(convert(initial, type, node), storage);
    auto& scope = global ? module.scope : scopes.back();
    if (!scope.emplace(node.name, Binding{storage}).second) {
        fail(node, "duplicate declaration of '" + node.name + "'");
    }
}

void Function::typeDeclaration(const TypeDeclaration& node, bool global) {
    const auto* type = resolveType(*node.type);
    auto& scope = global ? module.scope : scopes.back();
    if (!scope.emplace(node.name, Binding{{type, {}}, true}).second) {
        fail(node, "duplicate declaration of '" + node.name + "'");
    }
}

std::string Function::routine(const RoutineDeclaration& node, const Routine& signature) {
    std::string header = "internal " + result->ir + " " + signature.ir + "(";
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        if (i != 0) header += ", ";
        const Value parameter{signature.parameters[i], "%arg." + std::to_string(i)};
        header += parameter.typed();
        const auto storage = slot(parameter.type);
        scopes.back().emplace(node.parameters[i]->name, Binding{storage});
        store(parameter, storage);
    }
    if (node.expressionBody) {
        emitReturn(*node.expressionBody);
    } else {
        body(*node.body);
    }
    if (!terminated) {
        if (result == &module.voidType) {
            terminate("ret void");
        } else {
            emit("call void @llvm.trap()");
            terminate("unreachable");
        }
    }
    return finish(header + ")");
}

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
    const auto no = node.elseBody ? label("if.else") : done;
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
    Value first = expression(*node.rangeStart);
    const bool arrayRange = !node.rangeEnd;
    Value last = arrayRange ? arrayLength(first) : expression(*node.rangeEnd);
    const auto array = first;
    if (arrayRange) {
        first = {&module.integer, "1"};
    } else {
        first = convert(first, &module.integer, *node.rangeStart);
        last = convert(last, &module.integer, *node.rangeEnd);
    }
    if (node.reverse) std::swap(first, last);

    const auto index = slot(&module.integer);
    const auto variable = arrayRange ? slot(array.type->element) : index;
    store(first, index);
    const auto test = label("for.test");
    const auto loop = label("for.body");
    const auto increment = label("for.increment");
    const auto done = label("for.end");
    jump(test);
    block(test);
    const auto iteration = load(index);
    const std::string compare = node.reverse ? "icmp sge i32 " : "icmp sle i32 ";
    branch(temporary(compare + iteration.ir + ", " + last.ir), loop, done);
    block(loop);
    if (arrayRange) store(load(arrayElement(array, iteration)), variable);
    scopes.push_back({{node.variable, Binding{variable}}});
    body(*node.body);
    scopes.pop_back();
    if (!terminated) {
        // Exit before stepping beyond INT_MAX or INT_MIN on the last iteration.
        branch(temporary("icmp eq i32 " + iteration.ir + ", " + last.ir), done, increment);
        block(increment);
        const std::string step = node.reverse ? "sub i32 " : "add i32 ";
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
