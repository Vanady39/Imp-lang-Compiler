#ifndef READER_H
#define READER_H

#include <memory>
#include <string>

using namespace std;

class Position {
public:
    int line, column;
    Position(int l, int c) : line(l), column(c) {}
};

class IReader {
public:
    virtual ~IReader() = default;

    virtual char peek(int offset = 0) const = 0;
    virtual char get() = 0;
    virtual bool eof() const = 0;
    virtual Position here() const = 0;
};

unique_ptr<IReader> makeReader(const string& source);

#endif
