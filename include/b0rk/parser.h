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
    Context* m_context;

    std::vector<Token> m_tokens;
    std::vector<Expression*> m_expressions;
    std::map<std::string, Class*> m_imports;
    size_t m_pos;

    Token* nextToken();
    bool moreTokens() { return m_pos < m_tokens.size(); }

    Class* parseClass();
    Function* parseFunction(Class* clazz);

    CodeBlock* parseCodeBlock(ScriptFunction* function);
    Expression* parseExpression(CodeBlock* code);
    bool parseIdentifier(Identifier& id);
    bool parseList(std::vector<Token*>& list, TokenType type);
    bool parseExpressionList(CodeBlock* code, std::vector<Expression*>& list);

    bool resolveTypes();

    int getVarId(CodeBlock* block);
    void setVarIds(CodeBlock* block, int startingId);

 public:

    Parser(Context* context);
    ~Parser();

    bool parse(std::vector<Token> tokens);
};

#endif
