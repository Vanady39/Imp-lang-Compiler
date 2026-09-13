#ifndef LEXER_H
#define LEXER_H

#include "reader.h"
#include "vector"

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

string tokenName(TokenCode code);
bool convert_integer(string text, int& value, string& error);
bool convert_real(string text, double& value, string& error);

struct Span {
    int lineNum;
    int posBegin;
    int posEnd;
public:
    Span(int line, int begin, int end) : lineNum(line), posBegin(begin), posEnd(end) {}
};

struct Token {
    Span span;
    TokenCode code;
    int intValue;
    double realValue;
    string ID;
};

class Lexer {
    Reader reader;

    vector<string> errors = vector<string>();
public:
    Lexer(string source);

    vector<Token> GetBuffer(string path);
    const vector<string>& GetErrors() const;
    Token getNextToken();
    Token scanWord(int line, int column);
    Token scanNumber(int line, int column);
    Token scan_operator(int line, int column);
    void skipInsignificant();
    void skipSpaces();
    void skipShortComment();
    bool is_digit(char c);
    bool is_identifier_start(char c);
    bool is_identifier_part(char c);
};

#endif
