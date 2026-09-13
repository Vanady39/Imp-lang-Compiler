#include "lexer.h"
#include "reader.h"
#include "token_name_map.h"

#include <format>
#include <stdexcept>

using namespace std;

string tokenName(TokenCode code) {
    auto val = nameMap.find(code);
    if (val == nameMap.end()) {
        return "<unnamed token>";
    }
    return val->second;
}

bool convert_integer(string text, int& value, string& error) {
    try {
        value = stoi(text);
        return true;
    } catch (const out_of_range&) {
        error = format("Integer literal {} does not fit in the integer type", text);
    } catch (const invalid_argument&) {
        error = format("Malformed integer literal {}", text);
    }
    value = 0;
    return false;
}

bool convert_real(string text, double& value, string& error) {
    try {
        value = stod(text);
        return true;
    } catch (const out_of_range&) {
        error = format("Real literal {} does not fit in the real type", text);
    } catch (const invalid_argument&) {
        error = format("Malformed real literal {}", text);
    }
    value = 0;
    return false;
}

class Lexer : public ILexer {
    unique_ptr<IReader> reader;
    vector<string> errors = vector<string>();

public:
    Lexer(string source) : reader(makeReader(source)) {}

    vector<Token> GetBuffer(string path) override {
        vector<Token> tokens;
        while (true) {
            Token token = getNextToken();
            tokens.push_back(token);
            if (token.code == tokenEOF) return tokens;
        }
    }

    const vector<string>& GetErrors() const override {
        return errors;
    }

    Token getNextToken() override {
        skipInsignificant();
        auto pos = reader->here();
        char c = reader->peek();
        if (c == '\0') {
            return { { pos.line, pos.column, pos.column }, tokenEOF, 0, 0, "" };
        }
        if (c == '\n') {
            reader->get();
            return { { pos.line, pos.column, pos.column }, tokenNewline, 0, 0, "" };
        }
        if (is_identifier_start(c)) {
            return scanWord(pos.line, pos.column);
        }
        if (is_digit(c)) {
            return scanNumber(pos.line, pos.column);
        }
        return scan_operator(pos.line, pos.column);
    }

    Token scanWord(int line, int column) override {
        string buffer = "";
        while (is_identifier_part(reader->peek())) {
            buffer += reader->get();
        }
        auto val = tokenMap.find(buffer);
        TokenCode code;
        if (val == tokenMap.end()) {
            code = tokenIdentifier;
        } else {
            code = val->second;
        }
        Span span = { line, column, column + (int)buffer.size() - 1 };
        if (code == tokenIdentifier) {
            return { span, code, 0, 0, buffer };
        } else {
            return { span, code, 0, 0, "" };
        }
    }

    Token scanNumber(int line, int column) override {
        string buffer = "";
        while (is_digit(reader->peek()) ){
            buffer += reader->get();
        }
        bool is_real = false;
        if (reader->peek() == '.')  {
            if (reader->peek(1) != '.' && is_digit(reader->peek(1))){
                is_real = true;
                buffer += reader->get();
                while (is_digit(reader->peek())) {
                    buffer += reader->get();
                }
            }
        }
        Span span = { line, column, column + (int)buffer.size() - 1 };
        string error = "";
        if (is_real) {
            double realValue = 0;
            if (!convert_real(buffer, realValue, error)) {
                errors.push_back(format("{} on line {} column {}", error, line, column));
                return { span, tokenUnknown, 0, 0, buffer };
            }
            return { span, tokenRealLiteral, 0, realValue, buffer };
        }
        int intValue = 0;
        if (!convert_integer(buffer, intValue, error)) {
            errors.push_back(format("{} on line {} column {}", error, line, column));
            return { span, tokenUnknown, 0, 0, buffer };
        }
        return { span, tokenIntegerLiteral, intValue, 0, buffer };

    }

    Token scan_operator(int line, int column) override {
        char c = reader->get();
        switch (c)
        {
        case ':':
            if (reader->peek() == '=')
            {
                reader->get();
                Span span = {line, column, column+1};
                return {span, tokenAssign, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenColon, 0, 0, ""};
            }
            break;
        case '.':
            if (reader->peek() == '.')
            {
                reader->get();
                Span span = {line, column, column+1};
                return {span, tokenDotDot, 0, 0, ""};
            }
            else {
                Span span = {line, column, column};
                return {span, tokenDot, 0, 0, ""};
            }
            break;
        case '=':
            if (reader->peek() == '>'){
                reader->get();
                Span span = {line, column, column+1};
                return {span, tokenArrow, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenEqual, 0, 0, ""};
            }
            break;
        case '<':
            if (reader->peek() == '=')
            {
                reader->get();
                Span span = {line, column, column+1};
                return {span, tokenLessEqual, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenLess, 0, 0, ""};
            }
            break;
        case '>':
            if (reader->peek() == '=')
            {
                reader->get();
                Span span = {line, column, column+1};
                return {span, tokenGreaterEqual, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenGreater, 0, 0, ""};
            }
            break;
        case '/':
            if (reader->peek() == '=')
            {
                reader->get();
                Span span = {line, column, column+1};
                return {span, tokenNotEqual, 0, 0, ""};
            } else {
                Span span = {line, column, column};
                return {span, tokenSlash, 0, 0, ""};
            }
            break;
        case '+': {
            Span span = {line, column, column};
            return {span, tokenPlus, 0, 0, ""};
            break;
        }
        case '-': {
            Span span = {line, column, column};
            return {span, tokenMinus, 0, 0, ""};
            break;
        }
        case '*': {
            Span span = {line, column, column};
            return {span, tokenStar, 0, 0, ""};
            break;
        }
        case '%': {
            Span span = {line, column, column};
            return {span, tokenPercent, 0, 0, ""};
            break;
        }
        case ',': {
            Span span = {line, column, column};
            return {span, tokenComma, 0, 0, ""};
            break;
        }
        case ';': {
            Span span = {line, column, column};
            return {span, tokenSemicolon, 0, 0, ""};
            break;
        }
        case '(': {
            Span span = {line, column, column};
            return {span, tokenLeftParen, 0, 0, ""};
            break;
        }
        case ')': {
            Span span = {line, column, column};
            return {span, tokenRightParen, 0, 0, ""};
            break;
        }
        case '[': {
            Span span = {line, column, column};
            return {span, tokenLeftBracket, 0, 0, ""};
            break;
        }
        case ']': {
            Span span = {line, column, column};
            return {span, tokenRightBracket, 0, 0, ""};
            break;
        }
        default: {
            errors.push_back(format("Invalid symbol {} on line {} column {}", c, line, column));
            Span span = {line, column, column};
            return {span, tokenUnknown, 0, 0, ""};
        }
        }
    }

    void skipInsignificant() override {
        while (true) {
            skipSpaces();
            if (reader->peek() == '/' && reader->peek(1) == '/') {
                skipShortComment();
                continue;
            }
            return;
        }
    }

    void skipSpaces() override {
        while (reader->peek() == ' ' || reader->peek() == '\t') {
            reader->get();
        }
    }

    void skipShortComment() override {
        reader->get();
        reader->get();
        while (reader->peek() != '\n' && reader->peek() != '\0') {
            reader->get();
        }
    }

    bool is_digit(char c) override {
        return c >= '0' && c <= '9';
    }

    bool is_identifier_start(char c) override {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    bool is_identifier_part(char c) override {
        return is_identifier_start(c) || is_digit(c);
    }
};

unique_ptr<ILexer> makeLexer(const string& source) {
    return make_unique<Lexer>(source);
}
