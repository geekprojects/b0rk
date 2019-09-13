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
#include <time.h>

#include "packages/system/lang/RuntimeClass.h"

using namespace std;
using namespace b0rk;

RuntimeClass::RuntimeClass() : Class(NULL, L"system.lang.Runtime")
{
    addField("Runtime");

    addMethod("exit", new NativeFunction(this, (nativeFunction_t)&RuntimeClass::exitB));
}

RuntimeClass::~RuntimeClass()
{
}

bool RuntimeClass::exitB(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    if (argCount != 1)
    {
        return false;
    }

    Value status = args[0];

    exit(status.i);

    // Should never reach here!
    return false;
}

