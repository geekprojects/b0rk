#ifndef __BSCRIPT_PARSER_H_
#define __BSCRIPT_PARSER_H_

#include "token.h"
#include "expression.h"
#include "class.h"
#include "runtime.h"

#include <vector>

class Parser
{
 private:
    std::vector<Token> m_tokens;
    size_t m_pos;

    Token* nextToken();
    bool moreTokens() { return m_pos < m_tokens.size(); }

    Class* parseClass();
    CodeBlock* parseCodeBlock();

    Expression* parseExpression();
    bool parseIdentifier(Identifier& id);
    bool parseList(std::vector<Token*>& list, TokenType type);
    bool parseExpressionList(std::vector<Expression*>& list);

 public:

    Parser();
    ~Parser();

    bool parse(Runtime* runtime, std::vector<Token> tokens);
};

#endif
