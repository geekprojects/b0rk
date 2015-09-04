
#include "expression.h"
#include "function.h"

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
    }
    str += ":" + left->toString();
    if (right != NULL)
    {
        str += "," + right->toString();
    }
    str += "}";
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
}

string VarExpression::toString()
{
    return "{VAR:" + var.toString() + "}";
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
    else if (m_function != NULL)
    {
        return m_function->getArgId(var);
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
        return m_varTypes[thisId] == type;
    }
    m_varTypes[thisId] = type;
    return true;
}

ValueType CodeBlock::getVarType(int id)
{
    if (id < m_startingVarId)
    {
        if (m_parent != NULL)
        {
            return m_parent->getVarType(id);
        }
        return VALUE_UNKNOWN;
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

