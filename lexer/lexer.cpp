#include <iostream>
#include "reader.h"
#include <vector>
#include <unordered_map>
#include <cctype>
#include <string>
#include <format>

using namespace std;
enum TokenCode {
    tokenVar,
    tokenType,
    tokenRoutine,
    tokenIs,
    tokenInteger,
    tokenReal,
    tokenBoolean,
    tokenRecord,
    tokenArray,
    tokenWhile,
    tokenLoop,
    tokenFor,
    tokenIn,
    tokenReverse,
    tokenIf,
    tokenThen,
    tokenElse,
    tokenPrint,
    tokenReturn,
    tokenEnd,
    tokenAnd,
    tokenOr,
    tokenXor,
    tokenNot,
    tokenTrue,
    tokenFalse,
    tokenAssign,
    tokenDotDot,
    tokenArrow,
    tokenLessEqual,
    tokenGreaterEqual,
    tokenNotEqual,
    tokenLess,
    tokenGreater,
    tokenEqual,
    tokenPlus,
    tokenMinus,
    tokenStar,
    tokenSlash,
    tokenPercent,
    tokenDot,
    tokenComma,
    tokenSemicolon,
    tokenColon,
    tokenLeftParen,
    tokenRightParen,
    tokenLeftBracket,
    tokenRightBracket,
    tokenIdentifier,
    tokenIntegerLiteral,
    tokenRealLiteral,
    tokenNewline,
    tokenEOF,
    tokenUnknown
};

const unordered_map<string, TokenCode> tokenMap = {
    {"var", tokenVar},
    {"type", tokenType},
    {"routine", tokenRoutine},
    {"is", tokenIs},
    {"integer", tokenInteger},
    {"real", tokenReal},
    {"boolean", tokenBoolean},
    {"record", tokenRecord},
    {"array", tokenArray},
    {"while", tokenWhile},
    {"loop", tokenLoop},
    {"for", tokenFor},
    {"in", tokenIn},
    {"reverse", tokenReverse},
    {"if", tokenIf},
    {"then", tokenThen},
    {"else", tokenElse},
    {"print", tokenPrint},
    {"return", tokenReturn},
    {"end", tokenEnd},
    {"and", tokenAnd},
    {"or", tokenOr},
    {"xor", tokenXor},
    {"not", tokenNot},
    {"true", tokenTrue},
    {"false", tokenFalse}
};
struct Span {
    long lineNum;
    int posBegin, posEnd;
public:
    Span(long line, int begin, int end) : lineNum(line), posBegin(begin), posEnd(end) {}
};

