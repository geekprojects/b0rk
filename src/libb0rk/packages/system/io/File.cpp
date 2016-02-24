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
#include <b0rk/utf8.h>

#include "packages/system/io/File.h"
#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;

File::File() : Class(NULL, "system.io.File")
{
    addMethod("<staticinit>", new NativeFunction(this, (nativeFunction_t)&File::init));
    addMethod("write", new NativeFunction(this, (nativeFunction_t)&File::write));

    addField("fileDescriptor");

    addStaticField("out");
}

File::~File()
{
}

bool File::init(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    Class* fileClass = context->getRuntime()->findClass(context, "system.io.File");
    Object* outFile = context->getRuntime()->allocateObject(fileClass);

    int fdId = getFieldId("fileDescriptor");

    // stdout
    Value outValue;
    outValue.type = VALUE_INTEGER;
    outValue.i = 1;
    outFile->setValue(fdId, outValue);

    Value outObjValue;
    outObjValue.type = VALUE_OBJECT;
    outObjValue.object = outFile;

    int outId = getStaticFieldId("out");
    fileClass->setStaticField(outId, outObjValue);

    result.type = VALUE_VOID;

    return true;
}

bool File::write(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    int fd;
    if (instance != NULL)
    {
        int fdId = getFieldId("fileDescriptor");
        fd = instance->getValue(fdId).i;
    }
    else
    {
        // Called statically. Just use stdout
        fd = 1;
    }

    int i;
    for (i = 0; i < argCount; i++)
    {
        Value v = args[i];
        wstring str;

        if (v.type == VALUE_OBJECT &&
            v.object != NULL &&
            v.object->getClass()->getName() == L"system.lang.String")
        {
            str = String::getString(context, v.object);
        }
        else
        {
            str = v.toString();
        }

        int j;
        for (j = 0; j < str.length(); j++)
        {
            char out[4];
            char* end = utf8::append(str[j], out);
            ::write(fd, out, end - out);
        }
//        ::write(fd, str.c_str(), str.length());
    }

    result.type = VALUE_VOID;

    return true;
}

