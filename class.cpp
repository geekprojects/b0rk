
#include "class.h"

using namespace std;

Class::Class(string name)
{
    m_name = name;
}

Class::~Class()
{
}

size_t Class::getValueCount()
{
    return m_fields.size();
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
            return i;
        }
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
    return NULL;
}

