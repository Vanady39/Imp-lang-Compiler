#include "codegen/emitter.h"
#include "codegen/runtime_ir.h"

#include <iomanip>
#include <stdexcept>

namespace codegen {

[[noreturn]] void fail(const Node& node, const std::string& message) {
    throw std::runtime_error("Code generation error (line " +
                             std::to_string(node.span.lineNum) + "): " + message);
}

std::string Value::typed() const {
    return type->ir + " " + ir;
}

const Type* Module::addType(Type type) {
    if (!type.layout.empty()) {
        type.layout += std::to_string(types.size());
        globals << type.layout << " = type { ";
        for (size_t i = 0; i < type.fields.size(); ++i) {
            if (i != 0) globals << ", ";
            globals << type.fields[i].type->ir;
        }
        globals << " }\n";
    }
    types.push_back(std::move(type));
    return &types.back();
}

std::string Module::addGlobal(const Type* type) {
    const std::string name = "@imp.global." + std::to_string(scope.size());
    globals << name << " = internal global " << type->ir << " zeroinitializer\n";
    return name;
}

std::string Module::stringConstant(const std::string& value) {
    auto [entry, inserted] = strings.try_emplace(
        value, "@imp.string." + std::to_string(strings.size()));
    if (inserted) {
        globals << entry->second << " = private unnamed_addr constant ["
                << value.size() + 1 << " x i8] c\"";
        for (unsigned char ch : value) {
            globals << '\\' << std::hex << std::uppercase << std::setfill('0')
                    << std::setw(2) << static_cast<unsigned>(ch) << std::dec;
        }
        globals << "\\00\"\n";
    }
    return entry->second;
}

void Module::declareRoutine(Function& global, const RoutineDeclaration& node) {
    Routine signature{"@imp.routine." + std::to_string(routines.size()),
                      node.returnType ? global.resolveType(*node.returnType) : &voidType, {}};
    for (const auto& param : node.parameters) {
        signature.parameters.push_back(global.resolveType(*param->type));
    }
    const auto item = routines.try_emplace(node.name, std::move(signature)).first;
    if (node.body || node.expressionBody) {
        entryNames.push_back(node.name);
        Function function(*this, item->second.result);
        functions << function.routine(node, item->second);
    }
}

std::string Module::generate(const Program& program) {
    Function global(*this, &voidType);
    for (const auto& item : program.declarations) {
        switch (item->nodeType) {
        case NodeType::TypeDeclaration:
            global.typeDeclaration(static_cast<const TypeDeclaration&>(*item), true);
            break;
        case NodeType::VariableDeclaration:
            global.variable(static_cast<const VariableDeclaration&>(*item), true);
            break;
        case NodeType::RoutineDeclaration:
            declareRoutine(global, static_cast<const RoutineDeclaration&>(*item));
            break;
        default:
            fail(*item, "unexpected top-level node");
        }
    }
    global.terminate("ret void");
    functions << global.finish("internal void @imp.initialize()");
    emitEntry();
    std::string ir = "; Imp program\n%imp.array = type { i32, ptr }\n";
    ir += globals.str();
    ir += runtimeIr;
    ir += functions.str();
    return ir;
}

} // namespace codegen

std::string generateLlvmIr(const Program& program) {
    return codegen::Module().generate(program);
}
