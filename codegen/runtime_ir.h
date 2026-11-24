#ifndef CODEGEN_RUNTIME_IR_H
#define CODEGEN_RUNTIME_IR_H

namespace codegen {

// LLVM declarations and argument parsers used by the generated entry point.
inline constexpr char runtimeIr[] = R"llvm(
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

)llvm";

} // namespace codegen

#endif
