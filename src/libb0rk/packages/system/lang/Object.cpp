
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packages/system/lang/Object.h"

using namespace std;
using namespace b0rk;

ObjectClass::ObjectClass() : Class(NULL, "system.lang.Object")
{
    addField("ObjectClass");

    addMethod("toString", new NativeFunction(this, (nativeFunction_t)&ObjectClass::toString));
}

ObjectClass::~ObjectClass()
{
}

bool ObjectClass::toString(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    printf("ObjectClass::toString: Here!\n");

    // No result
    result.type = VALUE_VOID;

    return true;
}

