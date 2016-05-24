/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <b0rk/lexer.h>
#include <b0rk/utils.h>
#include <b0rk/utf8.h>

#undef DEBUG_LEXER
#undef DEBUG_LEXER_DETAILED

using namespace std;
using namespace b0rk;
using namespace Geek;

SimpleToken tokenTable[] = {
    { ".", 1, TOK_DOT },
    { ",", 1, TOK_COMMA },
    { ";", 1, TOK_SEMICOLON },
    { ":", 1, TOK_COLON },
    { "==", 2, TOK_EQUALS },
    { "=", 1, TOK_ASSIGN },
    { "<=", 2, TOK_LESS_THAN_EQUAL },
    { "<", 1, TOK_LESS_THAN },
    { ">=", 2, TOK_GREATER_THAN_EQUAL },
    { ">", 1, TOK_GREATER_THAN },
    { "+=", 2, TOK_ADD_ASSIGN },
    { "-=", 2, TOK_SUB_ASSIGN },
    { "++", 2, TOK_INCREMENT },
    { "--", 2, TOK_DECREMENT },
    { "+", 1, TOK_PLUS },
    { "-", 1, TOK_MINUS },
    { "*", 1, TOK_ASTERISK },
    { "/", 1, TOK_SLASH_FORWARD },
    { "&&", 2, TOK_LOGICAL_AND },
    { "{", 1, TOK_BRACE_LEFT },
    { "}", 1, TOK_BRACE_RIGHT },
    { "(", 1, TOK_BRACKET_LEFT },
    { ")", 1, TOK_BRACKET_RIGHT },
    { "[", 1, TOK_SQUARE_BRACKET_LEFT },
    { "]", 1, TOK_SQUARE_BRACKET_RIGHT },
    { "\"", 1, TOK_DOUBLE_QUOTE },
    { "'", 1, TOK_QUOTE },
    { "var", 3, TOK_VAR },
    { "import", 6, TOK_IMPORT },
    { "class", 5, TOK_CLASS },
    { "extends", 7, TOK_EXTENDS },
    { "function", 8, TOK_FUNCTION },
    { "static", 6, TOK_STATIC },
    { "new", 3, TOK_NEW },
    { "if", 2, TOK_IF },
    { "else", 4, TOK_ELSE },
    { "for", 3, TOK_FOR },
    { "while", 5, TOK_WHILE },
    { "return", 6, TOK_RETURN },
    { "true", 4, TOK_LITERAL },
    { "false", 5, TOK_LITERAL },
    { "try", 3, TOK_TRY },
    { "throw", 5, TOK_THROW },
    { "catch", 5, TOK_CATCH },
};

Lexer::Lexer() : Logger("Lexer")
{
}

Lexer::~Lexer()
{
}

CharType Lexer::chartype(int c)
{
    if (isalpha(c))
    {
        return ALPHA;
    }
    else if (isdigit(c))
    {
        return DIGIT;
    }
    else if (isspace(c))
    {
        return WHITESPACE;
    }
    else if (isspace(c))
    {
        return WHITESPACE;
    }
    else
    {
        return OTHER;
    }
}

static bool isIdentifierChar(wchar_t c)
{
    return iswalnum(c) || c == '_' || (c & 0x2600) == 0x2600;
}

bool Lexer::checkWord(char** pos, SimpleToken* token)
{
    CharType firstType = chartype(**pos);
    if (!strncmp(*pos, token->str, token->length))
    {
        if (firstType == ALPHA || firstType == DIGIT)
        {
            CharType type = chartype(*(*pos + token->length));
            if ((type == ALPHA || type == DIGIT))
            {
                return false;
            }
        }
        *pos += token->length;
        return true;
    }
    return false;
}

