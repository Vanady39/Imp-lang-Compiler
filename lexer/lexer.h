#ifndef LEXER_H
#define LEXER_H

#include <memory>
#include <string>
#include <vector>

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

class ILexer {
public:
    virtual ~ILexer() = default;

    virtual vector<Token> GetBuffer(string path) = 0;
    virtual const vector<string>& GetErrors() const = 0;
    virtual Token getNextToken() = 0;
    virtual Token scanWord(int line, int column) = 0;
    virtual Token scanNumber(int line, int column) = 0;
    virtual Token scan_operator(int line, int column) = 0;
    virtual void skipInsignificant() = 0;
    virtual void skipSpaces() = 0;
    virtual void skipShortComment() = 0;
    virtual bool is_digit(char c) = 0;
    virtual bool is_identifier_start(char c) = 0;
    virtual bool is_identifier_part(char c) = 0;
};

unique_ptr<ILexer> makeLexer(const string& source);

#endif
