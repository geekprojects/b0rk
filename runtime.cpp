
#include <stdio.h>

#include "runtime.h"
#include "string.h"
#include "system.h"
#include "executor.h"

using namespace std;

Runtime::Runtime()
{
    addClass(new System());
    addClass(new String());

    m_executor = new Executor();
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

Object* Runtime::newObject(Context* context, Class* clazz, int argCount)
{
    Object* obj = new Object(clazz);

    Function* ctor = clazz->findMethod(clazz->getName());
    if (ctor != NULL)
    {
        bool res = ctor->execute(context, obj, argCount);
        if (!res)
        {
            delete obj;
            return NULL;
        }
    }

    return obj;
}

Context* Runtime::createContext()
{
    return new Context(this);
}

Executor* Runtime::getExecutor()
{
    return m_executor;
}

