
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
printf("Lexer: Found String: %s\n", str.c_str());

Token token;
token.type = TOK_STRING;
token.string = str;
m_tokens.push_back(token);

}
        else
        {
            bool found = false;
            int i;
            for (i = 0; i < sizeof(tokenTable) / sizeof(SimpleToken); i++)
            {
                if (checkWord(&pos, &(tokenTable[i])))
                {
                    printf("Lexer: Found Token: %s\n", tokenTable[i].str);
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
                    printf("Lexer: found Identifier: %s\n", str.c_str());
Token token;
token.type = TOK_IDENTIFIER;
token.string = str;
m_tokens.push_back(token);
                }
                else
                {
                    printf("Unknown token:\n%s\n", pos);
                    break;
                }
            }
        }
    }

return true;
}

