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

#include "packages/system/lang/Exception.h"
#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;

Exception::Exception() : Class(NULL, L"system.lang.Exception")
{
    addField("message");
    addField("stacktrace");

    addMethod("printStackTrace", new NativeFunction(this, (nativeFunction_t)&Exception::printStackTraceB));
}

Exception::~Exception()
{
}

bool Exception::printStackTraceB(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    int stackTraceId = this->getFieldId(L"stacktrace");
    Value stacktraceValue = instance->getValue(stackTraceId);
    StackTrace* stackTrace = (StackTrace*)stacktraceValue.pointer;
    vector<wstring>::iterator it;
    for (it = stackTrace->stacktrace.begin(); it != stackTrace->stacktrace.end(); it++)
    {
        printf("Exception::printStackTrace: %ls\n", (*it).c_str());
    }
    return true;
}

Object* Exception::createException(Context* context, std::string str)
{
    Object* strObj = String::createString(context, str);
    Value v;
    v.type = VALUE_OBJECT;
    v.object = strObj;

    return createException(context, v);
}

Object* Exception::createException(Context* context, Value& value)
{
    Class* exceptionClass = context->getRuntime()->getExceptionClass();
    Object* object = context->getRuntime()->newObject(context, exceptionClass, 0);
    int messageId = exceptionClass->getFieldId(L"message");
    object->setValue(messageId, value);

    int stackTraceId = exceptionClass->getFieldId(L"stacktrace");
    StackTrace* stacktrace = new StackTrace();
    stacktrace->stacktrace = context->getStackTrace();
    Value stacktraceValue;
    stacktraceValue.type = VALUE_POINTER;
    stacktraceValue.pointer = stacktrace;
    object->setValue(stackTraceId, stacktraceValue);

    return object;
}

Object* Exception::getExceptionValue(Context* context, Object* e)
{
    Class* exceptionClass = context->getRuntime()->getExceptionClass();
    int messageId = exceptionClass->getFieldId(L"message");
    Value messageValue = e->getValue(messageId);
    if (messageValue.type != VALUE_OBJECT)
    {
        return NULL;
    }
    return messageValue.object;
}

wstring Exception::getExceptionString(Context* context, Object* e)
{
    Object* messageObj = getExceptionValue(context, e);
    if (messageObj == NULL)
    {
        return L"";
    }
    return String::getString(context, messageObj);
}

