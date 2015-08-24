
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

