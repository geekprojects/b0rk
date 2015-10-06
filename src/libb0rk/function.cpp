
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <b0rk/function.h>
#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;

Function::Function(Class* clazz)
{
    m_class = clazz;
    m_static = false;
}

Function::Function(Class* clazz, vector<string> args)
{
    m_class = clazz;
    m_args = args;
    m_static = false;
}

Function::~Function()
{
}

int Function::getArgId(string arg)
{
    unsigned int i;
    for (i = 0; i < m_args.size(); i++)
    {
        if (m_args[i] == arg)
        {
            return i + 1;
        }
    }
    return -1;
}

bool Function::execute(Context* context, Object* instance, int argCount)
{
    return false;
}

NativeFunction::NativeFunction(Class* clazz, nativeFunction_t func)
    : Function(clazz)
{
    m_native = func;
}

bool NativeFunction::execute(Context* context, Object* instance, int argCount)
{
    Value args[argCount];

    // Populate the args array
    // TODO: This will make objects in arrays invisible to the GC!
    int i;
    for (i = 0; i < argCount; i++)
    {
        args[(argCount - 1) - i] = context->pop();
    }

    Value result;
    result.type = VALUE_VOID;
    result.i = 0;

    bool res = (m_class->*m_native)(context, instance, argCount, args, result);

    if (!res)
    {
        return false;
    }

    context->push(result);
    return true;
}

