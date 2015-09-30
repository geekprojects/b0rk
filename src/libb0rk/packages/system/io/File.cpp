
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packages/system/io/File.h"
#include "packages/system/lang/StringClass.h"

using namespace std;

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

bool File::init(Context* context, Object* instance, int argCount)
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

    context->pushVoid();
    return true;
}

bool File::write(Context* context, Object* instance, int argCount)
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
        Value v = context->pop();
        string str;

        if (v.type == VALUE_OBJECT &&
            v.object != NULL &&
            v.object->getClass()->getName() == "system.lang.String")
        {
            str = String::getString(context, v.object);
        }
        else
        {
            str = v.toString().c_str();
        }
        ::write(fd, str.c_str(), str.length());
    }

    context->pushVoid();
    return true;
}

