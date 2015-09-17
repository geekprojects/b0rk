
#include <stdio.h>

#include "file.h"
#include "string.h"

using namespace std;

File::File() : Class(NULL, "system.io.File")
{
    addMethod("write", new NativeFunction(this, (nativeFunction_t)&File::write));
}

File::~File()
{
}

bool File::write(Context* context, Object* instance, int argCount)
{
    int i;
    //printf("File::write: ");
    for (i = 0; i < argCount; i++)
    {
        Value v = context->pop();

        if (v.type == VALUE_OBJECT && v.object->getClass()->getName() == "system.lang.String")
        {
            printf("%s", String::getString(context, v.object).c_str());
        }
        else
        {
            printf("%s", v.toString().c_str());
        }
    }
    printf("\n");
    context->pushVoid();
    return true;
}

