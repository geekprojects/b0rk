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


#include <b0rk/codeblock.h>
#include <b0rk/expression.h>
#include <b0rk/function.h>

using namespace std;
using namespace b0rk;

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
        //delete[] m_varTypes;
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

int CodeBlock::getVarId(wstring var)
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

    if (m_varTypes == NULL)
    {
        return VALUE_UNKNOWN;
    }

    int thisId = id - m_startingVarId;
    return m_varTypes[thisId];
}

wstring CodeBlock::toString()
{
    wstring str;
    vector<Expression*>::iterator it;
    for (it = m_code.begin(); it != m_code.end(); it++)
    {
        str += (*it)->toString();
        str += ';';
        str += ' ';
    }
    return str;
}

