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

#include <b0rk/class.h>
#include <b0rk/utils.h>

using namespace std;
using namespace b0rk;

Class::Class(Class* superClass, string name)
{
    m_superClass = superClass;
    m_name = Utils::string2wstring(name);

    m_fieldStartId = 0;
    if (m_superClass != NULL)
    {
        m_fieldStartId = m_superClass->getFieldCount();
    }

    m_staticValues = NULL;
}

Class::Class(Class* superClass, wstring name)
{
    m_superClass = superClass;
    m_name = name;

    m_fieldStartId = 0;
    if (m_superClass != NULL)
    {
        m_fieldStartId = m_superClass->getFieldCount();
    }

    m_staticValues = NULL;
}


Class::~Class()
{
    map<std::wstring, Function*>::iterator methIt;
    for (methIt = m_methods.begin(); methIt != m_methods.end(); methIt++)
    {
        delete methIt->second;
    }

    if (m_staticValues != NULL)
    {
        delete[] m_staticValues;
    }
}

size_t Class::getFieldCount()
{
    return m_fieldStartId + m_fields.size();
}

void Class::addField(string name)
{
    addField(Utils::string2wstring(name));
}

void Class::addField(wstring name)
{
    m_fields.push_back(name);
}

int Class::getFieldId(wstring name)
{
    unsigned int i;
    for (i = 0; i < m_fields.size(); i++)
    {
        if (m_fields[i] == name)
        {
            return i + m_fieldStartId;
        }
    }

    if (m_superClass != NULL)
    {
        return m_superClass->getFieldId(name);
    }

    return -1;
}

void Class::addStaticField(string name)
{
    addStaticField(Utils::string2wstring(name));
}

void Class::addStaticField(wstring name)
{
    m_staticFields.push_back(name);
}

size_t Class::getStaticFieldCount()
{
    return m_staticFields.size();
}

int Class::getStaticFieldId(wstring name)
{
    unsigned int i;
    for (i = 0; i < m_staticFields.size(); i++)
    {
        if (m_staticFields[i] == name)
        {
            return i;// + m_fieldStartId;
        }
    }
/*
    if (m_superClass != NULL)
    {
        return m_superClass->getFieldId(name);
    }
*/
    return -1;
}

void Class::initStaticFields()
{
    int count = m_staticFields.size();
    m_staticValues = new Value[count];

    int i;
    for (i = 0; i < count; i++)
    {
        m_staticValues[i].type = VALUE_VOID;
    }
}

void Class::addMethod(string name, Function* function)
{
    addMethod(Utils::string2wstring(name), function);
}

void Class::addMethod(wstring name, Function* function)
{
    function->setName(name);
    m_methods.insert(make_pair(name, function));
}

Function* Class::findMethod(wstring name)
{
    map<wstring, Function*>::iterator it;
    it = m_methods.find(name);
    if (it != m_methods.end())
    {
        return it->second;
    }

    if (m_superClass != NULL)
    {
        return m_superClass->findMethod(name);
    }

    return NULL;
}

