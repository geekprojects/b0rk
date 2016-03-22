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
#include <math.h>

#include "packages/system/lang/Maths.h"
#include "packages/system/lang/Exception.h"

using namespace std;
using namespace b0rk;

Maths::Maths() : Class(NULL, "system.lang.Maths")
{
    addMethod("randomInt", new NativeFunction(this, (nativeFunction_t)&Maths::randomInt, true));
    addMethod("round", new NativeFunction(this, (nativeFunction_t)&Maths::round, true));
}

Maths::~Maths()
{
}

bool Maths::randomInt(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    if (argCount != 2)
    {
        context->throwException(Exception::createException(context, "Incorrect number of arguments"));
        return true;
    }

    int from = args[0].i;
    int to = args[1].i;

    result.type = VALUE_INTEGER;
    result.i = m_random.range(from, to);

    return true;
}

bool Maths::round(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    if (argCount != 1)
    {
        context->throwException(Exception::createException(context, "Incorrect number of arguments"));
        return true;
    }

    result.type = VALUE_INTEGER;
    result.i = lround(args[0].d);

    return true;
}
 
