#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "reader.h"

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

std::string tokenName(TokenCode code);
bool convert_integer(std::string text, int& value, std::string& error);
bool convert_real(std::string text, double& value, std::string& error);

struct Span {
    int lineNum;
    int posBegin, posEnd;
public:
    Span(int line, int begin, int end) : lineNum(line), posBegin(begin), posEnd(end) {}
};

struct Token {
    Span span;
    TokenCode code;
    int intValue;
    double realValue;
    std::string ID;

};

class Lexer {
    Reader reader;
    std::vector<std::string> errors = std::vector<std::string>();
public:
    Lexer(std::string source);
    std::vector<Token> GetBuffer(std::string path);
    const std::vector<std::string>& GetErrors() const;
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
