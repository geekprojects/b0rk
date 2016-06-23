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

#include <b0rk/utils.h>
#include <b0rk/object.h>

#include "packages/system/lang/Function.h"

using namespace std;
using namespace b0rk;

FunctionClass::FunctionClass() : Class(NULL, L"system.lang.Function")
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

Function* FunctionClass::getFunction(Object* funcObj)
{
    if (funcObj != NULL &&
        funcObj->getClass()->getName() == L"system.lang.Function")
    {
        return (Function*)(funcObj->getValue(0)).object;
    }
    return NULL;
}

