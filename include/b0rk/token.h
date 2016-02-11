/*
 * b0rk - The b0rk Embeddable Runtime Environment
 * Copyright (C) 2015, 2016 GeekProjects.com
 *
 * This file is part of b0rk.
 *
 * b0rk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * b0rk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BSCRIPT_TOKEN_H_
#define __BSCRIPT_TOKEN_H_

#include <string>

namespace b0rk
{

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
    TOK_ASSIGN,
    TOK_ASTERISK,
    TOK_SLASH_FORWARD,
    TOK_LOGICAL_AND,
    TOK_LESS_THAN,
    TOK_LESS_THAN_EQUAL,
    TOK_GREATER_THAN,
    TOK_GREATER_THAN_EQUAL,
    TOK_DECREMENT,
    TOK_INCREMENT,
    TOK_ADD_ASSIGN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_BRACE_LEFT,
    TOK_BRACE_RIGHT,
    TOK_BRACKET_LEFT,
    TOK_BRACKET_RIGHT,
    TOK_SQUARE_BRACKET_LEFT,
    TOK_SQUARE_BRACKET_RIGHT,
    TOK_DOUBLE_QUOTE,
    TOK_QUOTE,
    TOK_IMPORT,
    TOK_CLASS,
    TOK_EXTENDS,
    TOK_VAR,
    TOK_FUNCTION,
    TOK_STATIC,
    TOK_NEW,
    TOK_IF,
    TOK_ELSE,
    TOK_FOR,
    TOK_WHILE,
    TOK_RETURN,
    TOK_IDENTIFIER,
    TOK_LITERAL,
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

};

#endif
