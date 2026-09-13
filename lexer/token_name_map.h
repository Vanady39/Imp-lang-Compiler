#ifndef TOKEN_NAME_MAP_H
#define TOKEN_NAME_MAP_H

#include "unordered_map"
#include "lexer.h"

using namespace std;

extern const unordered_map<string, TokenCode> tokenMap;
extern const unordered_map<TokenCode, string> nameMap;

#endif