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
        if (found == initializerScope->end()) return nullptr;
        return &found->second;
    }
    for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end()) return &found->second;
    }
    auto found = module.scope.find(name);
    if (found == module.scope.end()) return nullptr;
    return &found->second;
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
        std::optional<int> size;
        if (array.size) {
            size = constantInteger(*array.size);
        }
        return module.addType({TypeKind::Array, "ptr", resolveType(*array.elementType),
                               size});
    }
    case NodeType::RecordType: {
        const auto& record = static_cast<const RecordType&>(node);
        Type type{TypeKind::Record, "ptr", nullptr, {}, "%imp.record.", {}};
        type.initializerScope = visibleBindings();
        scopes.emplace_back();
        for (const auto& field : record.fields) {
            const Type* fieldType;
            if (field->type) {
                fieldType = resolveType(*field->type);
            } else {
                fieldType = typeOf(*field->initializer);
            }
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
    const Type* type = nullptr;
    if (node.type) {
        type = resolveType(*node.type);
    }

    Value initial;
    if (node.initializer) {
        initial = expression(*node.initializer);
    } else {
        initial = defaultValue(type, node);
    }
    if (!type) {
        type = initial.type;
    }
    if (type == &module.voidType) fail(node, "variable cannot store a procedure result");

    Value storage;
    auto* scope = &scopes.back();
    if (global) {
        storage = {type, module.addGlobal(type)};
        scope = &module.scope;
    } else {
        storage = slot(type);
    }
    store(convert(initial, type, node), storage);
    if (!scope->emplace(node.name, Binding{storage}).second) {
        fail(node, "duplicate declaration of '" + node.name + "'");
    }
}

void Function::typeDeclaration(const TypeDeclaration& node, bool global) {
    const auto* type = resolveType(*node.type);
    auto* scope = &scopes.back();
    if (global) {
        scope = &module.scope;
    }
    if (!scope->emplace(node.name, Binding{{type, {}}, true}).second) {
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

} // namespace codegen
