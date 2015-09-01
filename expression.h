#ifndef __BSCRIPT_EXPRESSION_H_
#define __BSCRIPT_EXPRESSION_H_

#include <vector>
#include <string>

#include "token.h"
#include "value.h"

struct CodeBlock;

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
    EXPR_FOR,
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
    Identifier function;
    std::vector<Expression*> parameters;

    CallExpression();

    virtual std::string toString();

    std::string argsToString();
};

struct NewExpression : public CallExpression
{
    Identifier clazz;

    NewExpression();

    virtual std::string toString();
};

enum OpType
{
    OP_NONE,
    OP_SET,
    OP_ADD,
    OP_SUB,
    OP_INCREMENT,
    OP_LESS_THAN
};

struct OperationExpression : public Expression
{

    OpType operType;
    Expression* left;
    Expression* right;

    OperationExpression();

    virtual std::string toString();
};

struct ForExpression : public Expression
{
    Expression* initExpr;
    Expression* testExpr;
    Expression* incExpr;
    CodeBlock* body;

    ForExpression();

    virtual std::string toString();
};

struct VarExpression : public Expression
{
    Identifier var;

    VarExpression();

    virtual std::string toString();
};

struct StringExpression : public Expression
{
    std::string str;

    StringExpression();

    virtual std::string toString();
};

struct IntegerExpression : public Expression
{
    int i;

    IntegerExpression();

    virtual std::string toString();
};

struct DoubleExpression : public Expression
{
    double d;

    DoubleExpression();

    virtual std::string toString();
};

struct CodeBlock
{
    CodeBlock* m_parent;
    int m_startingVarId;
    int m_maxVarId;

    ValueType* m_varTypes;

    std::vector<std::string> m_vars;
    std::vector<Expression*> m_code;
    std::vector<CodeBlock*> m_childBlocks;

    CodeBlock();

    int setStartingVarId(int id);
    int getVarId(std::string var);

    std::string toString();
};

#endif