bool Lexer::lexer(char* buffer, int length)
{
    if (buffer == NULL || length == 0)
    {
        return false;
    }

    char* pos = buffer;
    char* end = buffer + length;

    int line = 1;

    while (pos < end)
    {
        wchar_t cur = utf8::next(pos, end);
        if (cur == 0)
        {
            break;
        }

        wchar_t next = 0;
        if (pos < end - 1)
        {
            next = utf8::peek_next(pos, end);
        }

#ifdef DEBUG_LEXER_DETAILED
        log(DEBUG, "cur=%d, next=%d", cur, next);
#endif

        if (cur == '/')
        {
#ifdef DEBUG_LEXER_DETAILED
            log(DEBUG, "Comment? n=%d (%c)", next, next);
#endif
            if (next == '/')
            {
                // // style comment
                utf8::next(pos, end);
                while (true)
                {
                    next = utf8::peek_next(pos, end);
                    if (next == '\n' || next == '\r' || next == '\0')
                    {
                        line++;
                        break;
                    }
                    utf8::next(pos, end);
                }
                continue;
            }
            else if (next == '*')
            {
                // /* style comment */
                utf8::next(pos, end);
                while (true)
                {
                    next = utf8::peek_next(pos, end);
                    if (next == '*')
                    {
                        utf8::next(pos, end);
                        next = utf8::peek_next(pos, end);
                        if (next == '/')
                        {
                            utf8::next(pos, end);
                            break;
                        }
                    }
                    else if (next == '\0')
                    {
                        break;
                    }
                    if (next == '\n')
                    {
                        line++;
                    }
                    utf8::next(pos, end);
                }
                continue;
            }
        }

        if (iswspace(cur))
        {
            // Skip whitespace
            if (cur == '\n')
            {
                line++;
            }
        }
        else if (iswdigit(cur) || (cur == '-' && iswdigit(next)))
        {
            wstring str;
            wstring exponent;
            bool dot = false;
            bool enotation = false;
            while (true)
            {
                if (cur == '-')
                {
                    if (str.length() > 0)
                    {
                        utf8::prior(pos, end);
                        break;
                    }
                    str += '-';
                }
                else if (cur == '.' && dot == false)
                {
                    str += '.';
                    dot = true;
                }
                else if (iswdigit(cur))
                {
                    str += cur;
                }
                else if (cur == 'e' || cur == 'E')
                {
                    cur = utf8::next(pos, end);
                    if (cur != '-' && cur != '+')
                    {
                        log(ERROR, "Line %d: Invalid number, expected - or +, got %lc", line, cur);
                        return false;
                    }

                    enotation = true;
                    exponent += cur;
                    while (true)
                    {
                        cur = utf8::next(pos, end);
                        if (!isdigit(cur))
                        {
                            utf8::prior(pos, end);
                            break;
                        }
                        exponent += cur;
                    }
                    break;
                }
                else
                {
                    utf8::prior(pos, end);
                    break;
                }
                cur = utf8::next(pos, end);
            }
#ifdef DEBUG_LEXER
            log(DEBUG, "found number: %ls", str.c_str());
#endif

            Token token;
            token.line = line;
            if (dot || enotation)
            {
                token.d = atof(Utils::wstring2string(str).c_str());
                if (enotation)
                {
#ifdef DEBUG_LEXER
                    log(DEBUG, "Line %d: -> Exponent: %ls", line, exponent.c_str());
#endif
                    int exponenti = atof(Utils::wstring2string(exponent).c_str());
                    token.d *= pow(10, exponenti);
                }
#ifdef DEBUG_LEXER
                log(DEBUG, "Line %d: -> Double: %0.6f", line, token.d);
#endif
                token.type = TOK_DOUBLE;
#ifdef DEBUG_LEXER
                log(DEBUG, "found number:  -> double: %0.2f", token.d);
#endif
            }
            else
            {
                token.i = atoi(Utils::wstring2string(str).c_str());
                token.type = TOK_INTEGER;
#ifdef DEBUG_LEXER
                log(DEBUG, "found number:  -> int: %d", token.i);
#endif
            }
            m_tokens.push_back(token);
        }
        else if (cur == '"')
        {
#ifdef DEBUG_LEXER
            log(DEBUG, "Reading string...");
#endif
            wstring str;
            while (pos < end)
            {
                cur = utf8::next(pos, end);
#ifdef DEBUG_LEXER_DETAILED
                log(DEBUG, "String: cur=0x%x %lc", cur, cur);
#endif
                if (cur == '\\')
                {
                    cur = utf8::next(pos, end);
                    if (cur == 'n')
                    {
                        str += '\n';
                    }
                    else
                    {
                        log(ERROR, "Unhandled backslash character: \\%lc", cur);
                        return false;
                    }
                }
                else if (cur == '\"')
                {
                    break;
                }
                else
                {
                    str += cur;
                }
            }
#ifdef DEBUG_LEXER
            log(DEBUG, "Found String: %ls", str.c_str());
#endif

            Token token;
            token.type = TOK_STRING;
            token.line = line;
            token.string = str;
            m_tokens.push_back(token);
        }
        else
        {
            bool found = false;
            unsigned int i;
            utf8::prior(pos, buffer);
            for (i = 0; i < sizeof(tokenTable) / sizeof(SimpleToken); i++)
            {
                if (checkWord(&pos, &(tokenTable[i])))
                {
#ifdef DEBUG_LEXER
                    log(DEBUG, "line %d: Found Token: %s", line, tokenTable[i].str);
#endif
                    Token token;
                    token.type = tokenTable[i].token;
                    token.line = line;
                    token.string = Utils::string2wstring(tokenTable[i].str);
                    m_tokens.push_back(token);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                if (isIdentifierChar(cur) && !iswdigit(cur))
                {
                    wstring str;
                    str += cur;
                    utf8::next(pos, end);

                    while (pos < end)
                    {
                        cur = utf8::peek_next(pos, end);
                        if (!isIdentifierChar(cur))
                        {
                            break;
                        }
                        utf8::next(pos, end);
                        str += cur;
                    }
#ifdef DEBUG_LEXER
                    log(DEBUG, "found Identifier: %ls", str.c_str());
#endif
                    Token token;
                    token.type = TOK_IDENTIFIER;
                    token.line = line;
                    token.string = str;
                    m_tokens.push_back(token);
                }
                else
                {
                    string restOfLine = "";
                    while (*pos != '\n' && *pos != '\0')
                    {
                        restOfLine += *pos;
                        pos++;
                    }
                    log(ERROR, "Unknown token: Line %d: %s", line, restOfLine.c_str());
                    return false;
                }
            }
        }
    }

    return true;
}

