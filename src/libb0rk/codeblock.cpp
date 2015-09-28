
#include <b0rk/codeblock.h>
#include <b0rk/expression.h>
#include <b0rk/function.h>

using namespace std;

CodeBlock::CodeBlock()
{
    m_parent = NULL;
    m_startingVarId = 0;
    m_maxVarId = 0;
    m_varTypes = NULL;
}

CodeBlock::~CodeBlock()
{
    vector<CodeBlock*>::iterator blockIt;
    for (blockIt = m_childBlocks.begin(); blockIt != m_childBlocks.end(); blockIt++)
    {
        delete *blockIt;
    }

    vector<Expression*>::iterator codeIt;
    for (codeIt = m_code.begin(); codeIt != m_code.end(); codeIt++)
    {
        delete *codeIt;
    }

    if (m_varTypes != NULL)
    {
        delete[] m_varTypes;
    }
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

