
#include "expression.h"

using namespace std;

Expression::Expression(CodeBlock* _block)
{
    block = _block;
    valueType = VALUE_UNKNOWN;
}

CallExpression::CallExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_CALL;
}

string CallExpression::toString()
{
    return "CALL " + function.toString() + "(" + argsToString() + ")";
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

OperationExpression::OperationExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_OPER;
}

void OperationExpression::resolveType()
{
    printf("Parser::parseExpression: %s\n", toString().c_str());
    // Figure out types
    if (right == NULL)
    {
        valueType = left->valueType;
        printf("Parser::parseExpression: Expression type=%d (Left Only)\n", valueType);
    }
    else
    {
        ValueType leftType = left->valueType;
        ValueType rightType = right->valueType;
        if (leftType == rightType)
        {
            // Easy!
            valueType = left->valueType;
        }
        else if (operType == OP_SET)
        {
            // Assignment: Left becomes type of right
            left->valueType = right->valueType;
            valueType = right->valueType;
        }
        else if (
            (left->valueType == VALUE_INTEGER && right->valueType == VALUE_DOUBLE) ||
            (left->valueType == VALUE_DOUBLE && right->valueType == VALUE_INTEGER))
        {
            // Maths to anything involving a double causes the output to always be a double
            valueType = VALUE_DOUBLE;
        }

        printf("OperationExpression::resolveType: Expression type=%d, left=%d, right=%d\n", valueType, left->valueType, right->valueType);
    }
}

string OperationExpression::toString()
{
    string str = left->toString() + " ";
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
        case OP_INCREMENT:
            str +="INCREMENT";
            break;
        case OP_LESS_THAN:
            str +="LESS_THAN";
            break;
        default:
            str += "?OP?";
            break;
    }
    if (right != NULL)
    {
        str += " " + right->toString();
    }
    return str;
}

ForExpression::ForExpression(CodeBlock* block)
    : Expression(block)
    {
        type = EXPR_FOR;
    }

string ForExpression::toString()
{
    std::string str = "FOR (";
    str += initExpr->toString();
    str += " ; ";
    str += testExpr->toString();
    str += " ; ";
    str += incExpr->toString();
    str += " )  {";
    str += body->toString();
    str += " }";
    return str;
}

VarExpression::VarExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_VAR;
}

string VarExpression::toString()
{
    return var.toString();
}

StringExpression::StringExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_STRING;
}

string StringExpression::toString()
{
    return "\"" + str + "\"";
}

IntegerExpression::IntegerExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_INTEGER;
}

string IntegerExpression::toString()
{
    return "INTEGER";
}

DoubleExpression::DoubleExpression(CodeBlock* block)
    : Expression(block)
{
    type = EXPR_DOUBLE;
}

string DoubleExpression::toString()
{
    return "DOUBLE";
}

CodeBlock::CodeBlock()
{
    m_parent = NULL;
    m_startingVarId = 0;
    m_maxVarId = 0;
    m_varTypes = NULL;
}

int CodeBlock::setStartingVarId(int id)
{
    m_startingVarId = id;
    m_maxVarId = id + m_vars.size();

    m_varTypes = new ValueType[m_vars.size()];
    unsigned int i;
    for (i = 0; i < m_vars.size(); i++)
    {
        m_varTypes[i] = VALUE_UNKNOWN;
    }

    vector<CodeBlock*>::iterator it;
    for (it = m_childBlocks.begin(); it != m_childBlocks.end(); it++)
    {
        int childIdMax = (*it)->setStartingVarId(id + m_vars.size());
        if (childIdMax > m_maxVarId)
        {
            m_maxVarId = childIdMax;
        }
    }
    return m_maxVarId;
}

int CodeBlock::getVarId(string var)
{
    vector<string>::iterator it;
    int i;
    for (i = 0; i < m_vars.size(); i++)
    {
        if (m_vars[i] == var)
        {
            return i + m_startingVarId;
        }
    }
    if (m_parent != NULL)
    {
        return m_parent->getVarId(var);
    }
    return -1;
}

bool CodeBlock::setVarType(int id, ValueType type)
{
    if (id < m_startingVarId)
    {
        return m_parent->setVarType(id, type);
    }
    int thisId = id - m_startingVarId;
    if (m_varTypes[thisId] != VALUE_UNKNOWN)
    {
        printf("CodeBlock::setVarType: Existing type=%d, new type=%d\n", m_varTypes[thisId], type);
        return m_varTypes[thisId] == type;
    }
    else
    {
        printf("CodeBlock::setVarType: Setting type=%d\n", type);
    }
    m_varTypes[thisId] = type;
    return true;
}

ValueType CodeBlock::getVarType(int id)
{
    if (id < m_startingVarId)
    {
        return m_parent->getVarType(id);
    }
    int thisId = id - m_startingVarId;
    return m_varTypes[thisId];
}

string CodeBlock::toString()
{
    std::string str = "";
    std::vector<Expression*>::iterator it;
    for (it = m_code.begin(); it != m_code.end(); it++)
    {
        str += (*it)->toString() + "; ";
    }
    return str;
}

