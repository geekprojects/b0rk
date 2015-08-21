#ifndef __BSCRIPT_PARSER_H_
#define __BSCRIPT_PARSER_H_

#include "token.h"

#include <vector>

struct Identifier
{
    std::vector<std::string> identifier;
    std::string toString();
};

enum ExpressionType
{
    EXPR_CALL,
    EXPR_NEW,
    EXPR_OPER,
    EXPR_VAR,
    EXPR_STRING
};

struct Expression
{
    ExpressionType type;

    virtual std::string toString() = 0;
};

struct CallExpression : public Expression
{
    CallExpression()
    {
        type = EXPR_CALL;
    }

    Identifier function;
    std::vector<Expression*> parameters;

    virtual std::string toString()
    {
return "CALL " + function.toString() + "(" + argsToString() + ")";
    }

std::string argsToString()
{
std::string str = "";
bool comma = false;
std::vector<Expression*>::iterator it;
for (it = parameters.begin(); it != parameters.end(); it++)
{
if (comma)
{
str += ", ";
}
comma = true;
str += (*it)->toString();
}
return str;
}
};

struct NewExpression : public CallExpression
{
    NewExpression()
    {
        type = EXPR_NEW;
    }

    Identifier clazz;

    virtual std::string toString()
    {
return "NEW " + clazz.toString() + "(" + argsToString() + ")";
    }
};

struct OperationExpression : public Expression
{
    OperationExpression()
    {
        type = EXPR_OPER;
    }

    TokenType operType;
    Expression* left;
    Expression* right;

virtual std::string toString()
{
return left->toString() + " OP " + right->toString();
}
};

struct VarExpression : public Expression
{
    VarExpression()
    {
        type = EXPR_VAR;
    }

    Identifier var;

virtual std::string toString()
{
return var.toString();
}
};

struct StringExpression : public Expression
{
    StringExpression()
    {
        type = EXPR_STRING;
    }

    std::string str;

virtual std::string toString()
{
return "\"" + str + "\"";
}
};


class Parser
{
 private:
    std::vector<Token> m_tokens;
    int m_pos;

    Token* nextToken();
    bool moreTokens() { return m_pos < m_tokens.size(); }

    bool parseClass();
    bool parseCodeBlock();

    Expression* parseExpression();
    bool parseIdentifier(Identifier& id);
    bool parseList(std::vector<Token*>& list, TokenType type);
    bool parseExpressionList(std::vector<Expression*>& list);

 public:

    Parser();
    ~Parser();

    bool parse(std::vector<Token> tokens);
};

#endif
