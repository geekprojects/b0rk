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
    TOK_COLON,
    TOK_EQUALS,
    TOK_ASTERISK,
    TOK_LOGICAL_AND,
    TOK_LESS_THAN,
    TOK_INCREMENT,
    TOK_ADD_ASSIGN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_BRACE_LEFT,
    TOK_BRACE_RIGHT,
    TOK_BRACKET_LEFT,
    TOK_BRACKET_RIGHT,
    TOK_DOUBLE_QUOTE,
    TOK_QUOTE,
    TOK_IMPORT,
    TOK_CLASS,
    TOK_EXTENDS,
    TOK_VAR,
    TOK_FUNCTION,
    TOK_STATIC,
    TOK_NEW,
    TOK_FOR,
    TOK_WHILE,
    TOK_RETURN,
    TOK_IDENTIFIER,
    TOK_STRING,
    TOK_INTEGER,
    TOK_DOUBLE,
    TOK_EXPRESSION,
    TOK_ANY = -1,
};

struct Token
{
    TokenType type;
    std::string string;
    int i;
    double d;
};

#endif
