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
#include <stdlib.h>
#include <unistd.h>

#include <cinttypes>

#include "packages/system/lang/Array.h"
#include "packages/system/lang/Exception.h"

#undef DEBUG_ARRAYS

using namespace std;
using namespace b0rk;

Array::Array() : Class(NULL, "system.lang.Array")
{
    addField("length");
    addField("data");

    addMethod("Array", new NativeFunction(this, (nativeFunction_t)&Array::constructor));
}

Array::~Array()
{
}

bool Array::constructor(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    if (argCount != 1)
    {
        context->throwException(Exception::createException(context, "Wrong number of arguments"));
        return true;
    }

    Value sizeVal = args[0];
#ifdef DEBUG_ARRAYS
    printf("Array::constructor: size=%" PRId64 "\n", sizeVal.i);
#endif

    Object* arrayData = context->getRuntime()->allocateObject(context->getRuntime()->getArrayDataClass(), sizeVal.i);
    Value arrayVal;
    arrayVal.type = VALUE_OBJECT;
    arrayVal.object = arrayData;
    instance->setValue(0, sizeVal);
    instance->setValue(1, arrayVal);

    return true;
}

bool Array::load(Context* context, Object* array, Value index, Value& value)
{
    int size = array->getValue(0).i;

#ifdef DEBUG_ARRAYS
    printf("Array::load: size=%d, index=%ls\n", size, index.toString().c_str());
#endif

    if (index.type != VALUE_INTEGER)
    {
        context->throwException(Exception::createException(context, "Integer index required"));
        return true;
    }
    if (index.i >= size)
    {
        context->throwException(Exception::createException(context, "Index out of bounds"));
        return true;
    }

    Object* arrayData = array->getValue(1).object;
    value = arrayData->m_values[index.i];

    return true;
}

bool Array::store(Context* context, Object* array, Value index, Value value)
{
    int size = array->getValue(0).i;

#ifdef DEBUG_ARRAYS
    printf("Array::store: size=%d, index=%ls\n", size, index.toString().c_str());
#endif

    if (index.type != VALUE_INTEGER)
    {
        context->throwException(Exception::createException(context, "Integer index required"));
        return true;
    }
    if (index.i >= size)
    {
        context->throwException(Exception::createException(context, "Index out of bounds"));
        return true;
    }

    Object* arrayData = array->getValue(1).object;
    arrayData->m_values[index.i] = value;
    return true;
}

