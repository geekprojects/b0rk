/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <b0rk/expression.h>
#include <b0rk/function.h>
#include <b0rk/utils.h>

#include <stdio.h>
#include <wchar.h>

using namespace std;
using namespace b0rk;

Expression::Expression(CodeBlock* _block)
{
    parent = NULL;
    block = _block;
    valueType = VALUE_UNKNOWN;
    resultOnStack = false;
}

Expression::~Expression()
{
}

CallExpression::CallExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_CALL;
    resultOnStack = true;
}

CallExpression::~CallExpression()
{
    vector<Expression*>::iterator it;
    for (it = parameters.begin(); it != parameters.end(); it++)
    {
        delete *it;
    }
}

wstring CallExpression::toString()
{
    return L"CALL " + function + L"(" + argsToString() + L")";
}

wstring CallExpression::argsToString()
{
    wstring str;
    bool comma = false;
    std::vector<Expression*>::iterator it;
    for (it = parameters.begin(); it != parameters.end(); it++)
    {
        if (comma)
        {
            str += L", ";
        }
        comma = true;
        str += (*it)->toString();
    }
    return str;
}

NewExpression::NewExpression(CodeBlock* block)
    : CallExpression(block)
{
    type = EXPR_NEW;
}

std::wstring NewExpression::toString()
{
    return L"NEW " + clazz.toString() + L"(" + argsToString() + L")";
}

OperationExpression::OperationExpression(CodeBlock* block, OpDesc desc)
    : Expression(block)
{
    type = EXPR_OPER;
    resultOnStack = true;
    operType = desc.oper;
    operDesc = desc;
    left = NULL;
    right = NULL;
}

OperationExpression::~OperationExpression()
{
    if (left != NULL)
    {
        delete left;
    }
    if (right != NULL)
    {
        delete right;
    }
}

void OperationExpression::resolveType()
{
    if (valueType != VALUE_UNKNOWN)
    {
        return;
    }

#ifdef DEBUG_PARSER
    printf("OperationExpression::resolveType: %s\n", toString().c_str());
#endif

    // Figure out types
    if (right == NULL)
    {
        valueType = left->valueType;
#ifdef DEBUG_PARSER
        printf("OperationExpression::resolveType: Expression type=%d (Left Only)\n", valueType);
#endif
    }
    else if (left != NULL)
    {
        ValueType leftType = left->valueType;
        ValueType rightType = right->valueType;

#ifdef DEBUG_PARSER
        printf("OperationExpression::resolveType: Expression BEFORE: type=%d, left=%d, right=%d\n", valueType, left->valueType, right->valueType);
#endif
        if (leftType == rightType)
        {
            // Easy!
            valueType = left->valueType;
        }
        else if (operType == OP_SET && right->valueType != VALUE_UNKNOWN)
        {
            // Assignment: Left becomes type of right
            left->valueType = right->valueType;
            valueType = right->valueType;
        }
/*
        else if (
            (left->valueType == VALUE_INTEGER && right->valueType == VALUE_DOUBLE) ||
            (left->valueType == VALUE_DOUBLE && right->valueType == VALUE_INTEGER))
        {
            // Maths to anything involving a double causes the output to always be a double
            valueType = VALUE_DOUBLE;
        }
*/
        if (left->type == EXPR_VAR)
        {
#ifdef DEBUG_PARSER
            printf("OperationExpression::resolveType:  -> left=VAR %p! type=%d\n", left, left->valueType);
#endif
        }

#ifdef DEBUG_PARSER
        printf("OperationExpression::resolveType: Expression type=%d, left=%d, right=%d\n", valueType, left->valueType, right->valueType);
#endif
    }
}

wstring OperationExpression::toString()
{
    wstring str = L"{";
    switch (operType)
    {
        case OP_SET:
            str +=L"SET";
            break;
        case OP_ADD:
            str +=L"ADD";
            break;
        case OP_SUB:
            str +=L"SUB";
            break;
        case OP_MULTIPLY:
            str +=L"MULTIPLY";
            break;
        case OP_LOGICAL_AND:
            str +=L"LOGICAL_AND";
            break;
        case OP_INCREMENT:
            str +=L"INCREMENT";
            break;
        case OP_LESS_THAN:
            str +=L"LESS_THAN";
            break;
        case OP_REFERENCE:
            str +=L"REFERENCE";
            break;
        case OP_NOT:
            str +=L"NOT";
            break;
        default:
            str += L"?OP?";
            break;
    }
    str += L"-";

    switch (valueType)
    {
        case VALUE_UNKNOWN:
            str += L"UNKNOWN";
            break;
        case VALUE_VOID:
            str += L"VOID";
            break;
        case VALUE_OBJECT:
            str += L"OBJECT";
            break;
        case VALUE_POINTER:
            str += L"POINTER";
            break;
        case VALUE_INTEGER:
            str += L"INTEGER";
            break;
        case VALUE_DOUBLE:
            str += L"DOUBLE";
            break;
        case VALUE_FRAME:
            str += L"FRAME";
            break;
    }
    str += L":";
    if (left != NULL)
    {
        str += left->toString();
    }
    if (right != NULL)
    {
        str += L"," + right->toString();
    }
    str += L"}";
    return str;
}

