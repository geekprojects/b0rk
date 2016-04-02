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
#include <string.h>
#include <ctype.h>

#include <cinttypes>

#include <b0rk/lexer.h>
#include <b0rk/parser.h>
#include <b0rk/assembler.h>
#include <b0rk/executor.h>
#include <b0rk/utils.h>

#include "packages/system/lang/Array.h"
#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return 0;
    }

    Runtime* runtime = new Runtime();
    Context* context = runtime->createContext();

    string className = string(argv[1]);
    Class* clazz = runtime->findClass(context, Utils::string2wstring(className));
    if (clazz == NULL)
    {
        printf("%s: Unable to find class %s\n", argv[0], className.c_str());
        return 0;
    }

    Function* mainFunc = clazz->findMethod(L"main");
    if (mainFunc == NULL)
    {
        printf("%s: No main method found in class\n", argv[0]);
        return 0;
    }
    if (!mainFunc->getStatic())
    {
        printf("%s: Main is not static!\n", argv[0]);
        return 0;
    }

    Object* argsObj = runtime->newArray(context, argc - 2);
    int i;
    for (i = 2; i < argc; i++)
    {
        Value idx;
        idx.type = VALUE_INTEGER;
        idx.i = i - 2;
        Value value;
        value.type = VALUE_OBJECT;
        value.object = String::createString(context, argv[i]);
        ((Array*)runtime->getArrayClass())->store(context, argsObj, idx, value);
    }

    Value argsValue;
    argsValue.type = VALUE_OBJECT;
    argsValue.object = argsObj;

    Value result;
    mainFunc->execute(context, NULL, 1, &argsValue, result);

    delete runtime;

    return 0;
}

