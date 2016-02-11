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

#include "packages/system/lang/Array.h"

using namespace std;
using namespace b0rk;

Array::Array() : Class(NULL, "system.lang.Array")
{
    addField("Array");

    addMethod("Array", new NativeFunction(this, (nativeFunction_t)&Array::constructor));
}

Array::~Array()
{
}

bool Array::constructor(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    ArrayContainer* ac = new ArrayContainer();
    Value acValue;
    acValue.type = VALUE_POINTER;
    acValue.pointer = ac;

    instance->setValue(0, acValue);

    // No result
    result.type = VALUE_VOID;

    return true;
}

ArrayContainer* Array::getContainer(Object* array)
{
    Value acValue = array->getValue(0);
    if (acValue.type == VALUE_POINTER && acValue.pointer != NULL)
    {
        return (ArrayContainer*)acValue.pointer;
    }
    return NULL;
}

bool Array::load(Object* array, Value index, Value& value)
{
    ArrayContainer* ac = getContainer(array);
    string indexStr = index.toString();
    map<string, Value>::iterator it = ac->array.find(index.toString());
    if (it == ac->array.end())
    {
        printf("Array::load: %p[%s]: Not found!\n", array, index.toString().c_str());
        value.type = VALUE_VOID;
        value.i = 0;
        return true;
    }
    value = it->second;
    return true;
}

bool Array::store(Object* array, Value index, Value value)
{
    ArrayContainer* ac = getContainer(array);
    ac->array.insert(make_pair( index.toString(), value));
    return true;
}

