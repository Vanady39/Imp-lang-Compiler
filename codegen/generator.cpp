#include "codegen/emitter.h"

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

namespace {

std::string argvAt(Function& function, size_t index) {
    const auto slot = function.temporary("getelementptr ptr, ptr %argv, i64 " +
                                         std::to_string(index));
    return function.temporary("load ptr, ptr " + slot);
}

std::string parseArgument(Function& function, const Type& type, const std::string& text) {
    switch (type.kind) {
    case TypeKind::Integer:
        return function.temporary("call i32 @imp.parse.integer(ptr " + text + ")");
    case TypeKind::Real:
        return function.temporary("call double @imp.parse.real(ptr " + text + ")");
    case TypeKind::Boolean:
        return function.temporary("call i1 @imp.parse.boolean(ptr " + text + ")");
    default:
        throw std::logic_error("cannot parse an aggregate launch argument");
    }
}

bool supportsLaunch(const Routine& routine) {
    for (const auto* type : routine.parameters) {
        if (type->kind == TypeKind::Array || type->kind == TypeKind::Record) return false;
    }
    return true;
}

} // namespace

void Module::emitEntry() {
    Function entry(*this, &integer);
    const auto defaultEntry = entry.label("default");
    const auto explicitEntry = entry.label("select");
    const auto invalid = entry.label("invalid");
    const auto hasName = entry.temporary("icmp sgt i32 %argc, 1");
    entry.branch(hasName, explicitEntry, defaultEntry);

    const Routine* fallback = nullptr;
    for (const auto& name : entryNames) {
        const auto& routine = routines.at(name);
        if (routine.parameters.empty() && (!fallback || name == "test" || name == "main")) {
            fallback = &routine;
            if (name == "main") break;
        }
    }
    entry.block(defaultEntry);
    if (fallback) {
        entry.emit("call void @imp.initialize()");
        entry.emit("call " + fallback->result->ir + " " + fallback->ir + "()");
        entry.terminate("ret i32 0");
    } else {
        entry.jump(invalid);
    }

    entry.block(explicitEntry);
    const auto requested = argvAt(entry, 1);
    for (const auto& name : entryNames) {
        const auto& routine = routines.at(name);
        if (!supportsLaunch(routine)) continue;
        const auto invoke = entry.label("invoke");
        const auto next = entry.label("next");
        const auto comparison = entry.temporary("call i32 @strcmp(ptr " + requested +
                                                 ", ptr " + stringConstant(name) + ")");
        const auto sameName = entry.temporary("icmp eq i32 " + comparison + ", 0");
        const auto count = entry.temporary("icmp eq i32 %argc, " +
                                           std::to_string(routine.parameters.size() + 2));
        entry.branch(entry.temporary("and i1 " + sameName + ", " + count), invoke, next);
        entry.block(invoke);
        std::string args;
        for (size_t i = 0; i < routine.parameters.size(); ++i) {
            if (i != 0) args += ", ";
            const Type& type = *routine.parameters[i];
            args += type.ir + " " + parseArgument(entry, type, argvAt(entry, i + 2));
        }
        entry.emit("call void @imp.initialize()");
        entry.emit("call " + routine.result->ir + " " + routine.ir + "(" + args + ")");
        entry.terminate("ret i32 0");
        entry.block(next);
    }
    entry.jump(invalid);
    entry.block(invalid);
    entry.emit("call i32 @puts(ptr " +
               stringConstant("Specify a routine name and its scalar arguments.") + ")");
    entry.terminate("ret i32 1");
    functions << entry.finish("i32 @main(i32 %argc, ptr %argv)");
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
    return "; Imp program\n%imp.array = type { i32, ptr }\n" + globals.str() + R"(
declare ptr @calloc(i64, i64)
declare i32 @printf(ptr, ...)
declare i32 @putchar(i32)
declare i32 @puts(ptr)
declare i32 @strcmp(ptr, ptr)
declare i64 @strtoll(ptr, ptr, i32)
declare double @strtod(ptr, ptr)
declare void @exit(i32) noreturn
declare void @llvm.trap()
declare double @llvm.round.f64(double)

define internal void @imp.invalid.argument() noreturn {
entry:
  call i32 @puts(ptr @imp.bad.argument)
  call void @exit(i32 1)
  unreachable
}

@imp.bad.argument = private constant [25 x i8] c"Invalid routine argument\00"

define internal i1 @imp.valid.number(ptr %start, ptr %end) {
entry:
  %advanced = icmp ne ptr %start, %end
  %last = load i8, ptr %end
  %complete = icmp eq i8 %last, 0
  %valid = and i1 %advanced, %complete
  ret i1 %valid
}

define internal i32 @imp.parse.integer(ptr %text) {
entry:
  %end = alloca ptr
  %number = call i64 @strtoll(ptr %text, ptr %end, i32 10)
  %endptr = load ptr, ptr %end
  %spelling = call i1 @imp.valid.number(ptr %text, ptr %endptr)
  %low = icmp sge i64 %number, -2147483648
  %high = icmp sle i64 %number, 2147483647
  %range = and i1 %low, %high
  %valid = and i1 %spelling, %range
  br i1 %valid, label %success, label %invalid
success:
  %result = trunc i64 %number to i32
  ret i32 %result
invalid:
  call void @imp.invalid.argument()
  unreachable
}

define internal double @imp.parse.real(ptr %text) {
entry:
  %end = alloca ptr
  %number = call double @strtod(ptr %text, ptr %end)
  %endptr = load ptr, ptr %end
  %valid = call i1 @imp.valid.number(ptr %text, ptr %endptr)
  br i1 %valid, label %success, label %invalid
success:
  ret double %number
invalid:
  call void @imp.invalid.argument()
  unreachable
}

define internal i1 @imp.parse.boolean(ptr %text) {
entry:
  %first = load i8, ptr %text
  switch i8 %first, label %invalid [ i8 48, label %digit
                                    i8 49, label %digit ]
digit:
  %next = getelementptr i8, ptr %text, i64 1
  %last = load i8, ptr %next
  %valid = icmp eq i8 %last, 0
  br i1 %valid, label %success, label %invalid
success:
  %value = icmp eq i8 %first, 49
  ret i1 %value
invalid:
  call void @imp.invalid.argument()
  unreachable
}

)" + functions.str();
}

} // namespace codegen

std::string generateLlvmIr(const Program& program) {
    return codegen::Module().generate(program);
}