struct Token {
    Span span;
    unsigned int TokenCode;
    int intValue;
    double realValue;
    string ID;

};
class Lexer {
    Reader reader;
    vector<string> errors = vector<string>();
public:
    Lexer(string source) : reader(source) {};
    vector<Token> GetBuffer(string path) {
        vector<Token> tokens;
        while (true) {
            Token token = getNextToken();
            tokens.push_back(token);
            if (token.TokenCode == tokenEOF) return tokens;
        }
    }
    Token getNextToken() {
        skipSpaces();
        auto pos = reader.here();
        char c = reader.peek();
        if (c == '\0') {
            return { { pos.line, pos.column, pos.column }, tokenEOF, 0, 0, "" };
        }
        if (c == '\n') {
            reader.get();
            return { { pos.line, pos.column, pos.column }, tokenNewline, 0, 0, "" };
        }
        if (is_identifier_start(c)) {
            return scanWord(pos.line, pos.column);
        }
        if (isdigit(c)) {
            return scanNumber(pos.line, pos.column);
        }
        return scan_operator(pos.line, pos.column);
    }
    Token scanWord(int line, int column) {
        string buffer = "";
        while (is_identifier_part(reader.peek())) {
            buffer += reader.get();
        }
        auto val = tokenMap.find(buffer);
        unsigned int code;
        if (val == tokenMap.end()) {
            code = tokenIdentifier;
        } else {
            code = val->second;
        }
        Span span = { line, column, column + buffer.size() - 1 };
        if (code == tokenIdentifier) {
            return { span, code, 0, 0, buffer };
        } else {
            return { span, code, 0, 0, "" };
        }
    }
    Token scanNumber(int line, int column) {
        string buffer = "";
        while (isdigit(reader.peek()) ){
            buffer += reader.get();
        }
        bool is_real = false;
        if (reader.peek() == '.')  {
            if (reader.peek(1) != '.' && isdigit(reader.peek(1))){
                is_real = true;
                buffer += reader.get();
                while (isdigit(reader.peek())) {
                    buffer += reader.get();
                }
            }
        }
        Span span = { line, column, column + buffer.size() - 1 };
        if (is_real) {
            return { span, tokenRealLiteral, 0, stod(buffer), "" };
        } 
        return { span, tokenIntegerLiteral, stoi(buffer), 0, "" };
        
    }
    Token scan_operator(int line, int column) {
        char c = reader.get();
        switch (c)
        {
        case ':':
            if (reader.peek() == '=')
            {
                reader.get();
                Span span = {line, column, column+1};
                return {span, tokenAssign, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenColon, 0, 0, ""};
            }
            break;
        case '.':
            if (reader.peek() == '.')
            {
                reader.get();
                Span span = {line, column, column+1};
                return {span, tokenDotDot, 0, 0, ""};
            }
            else {
                Span span = {line, column, column};
                return {span, tokenDot, 0, 0, ""};
            }
            break;
        case '=':
            if (reader.peek() == '>'){
                reader.get();
                Span span = {line, column, column+1};
                return {span, tokenArrow, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenEqual, 0, 0, ""};
            }
            break;
        case '<':
            if (reader.peek() == '=')
            {
                reader.get();
                Span span = {line, column, column+1};
                return {span, tokenLessEqual, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenLess, 0, 0, ""};
            }
            break;
        case '>':
            if (reader.peek() == '=')
            {
                reader.get();
                Span span = {line, column, column+1};
                return {span, tokenGreaterEqual, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenGreater, 0, 0, ""};
            }
            break;
        case '/':
            if (reader.peek() == '=')
            {
                reader.get();
                Span span = {line, column, column+1};
                return {span, tokenNotEqual, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenSlash, 0, 0, ""};
            }
            break;
        case '+':
            Span span = {line, column, column};
            return {span, tokenPlus, 0, 0, ""};
            break;
        case '-':
            span = {line, column, column};
            return {span, tokenMinus, 0, 0, ""};
            break;
        case '*':
            span = {line, column, column};
            return {span, tokenStar, 0, 0, ""};
            break;
        case '%':
            span = {line, column, column};
            return {span, tokenPercent, 0, 0, ""};
            break;
        case ',':
            span = {line, column, column};
            return {span, tokenComma, 0, 0, ""};
            break;
        case ';':
            span = {line, column, column};
            return {span, tokenSemicolon, 0, 0, ""};
            break;
        case '(':
            span = {line, column, column};
            return {span, tokenLeftParen, 0, 0, ""};
            break;
        case ')':
            span = {line, column, column};
            return {span, tokenRightParen, 0, 0, ""};
            break;
        case '[':
            span = {line, column, column};
            return {span, tokenLeftBracket, 0, 0, ""};
            break;
        case ']':
            span = {line, column, column};
            return {span, tokenRightBracket, 0, 0, ""};
            break;
        default:
            errors.push_back(format("Invalid symbol {} on line {} column {}", c, line, column));
            span = {line, column, column};
            return {span, tokenUnknown, 0, 0, ""};
        }
    }
        
    void skipSpaces() {
        while (reader.peek() == ' ' || reader.peek() == '\t') {
            reader.get();
        }
    }
    bool is_identifier_start(char c) {
        return isalpha(c) || c == '_';
    }
    bool is_identifier_part(char c) {
        return isalnum(c) || c == '_'; 
    }

    

};