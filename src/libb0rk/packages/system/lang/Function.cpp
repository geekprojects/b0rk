
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packages/system/lang/Function.h"

using namespace std;
using namespace b0rk;

FunctionClass::FunctionClass() : Class(NULL, "system.lang.Function")
{
    addField("function");
    addMethod("Function", new NativeFunction(this, (nativeFunction_t)&FunctionClass::constructor));
}

FunctionClass::~FunctionClass()
{
}

bool FunctionClass::constructor(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    Value funcValue = args[0];

    instance->setValue(0, funcValue);

    result.type = VALUE_VOID;

    return true;
}

