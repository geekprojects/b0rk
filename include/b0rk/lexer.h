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

#ifndef __BSCRIPT_LEXER_H_
#define __BSCRIPT_LEXER_H_

#include <b0rk/token.h>

#include <string>
#include <vector>

namespace b0rk
{

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

};

#endif
