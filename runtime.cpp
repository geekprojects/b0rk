
#include <stdio.h>

#include "runtime.h"
#include "string.h"

using namespace std;

class System : public Class
{
 private:

 public:
    System();
    ~System();

    bool log(Context* context);
};

System::System() : Class("System")
{
addMethod("log", new NativeFunction(this, (nativeFunction_t)&System::log));
}

System::~System()
{
}

bool System::log(Context* context)
{
Value v = context->pop();

printf("System::log: here! obj=%p\n", v.v.object);

if (v.v.object->getClass()->getName() == "String")
{
Object* strObj = v.v.object;
printf("System::log: %s\n", (char*)strObj->data);
}
return true;
}

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

