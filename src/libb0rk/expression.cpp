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

string CallExpression::toString()
{
    return "CALL " + function + "(" + argsToString() + ")";
}

string CallExpression::argsToString()
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

NewExpression::NewExpression(CodeBlock* block)
    : CallExpression(block)
{
    type = EXPR_NEW;
}

std::string NewExpression::toString()
{
    return "NEW " + clazz.toString() + "(" + argsToString() + ")";
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
    else
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

string OperationExpression::toString()
{
    string str = "{";
    switch (operType)
    {
        case OP_SET:
            str +="SET";
            break;
        case OP_ADD:
            str +="ADD";
            break;
        case OP_SUB:
            str +="SUB";
            break;
        case OP_MULTIPLY:
            str +="MULTIPLY";
            break;
        case OP_LOGICAL_AND:
            str +="LOGICAL_AND";
            break;
        case OP_INCREMENT:
            str +="INCREMENT";
            break;
        case OP_LESS_THAN:
            str +="LESS_THAN";
            break;
        case OP_REFERENCE:
            str +="REFERENCE";
            break;
        default:
            str += "?OP?";
            break;
    }
    str += "-";

    switch (valueType)
    {
        case VALUE_UNKNOWN:
            str += "UNKNOWN";
            break;
        case VALUE_VOID:
            str += "VOID";
            break;
        case VALUE_VARIABLE:
            str += "VARIABLE";
            break;
        case VALUE_OBJECT:
            str += "OBJECT";
            break;
        case VALUE_POINTER:
            str += "POINTER";
            break;
        case VALUE_INTEGER:
            str += "INTEGER";
            break;
        case VALUE_DOUBLE:
            str += "DOUBLE";
            break;
        case VALUE_FRAME:
            str += "FRAME";
            break;
    }
    str += ":";
    if (left != NULL)
    {
        str += left->toString();
    }
    if (right != NULL)
    {
        str += "," + right->toString();
    }
    str += "}";
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

string IfExpression::toString()
{
    std::string str = "IF (";
    if (testExpr != NULL)
    {
        str += testExpr->toString();
    }
    str += ") {";
    if (trueBlock != NULL)
    {
        str += trueBlock->toString();
    }
    str += "}";

    if (falseBlock != NULL)
    {
        str += " ELSE {";
        str += falseBlock->toString();
        str += "}";
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

string ForExpression::toString()
{
    std::string str = "FOR (";
    if (initExpr != NULL)
    {
        str += initExpr->toString();
    }
    str += " ; ";
    if (testExpr != NULL)
    {
        str += testExpr->toString();
    }
    str += " ; ";
    if (incExpr != NULL)
    {
        str += incExpr->toString();
    }
    str += " )  {";
    str += body->toString();
    str += " }";
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

string ReturnExpression::toString()
{
    string str = "{RETURN";
    if (returnValue != NULL)
    {
        str += ":";
        str += returnValue->toString();
    }
    str += "}";
    return str;
}

VarExpression::VarExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_VAR;
    clazz = NULL;
}

string VarExpression::toString()
{
    return "{VAR:" + var + "}";
}

ArrayExpression::ArrayExpression(CodeBlock* block)
    : VarExpression(block)
{
    type = EXPR_ARRAY;
}

string ArrayExpression::toString()
{
    return "{ARRAY:" + var + "[" + indexExpr->toString().c_str() + "]}";
}

StringExpression::StringExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_STRING;
    resultOnStack = true;
}

string StringExpression::toString()
{
    return "\"" + str + "\"";
}

IntegerExpression::IntegerExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_INTEGER;
    resultOnStack = true;
}

string IntegerExpression::toString()
{
    char buffer[256];
    snprintf(buffer, 256, "{INTEGER:%d}", i);
    return string(buffer);
}

DoubleExpression::DoubleExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_DOUBLE;
    resultOnStack = true;
}

string DoubleExpression::toString()
{
    return "DOUBLE";
}

FunctionExpression::FunctionExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_FUNCTION;
}

string FunctionExpression::toString()
{
    char buffer[256];
    snprintf(buffer, 256, "{FUNCTION:%p}", function);
    return string(buffer);
}

