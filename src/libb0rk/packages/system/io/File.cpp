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

#define BUFFER_SIZE 1024

using namespace std;
using namespace b0rk;

File::File() : Class(NULL, L"system.io.File")
{
    addMethod("<staticinit>", new NativeFunction(this, (nativeFunction_t)&File::init));
    addMethod("write", new NativeFunction(this, (nativeFunction_t)&File::write));
    addMethod("writeln", new NativeFunction(this, (nativeFunction_t)&File::writeln));

    addField("fileDescriptor");

    addStaticField("out");
}

File::~File()
{
}

bool File::init(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    Class* fileClass = context->getRuntime()->findClass(context, L"system.io.File");
    Object* outFile = context->getRuntime()->allocateObject(fileClass);

    m_fdFieldId = getFieldId(L"fileDescriptor");

    // stdout
    Value outValue;
    outValue.type = VALUE_POINTER;
    outValue.pointer = stdout;
    outFile->setValue(m_fdFieldId, outValue);

    Value outObjValue;
    outObjValue.type = VALUE_OBJECT;
    outObjValue.object = outFile;

    int outId = getStaticFieldId(L"out");
    fileClass->setStaticField(outId, outObjValue);

    result.type = VALUE_VOID;

    return true;
}

bool File::write(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    FILE* fd = getDescriptor(instance);
    doWrite(context, fd, argCount, args);

    result.type = VALUE_VOID;
    return true;
}

bool File::writeln(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    FILE* fd = getDescriptor(instance);
    doWrite(context, fd, argCount, args);
    fputs("\n", fd);

    result.type = VALUE_VOID;
    return true;
}


FILE* File::getDescriptor(Object* instance) const
{
    FILE* fd;
    if (instance != NULL)
    {
        int fdId = m_fdFieldId;
        fd = (FILE*)instance->getValue(fdId).pointer;
    }
    else
    {
        // Called statically. Just use stdout
        fd = stdout;
    }
    return fd;
}

bool File::doWrite(Context* context, FILE* fd, int argCount, Value* args)
{
    char outbuffer[BUFFER_SIZE];

    int i;
    for (i = 0; i < argCount; i++)
    {
        Value v = args[i];
        wstring str;

        if (v.type == VALUE_OBJECT &&
            v.object != NULL &&
            v.object->getClass() == context->getRuntime()->getStringClass())
        {
            str = String::getString(context, v.object);
        }
        else
        {
            str = v.toString();
        }

        unsigned int strlength = str.length();
        unsigned int j;
        char* outpos = outbuffer;
        for (j = 0; j < strlength; j++)
        {
            wchar_t c = str[j];

            outpos = utf8::append(c, outpos);
            unsigned int len = (outpos - outbuffer);
            if (j >= strlength - 1 || len > BUFFER_SIZE - 4)
            {
                fwrite(outbuffer, len, 1, fd);
                outpos = outbuffer;
            }
        }
    }

    return true;
}

