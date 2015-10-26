
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <b0rk/function.h>
#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;

Function::Function(Class* clazz)
{
    m_name = "<anonymous>";
    m_class = clazz;
    m_static = false;
}

Function::Function(Class* clazz, vector<string> args)
{
    m_name = "<anonymous>";
    m_class = clazz;
    m_args = args;
    m_static = false;
}

Function::~Function()
{
}

string Function::getFullName()
{
    string name = "";
    if (m_class != NULL)
    {
        name = m_class->getName() + "::";
    }
    name += m_name;
    return name;
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

bool Function::execute(Context* context, Object* instance, int argCount, Value* argv, Value& result)
{
    int i;
    for (i = 0; i < argCount; i++)
    {
        context->push(argv[i]);
    }
    bool res;
    res = execute(context, instance, argCount);
    if (!res)
    {
        return false;
    }
    result = context->pop();
    return true;
}

bool Function::execute(Context* context, Object* instance, int argCount)
{
    return false;
}

NativeFunction::NativeFunction(Class* clazz, nativeFunction_t func, bool isStatic)
    : Function(clazz)
{
    m_native = func;
    m_static = isStatic;
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

NativeObjectFunction::NativeObjectFunction(Class* clazz, nativeObjectFunction_t func)
    : Function(clazz)
{
    m_native = func;
}

bool NativeObjectFunction::execute(Context* context, Object* instance, int argCount)
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

#if 0
    printf("NativeObjectFunction::execute: instance=%p\n", instance);
    printf("NativeObjectFunction::execute: native object=%p\n", instance->m_nativeObject);
#endif

    bool res = ((instance->m_nativeObject)->*m_native)(context, argCount, args, result);

    if (!res)
    {
        return false;
    }

    context->push(result);
    return true;
}

