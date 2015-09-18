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
    std::vector<Expression*> m_expressions;
    std::map<std::string, Class*> m_imports;
    size_t m_pos;

    Token* nextToken();
    bool moreTokens() { return m_pos < m_tokens.size(); }

    Class* parseClass(Runtime* runtime);
    Function* parseFunction(Runtime* runtime, Class* clazz);

    CodeBlock* parseCodeBlock(Runtime* runtime, ScriptFunction* function);
    Expression* parseExpression(Runtime* runtime, CodeBlock* code);
    bool parseIdentifier(Identifier& id);
    bool parseList(std::vector<Token*>& list, TokenType type);
    bool parseExpressionList(Runtime* runtime, CodeBlock* code, std::vector<Expression*>& list);

    bool resolveTypes();

    int getVarId(CodeBlock* block);
    void setVarIds(CodeBlock* block, int startingId);

 public:

    Parser();
    ~Parser();

    bool parse(Context* context, std::vector<Token> tokens);
};

#endif
