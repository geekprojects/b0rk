
#include "class.h"

using namespace std;

Class::Class(Class* superClass, string name)
{
    m_superClass = superClass;
    m_name = name;

    m_fieldStartId = 0;
    if (m_superClass != NULL)
    {
        m_fieldStartId = m_superClass->getFieldCount();
    }
}

Class::~Class()
{
    map<std::string, Function*>::iterator methIt;
    for (methIt = m_methods.begin(); methIt != m_methods.end(); methIt++)
    {
        delete methIt->second;
    }
}

size_t Class::getFieldCount()
{
    return m_fieldStartId + m_fields.size();
}

void Class::addField(string name)
{
    m_fields.push_back(name);
}

int Class::getFieldId(string name)
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

void Class::addMethod(string name, Function* function)
{
    m_methods.insert(make_pair(name, function));
}

Function* Class::findMethod(string name)
{
    map<string, Function*>::iterator it;
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

