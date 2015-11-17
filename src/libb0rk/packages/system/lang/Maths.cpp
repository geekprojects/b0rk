
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packages/system/lang/Maths.h"

using namespace std;
using namespace b0rk;

Maths::Maths() : Class(NULL, "system.lang.Maths")
{
    addMethod("randomInt", new NativeFunction(this, (nativeFunction_t)&Maths::randomInt, true));
}

Maths::~Maths()
{
}

bool Maths::randomInt(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    if (argCount != 2)
    {
        printf("Maths::randomInt: Incorrect number of arguments\n");
        return false;
    }

    int from = args[0].i;
    int to = args[1].i;

    result.type = VALUE_INTEGER;
    result.i = m_random.range(from, to);

    return true;
}

