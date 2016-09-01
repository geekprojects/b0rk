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

Class::Class(Class* superClass, wstring name)
{
    m_superClass = superClass;
    m_name = name;

    if (m_superClass != NULL)
    {
        m_fieldStartId = m_superClass->getFieldCount();
        m_fieldCount = m_superClass->getFieldCount() + 1;
        m_staticFieldStartId = m_superClass->getStaticFieldCount();
        m_staticFieldCount = m_superClass->getStaticFieldCount();
    }
    else
    {
        m_fieldStartId = 0;
        m_fieldCount = 1;
        m_staticFieldStartId = 0;
        m_staticFieldCount = 0;
    }
}


Class::~Class()
{
    map<std::wstring, Function*>::iterator methIt;
    for (methIt = m_methods.begin(); methIt != m_methods.end(); methIt++)
    {
        delete methIt->second;
    }
}

void Class::addField(string name)
{
    addField(Utils::string2wstring(name));
}

void Class::addField(wstring name)
{
    size_t id = m_fieldCount;
    m_fields.insert(make_pair(name, id));
    m_fieldCount++;
}

int Class::getFieldId(wstring name)
{
    map<std::wstring, size_t>::iterator it;
    it = m_fields.find(name);
    if (it != m_fields.end())
    {
        return it->second;
    }

    if (m_superClass != NULL)
    {
        return m_superClass->getFieldId(name);
    }

    return -1;
}

int Class::addStaticField(string name)
{
    return addStaticField(Utils::string2wstring(name));
}

int Class::addStaticField(wstring name)
{
    size_t id = m_staticFieldCount;
    m_staticFields.insert(make_pair(name, id));
    m_staticFieldCount++;

    Value initValue;
    initValue.type = VALUE_VOID;
    initValue.i = 0;
    m_staticValues.push_back(initValue);

    return id;
}

int Class::getStaticFieldId(wstring name)
{
    map<std::wstring, size_t>::iterator it;
    it = m_staticFields.find(name);
    if (it != m_staticFields.end())
    {
        return it->second;
    }

    if (m_superClass != NULL)
    {
        return m_superClass->getStaticFieldId(name);
    }

    return -1;
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

