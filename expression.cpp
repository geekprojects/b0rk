
#include "expression.h"

using namespace std;

CallExpression::CallExpression()
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

NewExpression::NewExpression()
{
    type = EXPR_NEW;
}

std::string NewExpression::toString()
{
    return "NEW " + clazz.toString() + "(" + argsToString() + ")";
}

OperationExpression::OperationExpression()
{
    type = EXPR_OPER;
}

string OperationExpression::toString()
{
    string str = left->toString() + " OP";
    if (right != NULL)
    {
        str + " " + right->toString();
    }
    return str;
}

ForExpression::ForExpression()
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

VarExpression::VarExpression()
{
    type = EXPR_VAR;
}

string VarExpression::toString()
{
    return var.toString();
}

StringExpression::StringExpression()
{
    type = EXPR_STRING;
}

string StringExpression::toString()
{
    return "\"" + str + "\"";
}

IntegerExpression::IntegerExpression()
{
    type = EXPR_INTEGER;
}

string IntegerExpression::toString()
{
    return "INTEGER";
}

DoubleExpression::DoubleExpression()
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
}

int CodeBlock::setStartingVarId(int id)
{
    m_startingVarId = id;
    m_maxVarId = id + m_vars.size();

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

