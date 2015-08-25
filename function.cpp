
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "function.h"
#include "string.h"

using namespace std;

Function::Function(Class* clazz)
{
    m_class = clazz;
    m_static = false;
}

Function::~Function()
{
}

bool Function::execute(Context* context, Object* instance)
{
    return false;
}

NativeFunction::NativeFunction(Class* clazz, nativeFunction_t func)
    : Function(clazz)
{
    m_native = func;
}

bool NativeFunction::execute(Context* context, Object* instance)
{
    printf("NativeFunction::execute: Here!\n");
    return (m_class->*m_native)(context, instance);
}

