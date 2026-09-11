#include "reader.h"

using namespace std;

Reader::Reader(string s) : source(s), position(0) {}

char Reader::peek(int offset) {
    size_t i = position + offset;
    if (i >= source.size()) return '\0';
    return source[i];
}

char Reader::get() {
    if (position >= source.size()) return '\0';
    char c = source[position++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Reader::eof() const { return position >= source.size(); }

Position Reader::here() const { return { line, column }; }
