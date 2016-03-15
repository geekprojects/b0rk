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

#include <b0rk/lexer.h>
#include <b0rk/utils.h>
#include <b0rk/utf8.h>

#undef DEBUG_LEXER
#undef DEBUG_LEXER_DETAILED

using namespace std;
using namespace b0rk;

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

Lexer::Lexer()
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
    char* pos = buffer;
    char* end = buffer + length;

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
        printf("Lexer::lexer: cur=%d, next=%d\n", cur, next);
#endif

        if (cur == '/')
        {
#ifdef DEBUG_LEXER_DETAILED
            printf("Lexer::lexer: Comment? n=%d (%c)\n", next, next);
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
                    utf8::next(pos, end);
                }
                continue;
            }
        }

        if (iswspace(cur))
        {
            // Skip whitespace
        }
        else if (iswdigit(cur) || (cur == '-' && iswdigit(next)))
        {
            wstring str;
            bool dot = false;
            while (true)
            {
                if (cur == '-')
                {
                    if (str.length() > 0)
                    {
                        printf("Lexer: OH NOES, WE'VE GOT A MINUS HERE\n");
                        return false;
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
                else
                {
                    utf8::prior(pos, end);
                    break;
                }
                cur = utf8::next(pos, end);
            }
#ifdef DEBUG_LEXER
            printf("Lexer: found number: %ls\n", str.c_str());
#endif

            Token token;
            if (dot)
            {
                token.d = atof(Utils::wstring2string(str).c_str());
                token.type = TOK_DOUBLE;
#ifdef DEBUG_LEXER
                printf("Lexer: found number:  -> double: %0.2f\n", token.d);
#endif
            }
            else
            {
                token.i = atoi(Utils::wstring2string(str).c_str());
                token.type = TOK_INTEGER;
#ifdef DEBUG_LEXER
                printf("Lexer: found number:  -> int: %d\n", token.i);
#endif
            }
            m_tokens.push_back(token);
        }
        else if (cur == '"')
        {
#ifdef DEBUG_LEXER
            printf("Lexer: Reading string...\n");
#endif
            wstring str;
            while (pos < end)
            {
                cur = utf8::next(pos, end);
#ifdef DEBUG_LEXER_DETAILED
            wprintf(L"Lexer: String: cur=0x%x %lc\n", cur, cur);
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
                        printf("Lexer: Unhandled backslash character: \\%lc\n", cur);
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
            printf("Lexer: Found String: %ls\n", str.c_str());
#endif

            Token token;
            token.type = TOK_STRING;
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
                    printf("Lexer: Found Token: %s\n", tokenTable[i].str);
#endif
                    Token token;
                    token.type = tokenTable[i].token;
                    token.string = Utils::string2wstring(tokenTable[i].str);
                    m_tokens.push_back(token);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                utf8::next(pos, end);
                if (isIdentifierChar(cur) && !iswdigit(cur))
                {
                    wstring str;
                    str += cur;

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
                    printf("Lexer: found Identifier: %ls\n", str.c_str());
#endif
                    Token token;
                    token.type = TOK_IDENTIFIER;
                    token.string = str;
                    m_tokens.push_back(token);
                }
               else
                {
                    printf("Unknown token:\n%s\n", pos);
                    return false;
                }
            }
        }
    }

    return true;
}

