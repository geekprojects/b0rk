
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"

using namespace std;

SimpleToken tokenTable[] = {
    { ".", 1, TOK_DOT },
    { ",", 1, TOK_COMMA },
    { ";", 1, TOK_SEMICOLON },
    { "=", 1, TOK_EQUALS },
    { "<", 1, TOK_LESS_THAN },
    { "+=", 2, TOK_ADD_ASSIGN },
    { "++", 2, TOK_INCREMENT },
    { "+", 1, TOK_PLUS },
    { "-", 1, TOK_MINUS },
    { "{", 1, TOK_BRACE_LEFT },
    { "}", 1, TOK_BRACE_RIGHT },
    { "(", 1, TOK_BRACKET_LEFT },
    { ")", 1, TOK_BRACKET_RIGHT },
    { "\"", 1, TOK_DOUBLE_QUOTE },
    { "'", 1, TOK_QUOTE },
    { "var", 3, TOK_VAR },
    { "class", 5, TOK_CLASS },
    { "function", 8, TOK_FUNCTION },
    { "static", 6, TOK_STATIC },
    { "new", 3, TOK_NEW },
    { "for", 3, TOK_FOR },
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
    while (*pos != '\0')
    {
        char c = *pos;
        if (isspace(c))
        {
            // Skip whitespace
            // TODO: Check whether we're in a string
            pos++;
        }
        else if (c == '"')
        {
            string str = "";
            pos++;
            while (*pos != '\0')
            {
                char strc = *(pos++);
                if (strc == '\"')
                {
                    break;
                }

                str += strc;
            }
#ifdef DEBUG_LEXER
            printf("Lexer: Found String: %s\n", str.c_str());
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
            for (i = 0; i < sizeof(tokenTable) / sizeof(SimpleToken); i++)
            {
                if (checkWord(&pos, &(tokenTable[i])))
                {
#ifdef DEBUG_LEXER
                    printf("Lexer: Found Token: %s\n", tokenTable[i].str);
#endif
                    Token token;
                    token.type = tokenTable[i].token;
                    token.string = tokenTable[i].str;
                    m_tokens.push_back(token);
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                if (isalpha(c))
                {
                    string str = "";
                    while (isalnum(*pos))
                    {
                        str += *(pos++);
                    }
#ifdef DEBUG_LEXER
                    printf("Lexer: found Identifier: %s\n", str.c_str());
#endif
                    Token token;
                    token.type = TOK_IDENTIFIER;
                    token.string = str;
                    m_tokens.push_back(token);
                }
                else if (isdigit(c))
                {
                    string str = "";
                    bool dot = false;
                    while (true)
                    {
                        char c = *pos;
                        if (c == '.' && dot == false)
                        {
                            str += '.';
                            dot = true;
                        }
                        else if (isdigit(c))
                        {
                            str += c;
                        }
                        else
                        {
                            break;
                        }
                        pos++;
                    }
#ifdef DEBUG_LEXER
                    printf("Lexer: found number: %s\n", str.c_str());
#endif

                    Token token;
                    if (dot)
                    {
                        token.d = atof(str.c_str());
                        token.type = TOK_DOUBLE;
#ifdef DEBUG_LEXER
                        printf("Lexer: found number:  -> double: %0.2f\n", token.d);
#endif
                    }
                    else
                    {
                        token.i = atoi(str.c_str());
                        token.type = TOK_INTEGER;
#ifdef DEBUG_LEXER
                        printf("Lexer: found number:  -> int: %d\n", token.i);
#endif
                    }
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

