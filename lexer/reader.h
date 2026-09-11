#ifndef READER_H
#define READER_H

#include <cstddef>
#include <string>

class Position {
public:
    int line, column;
    Position(int l, int c) : line(l), column(c) {}
};

class Reader {
    std::string source;
    size_t position;
    int line = 1, column = 1;
public:
    Reader(std::string s);
    char peek(int offset = 0);
    char get();
    bool eof() const;
    Position here() const;
};

#endif
