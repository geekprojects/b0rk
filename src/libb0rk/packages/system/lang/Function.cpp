
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packages/system/lang/Function.h"

using namespace std;

FunctionClass::FunctionClass() : Class(NULL, "system.lang.Function")
{
    addField("function");
    addMethod("Function", new NativeFunction(this, (nativeFunction_t)&FunctionClass::constructor));
}

FunctionClass::~FunctionClass()
{
}

bool FunctionClass::constructor(Context* context, Object* instance, int argCount)
{
Value funcValue = context->pop();

instance->setValue(0, funcValue);

    // No result
    context->pushVoid();

    return true;
}

