
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

    printf("System::log: here! obj=%p\n", v.v.object);

    if (v.v.object->getClass()->getName() == "String")
    {
        //Object* strObj = v.v.object;
        printf("System::log: %s\n", String::getString(context, v.v.object).c_str());
    }
    return true;
}

