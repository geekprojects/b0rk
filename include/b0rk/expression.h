#ifndef __BSCRIPT_EXPRESSION_H_
#define __BSCRIPT_EXPRESSION_H_

#include <vector>
#include <string>

#include <b0rk/token.h>
#include <b0rk/value.h>

namespace b0rk
{

struct CodeBlock;
class Function;
class ScriptFunction;
class Class;

struct Identifier
{
    std::vector<std::string> identifier;
    std::string end();
    std::string toString();
};

enum ExpressionType
{
    EXPR_CALL,
    EXPR_NEW,
    EXPR_OPER,
    EXPR_IF,
    EXPR_FOR,
    EXPR_RETURN,
    EXPR_VAR,
    EXPR_ARRAY,
    EXPR_STRING,
    EXPR_INTEGER,
    EXPR_DOUBLE,
    EXPR_FUNCTION,
};

struct Expression
{
    Expression* parent;
    CodeBlock* block;
    ExpressionType type;
    ValueType valueType;

    Expression(CodeBlock* block);
    virtual ~Expression();

    virtual std::string toString() = 0;
};

struct CallExpression : public Expression
{
    Class* clazz;
    std::string function;
    std::vector<Expression*> parameters;

    CallExpression(CodeBlock* block);
    virtual ~CallExpression();

    virtual std::string toString();

    std::string argsToString();
};

struct NewExpression : public CallExpression
{
    Identifier clazz;

    NewExpression(CodeBlock* block);
    virtual ~NewExpression() {}

    virtual std::string toString();
};

enum OpType
{
    OP_NONE,
    OP_SET,
    OP_ADD,
    OP_SUB,
    OP_MULTIPLY,
    OP_LOGICAL_AND,
    OP_INCREMENT,
    OP_LESS_THAN,
    OP_REFERENCE,
    OP_ARRAY
};

struct OperationExpression : public Expression
{

    OpType operType;
    Expression* left;
    Expression* right;

    OperationExpression(CodeBlock* block);
    virtual ~OperationExpression();

    void resolveType();

    virtual std::string toString();
};

struct IfExpression : public Expression
{
    Expression* testExpr;
    CodeBlock* trueBlock;
    CodeBlock* falseBlock;

    IfExpression(CodeBlock* block);
    virtual ~IfExpression();

    virtual std::string toString();
};

struct ForExpression : public Expression
{
    Expression* initExpr;
    Expression* testExpr;
    Expression* incExpr;
    CodeBlock* body;

    ForExpression(CodeBlock* block);
    virtual ~ForExpression();

    virtual std::string toString();
};

struct ReturnExpression : public Expression
{
    Expression* returnValue;

    ReturnExpression(CodeBlock* block);

    virtual std::string toString();
};

struct VarExpression : public Expression
{
    Class* clazz;
    std::string var;

    VarExpression(CodeBlock* block);
    virtual ~VarExpression() {}

    virtual std::string toString();
};

struct ArrayExpression : public VarExpression
{
    Expression* indexExpr;

    ArrayExpression(CodeBlock* block);
    virtual ~ArrayExpression() {}

    virtual std::string toString();
};


struct StringExpression : public Expression
{
    std::string str;

    StringExpression(CodeBlock* block);

    virtual std::string toString();
};

struct IntegerExpression : public Expression
{
    int i;

    IntegerExpression(CodeBlock* block);

    virtual std::string toString();
};

struct DoubleExpression : public Expression
{
    double d;

    DoubleExpression(CodeBlock* block);

    virtual std::string toString();
};

struct FunctionExpression : public Expression
{
    Function* function;

    FunctionExpression(CodeBlock* block);

    virtual std::string toString();
};

};


#include "codeblock.h"

#endif
