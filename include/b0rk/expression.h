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
    std::vector<std::wstring> identifier;
    std::wstring end();
    std::wstring toString();

    static Identifier makeIdentifier(std::wstring name);
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

    bool resultOnStack;

    Expression(CodeBlock* block);
    virtual ~Expression();

    virtual std::wstring toString() = 0;
};

struct CallExpression : public Expression
{
    Class* clazz;
    std::wstring function;
    std::vector<Expression*> parameters;

    CallExpression(CodeBlock* block);
    virtual ~CallExpression();

    virtual std::wstring toString();

    std::wstring argsToString();
};

struct NewExpression : public CallExpression
{
    Identifier clazz;

    NewExpression(CodeBlock* block);
    virtual ~NewExpression() {}

    virtual std::wstring toString();
};

enum OpType
{
    OP_NONE,
    OP_SET,
    OP_LOGICAL_AND,
    OP_EQUALS,
    OP_LESS_THAN,
    OP_LESS_THAN_EQUAL,
    OP_GREATER_THAN,
    OP_GREATER_THAN_EQUAL,
    OP_ADD,
    OP_SUB,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_INCREMENT,
    OP_DECREMENT,
    OP_REFERENCE,
    OP_ARRAY
};

struct OpDesc
{
    TokenType token;
    OpType oper;
    bool hasLeftExpr;
    bool hasRightExpr;
};

struct OperationExpression : public Expression
{

    OpType operType;
    OpDesc operDesc;
    Expression* left;
    Expression* right;

    OperationExpression(CodeBlock* block, OpDesc desc);
    virtual ~OperationExpression();

    void resolveType();

    virtual std::wstring toString();
};

struct IfExpression : public Expression
{
    Expression* testExpr;
    CodeBlock* trueBlock;
    CodeBlock* falseBlock;

    IfExpression(CodeBlock* block);
    virtual ~IfExpression();

    virtual std::wstring toString();
};

struct ForExpression : public Expression
{
    Expression* initExpr;
    Expression* testExpr;
    Expression* incExpr;
    CodeBlock* body;

    ForExpression(CodeBlock* block);
    virtual ~ForExpression();

    virtual std::wstring toString();
};

struct ReturnExpression : public Expression
{
    Expression* returnValue;

    ReturnExpression(CodeBlock* block);

    virtual std::wstring toString();
};

struct VarExpression : public Expression
{
    Class* clazz;
    std::wstring var;

    VarExpression(CodeBlock* block);
    virtual ~VarExpression() {}

    virtual std::wstring toString();
};

struct ArrayExpression : public VarExpression
{
    Expression* indexExpr;

    ArrayExpression(CodeBlock* block);
    virtual ~ArrayExpression() {}

    virtual std::wstring toString();
};


struct StringExpression : public Expression
{
    std::wstring str;

    StringExpression(CodeBlock* block);

    virtual std::wstring toString();
};

struct IntegerExpression : public Expression
{
    int i;

    IntegerExpression(CodeBlock* block);

    virtual std::wstring toString();
};

struct DoubleExpression : public Expression
{
    double d;

    DoubleExpression(CodeBlock* block);

    virtual std::wstring toString();
};

struct FunctionExpression : public Expression
{
    Function* function;

    FunctionExpression(CodeBlock* block);

    virtual std::wstring toString();
};

};


#include "codeblock.h"

#endif
