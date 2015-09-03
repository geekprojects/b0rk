
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

bool System::log(Context* context, Object* instance, int argCount)
{
    int i;
    //printf("System::log: ");
    for (i = 0; i < argCount; i++)
    {
        Value v = context->pop();

        if (v.type == VALUE_OBJECT && v.object->getClass()->getName() == "String")
            {
            printf("%s", String::getString(context, v.object).c_str());
        }
        else
        {
            printf("%s", v.toString().c_str());
        }
    }
    printf("\n");
    return true;
}

