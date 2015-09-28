#ifndef __BSCRIPT_LEXER_H_
#define __BSCRIPT_LEXER_H_

#include <b0rk/token.h>

#include <string>
#include <vector>

struct SimpleToken
{
    const char* str;
    int length;
    TokenType token;
};

class Lexer
{
 private:
    std::vector<Token> m_tokens;

    CharType chartype(int c);
    bool checkWord(char** pos, SimpleToken* token);

 public:
    Lexer();
    ~Lexer();

    bool lexer(char* buffer, int length);

    std::vector<Token>& getTokens() { return m_tokens; }
};

#endif
