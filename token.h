#ifndef __BSCRIPT_TOKEN_H_
#define __BSCRIPT_TOKEN_H_

#include <string>

enum CharType
{
    ALPHA,
    DIGIT,
    SYMBOL,
    WHITESPACE,
    OTHER
};

enum TokenType
{
    TOK_DOT,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_EQUALS,
    TOK_PLUS,
    TOK_MINUS,
    TOK_BRACE_LEFT,
    TOK_BRACE_RIGHT,
    TOK_BRACKET_LEFT,
    TOK_BRACKET_RIGHT,
    TOK_DOUBLE_QUOTE,
    TOK_QUOTE,
    TOK_CLASS,
    TOK_VAR,
    TOK_FUNCTION,
    TOK_STATIC,
    TOK_NEW,
    TOK_IDENTIFIER,
    TOK_STRING,
    TOK_EXPRESSION,
    TOK_ANY = -1,
};

struct Token
{
    TokenType type;
    std::string string;
};

#endif