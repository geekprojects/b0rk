
#include <stdio.h>

#include "system.h"
#include "string.h"

using namespace std;

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

    if (v.type == VALUE_OBJECT && v.object->getClass()->getName() == "String")
    {
        printf("System::log: %s\n", String::getString(context, v.object).c_str());
    }
    else
    {
        printf("System::log: %s\n", v.toString().c_str());
    }
    return true;
}

