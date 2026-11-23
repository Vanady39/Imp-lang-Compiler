#ifndef RUNNER_H
#define RUNNER_H

#include <string>

using namespace std;

enum class OutputKind { Bitcode, LlvmIr, Ast };

int runSourceFile(const string& path, const string& output, OutputKind kind);

#endif
