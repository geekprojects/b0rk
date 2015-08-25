#ifndef __BSCRIPT_EXPRESSION_H_
#define __BSCRIPT_EXPRESSION_H_

#include <vector>
#include <string>

#include "token.h"

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
    EXPR_STRING,
    EXPR_INTEGER,
    EXPR_DOUBLE,
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

enum OpType
{
    OP_NONE,
    OP_EQUALS,
    OP_PLUS,
    OP_MINUS
};

struct OperationExpression : public Expression
{
    OperationExpression()
    {
        type = EXPR_OPER;
    }

    OpType operType;
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

struct IntegerExpression : public Expression
{
    IntegerExpression()
    {
        type = EXPR_INTEGER;
    }
    int i;
    virtual std::string toString()
    {
        return "INTEGER";
    }
};

struct DoubleExpression : public Expression
{
    DoubleExpression()
    {
        type = EXPR_DOUBLE;
    }
    double d;
    virtual std::string toString()
    {
        return "DOUBLE";
    }
};


struct CodeBlock
{
    std::vector<std::string> m_vars;
    std::vector<Expression*> m_code;
};

#endif
