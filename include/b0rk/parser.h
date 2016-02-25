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

#ifndef __BSCRIPT_PARSER_H_
#define __BSCRIPT_PARSER_H_

#include <b0rk/token.h>
#include <b0rk/expression.h>
#include <b0rk/class.h>
#include <b0rk/runtime.h>

#include <vector>

namespace b0rk
{

class Parser
{
 private:
    Context* m_context;

    std::vector<Token> m_tokens;
    std::vector<Expression*> m_expressions;
    std::map<std::wstring, Class*> m_imports;
    size_t m_pos;

    Token* nextToken();
    Token* peekToken();
    bool moreTokens() { return m_pos < m_tokens.size(); }

    Class* parseClass(bool addToExisting);
    Function* parseFunction(Class* clazz);

    CodeBlock* parseCodeBlock(ScriptFunction* function);

    Expression* parseExpression(CodeBlock* code);
    Expression* parseExpressionValue(CodeBlock* code);
    Expression* buildTree(CodeBlock* code, std::vector<Expression*>& queue);

    bool parseIdentifier(Identifier& id);
    bool parseList(std::vector<Token*>& list, TokenType type);
    bool parseExpressionList(CodeBlock* code, std::vector<Expression*>& list);

    bool resolveTypes();

    int getVarId(CodeBlock* block);
    void setVarIds(CodeBlock* block, int startingId);

 public:

    Parser(Context* context);
    ~Parser();

    bool parse(std::vector<Token> tokens, bool addToExisting = false);
};

};

#endif
