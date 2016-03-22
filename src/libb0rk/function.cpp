/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <string.h>
#include <math.h>

#include <b0rk/function.h>
#include <b0rk/utils.h>
#include "packages/system/lang/StringClass.h"
#include "packages/system/lang/Exception.h"

using namespace std;
using namespace b0rk;

Function::Function(Class* clazz)
{
    m_name = L"<anonymous>";
    m_class = clazz;
    m_static = false;
}

Function::Function(Class* clazz, vector<wstring> args)
{
    m_name = L"<anonymous>";
    m_class = clazz;
    m_args = args;
    m_static = false;
}

Function::~Function()
{
}

wstring Function::getFullName()
{
    wstring name;
    if (m_class != NULL)
    {
        name = m_class->getName() + L"::";
    }
    name += m_name;
    return name;
}

int Function::getArgId(wstring arg)
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
    if (context->hasException())
    {
        Value e = context->getExceptionValue();
        fprintf(
            stderr,
            "Function::execute: Uncaught exception: %ls\n",
            Exception::getExceptionString(context, e.object).c_str());
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

    if (!context->hasException())
    {
        context->push(result);
    }
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

    if (!context->hasException())
    {
        context->push(result);
    }
    return true;
}

