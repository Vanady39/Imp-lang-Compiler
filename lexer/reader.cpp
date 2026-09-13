#include "reader.h"

using namespace std;

static bool startsWithUtf8Bom(string s) {
    return s.size() >= 3 &&
           (unsigned char)s[0] == 0xEF &&
           (unsigned char)s[1] == 0xBB &&
           (unsigned char)s[2] == 0xBF;
}

static string normalize(string s) {
    if (startsWithUtf8Bom(s)) {
        s.erase(0, 3);
    }

    string result = "";

    for (size_t i = 0; i < s.size(); i++) {
        // приводит конец строки к виду: \n (а сщуествует: \r\n и \r) 
        if (s[i] == '\r') {
            if (i + 1 < s.size() && s[i + 1] == '\n') {
                i++;
            }
            result += '\n';
        } else {
            result += s[i];
        }
    }
    return result;
}

Reader::Reader(string s) : source(normalize(s)), position(0) {}

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
