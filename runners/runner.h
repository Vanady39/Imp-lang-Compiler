#ifndef RUNNER_H
#define RUNNER_H

#include <string>

using namespace std;

// Единственная точка входа раннера: читает файл, прогоняет лексер и парсер,
// печатает ошибки или дерево программы. Возвращает код завершения.
int runSourceFile(const string& path);

#endif
