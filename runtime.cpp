
#include <stdio.h>

#include "runtime.h"
#include "string.h"
#include "system.h"

using namespace std;

Runtime::Runtime()
{
    addClass(new System());
    addClass(new String());
}

Runtime::~Runtime()
{
}

void Runtime::addClass(Class* clazz)
{
    m_classes.insert(make_pair(clazz->getName(), clazz));
}

Class* Runtime::findClass(string name)
{
    map<string, Class*>::iterator it;
    it = m_classes.find(name);
    if (it != m_classes.end())
    {
        return it->second;
    }
    return NULL;
}

Context* Runtime::createContext()
{
    return new Context(this);
}