IfExpression::IfExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_IF;
    testExpr = NULL;
    trueBlock = NULL;
    falseBlock = NULL;
}

IfExpression::~IfExpression()
{
    if (testExpr != NULL)
    {
        delete testExpr;
    }
}

wstring IfExpression::toString()
{
    wstring str = L"IF (";
    if (testExpr != NULL)
    {
        str += testExpr->toString();
    }
    str += L") {";
    if (trueBlock != NULL)
    {
        str += trueBlock->toString();
    }
    str += L"}";

    if (falseBlock != NULL)
    {
        str += L" ELSE {";
        str += falseBlock->toString();
        str += L"}";
    }
    return str;
}


ForExpression::ForExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_FOR;
    initExpr = NULL;
    testExpr = NULL;
    incExpr = NULL;
}

ForExpression::~ForExpression()
{
    if (initExpr != NULL)
    {
        delete initExpr;
    }
    if (testExpr != NULL)
    {
        delete testExpr;
    }
    if (incExpr != NULL)
    {
        delete incExpr;
    }
}

wstring ForExpression::toString()
{
    wstring str = L"FOR (";
    if (initExpr != NULL)
    {
        str += initExpr->toString();
    }
    str += L" ; ";
    if (testExpr != NULL)
    {
        str += testExpr->toString();
    }
    str += L" ; ";
    if (incExpr != NULL)
    {
        str += incExpr->toString();
    }
    str += L" )  {";
    str += body->toString();
    str += L" }";
    return str;
}

ReturnExpression::ReturnExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_RETURN;
    returnValue = NULL;

    // This is a special case. The result isn't on *this* frame's stack, but the callee's stack!
    resultOnStack = false;
}

wstring ReturnExpression::toString()
{
    wstring str = L"{RETURN";
    if (returnValue != NULL)
    {
        str += L":";
        str += returnValue->toString();
    }
    str += L"}";
    return str;
}

TryExpression::TryExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_TRY;
    tryBlock = NULL;
    catchBlock = NULL;
}

wstring TryExpression::toString()
{
    wstring str = L"{TRY:";
    if (tryBlock != NULL)
    {
        str += tryBlock->toString();
    }
    str += L":";
    if (catchBlock != NULL)
    {
        str += catchBlock->toString();
    }
    str += L"}";
    return str;
}


ThrowExpression::ThrowExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_THROW;
    throwValue = NULL;
}

wstring ThrowExpression::toString()
{
    wstring str = L"{THROW";
    if (throwValue != NULL)
    {
        str += L":";
        str += throwValue->toString();
    }
    str += L"}";
    return str;
}


VarExpression::VarExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_VAR;
    clazz = NULL;
}

wstring VarExpression::toString()
{
    return L"{VAR:" + var + L"}";
}

ArrayExpression::ArrayExpression(CodeBlock* block)
    : VarExpression(block)
{
    type = EXPR_ARRAY;
}

wstring ArrayExpression::toString()
{
    return L"{ARRAY:" + var + L"[" + indexExpr->toString().c_str() + L"]}";
}

StringExpression::StringExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_STRING;
    resultOnStack = true;
}

wstring StringExpression::toString()
{
    return L"\"" + str + L"\"";
}

IntegerExpression::IntegerExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_INTEGER;
    resultOnStack = true;
}

wstring IntegerExpression::toString()
{
    wchar_t buffer[256];
    swprintf(buffer, 256, L"{INTEGER:%lld}", i);
    return wstring(buffer);
}

DoubleExpression::DoubleExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_DOUBLE;
    resultOnStack = true;
}

wstring DoubleExpression::toString()
{
    return L"DOUBLE";
}

FunctionExpression::FunctionExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_FUNCTION;
}

wstring FunctionExpression::toString()
{
    wchar_t buffer[256];
    swprintf(buffer, 256, L"{FUNCTION:%p}", function);
    return wstring(buffer);
}

