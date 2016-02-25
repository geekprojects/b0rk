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


#undef DEBUG_STRING

#include "packages/system/lang/StringClass.h"
#include <b0rk/context.h>
#include <b0rk/utils.h>

using namespace std;
using namespace b0rk;

String::String()
    : Class(NULL, "system.lang.String")
{
    addMethod("String", new NativeFunction(this, (nativeFunction_t)&String::constructor));

    addMethod("operator+", new NativeObjectFunction(this, (nativeObjectFunction_t)&StringNative::addOperator));
    addMethod("length", new NativeObjectFunction(this, (nativeObjectFunction_t)&StringNative::length));
    addMethod("at", new NativeObjectFunction(this, (nativeObjectFunction_t)&StringNative::at));
}

String::~String()
{
}

bool String::constructor(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
#ifdef DEBUG_STRING
    printf("String::constructor: argCount=%d\n", argCount);
#endif

    if (argCount == 0)
    {
        instance->m_nativeObject = new StringNative(instance, "");
    }
    else if (argCount == 1)
    {
        wstring str;
        if (args[0].type == VALUE_OBJECT &&
            args[0].object != NULL &&
            args[0].object->getClass() == this)
        {
            StringNative* strNative = (StringNative*)(args[0].object->m_nativeObject);
            str = strNative->getString();
        }
        else
        {
            str = args[0].toString();
        }
 
#ifdef DEBUG_STRING
        printf("String::constructor: str=%ls\n", str.c_str());
#endif
        instance->m_nativeObject = new StringNative(instance, str);
    }
    else
    {
        fprintf(stderr, "String::constructor: Expected 0 or 1 arguments, got %d\n", argCount);
        return false;
    }

    return true;
}

StringNative::StringNative(Object* instance, wstring str)
    : NativeObject(instance)
{
    m_string = str;
}

bool StringNative::addOperator(Context* context, int argCount, Value* args, Value& result)
{
    Value rhs = args[0];

    wstring rhsStr;
    if (rhs.type == VALUE_OBJECT && rhs.object != NULL && rhs.object->getClass()->getName() == L"system.lang.String")
    {
        rhsStr =  String::getString(context, rhs.object);
#ifdef DEBUG_STRING
        printf("String::addOperator: rhs (String): %ls\n", rhsStr.c_str());
#endif
    }
    else
    {
        rhsStr = rhs.toString();
#ifdef DEBUG_STRING
        printf("String::addOperator: rhs (other): %ls\n", rhsStr.c_str());
#endif
    }

    wstring resultstr = m_string + rhsStr;

    result.type = VALUE_OBJECT;
    result.object = String::createString(context, resultstr);

    return true;
}

bool StringNative::length(Context* context, int argCount, Value* args, Value& result)
{
    result.type = VALUE_INTEGER;
    result.i = m_string.length();

    return true;
}

bool StringNative::at(Context* context, int argCount, Value* args, Value& result)
{
    if (argCount != 1)
    {
        return false;
    }

    Value idx = args[0];
    wchar_t c = m_string.at(idx.i);

    result.type = VALUE_INTEGER;
    result.i = c;

    return true;
}

Object* String::createString(Context* context, std::string str)
{
    Object* object = context->getRuntime()->allocateObject(context->getRuntime()->getStringClass());
    if (object == NULL)
    {
        return NULL;
    }

    object->m_nativeObject = new StringNative(object, str);
#ifdef DEBUG_STRING
    printf("String::createString: str=%s\n", str.c_str());
#endif

    return object;
}

Object* String::createString(Context* context, wstring str)
{
    Object* object = context->getRuntime()->allocateObject(context->getRuntime()->getStringClass());
    if (object == NULL)
    {
        return NULL;
    }

    object->m_nativeObject = new StringNative(object, str);
#ifdef DEBUG_STRING
    printf("String::createString: str=%ls\n", str.c_str());
#endif

    return object;
}


std::wstring String::getString(Context* context, Value& value)
{
    if (value.type == VALUE_OBJECT)
    {
        return getString(context, value.object);
    }
    else
    {
        return value.toString();
    }
}

std::wstring String::getString(Context* context, Object* obj)
{
    if (obj == NULL)
    {
        return L"INVALID";
    }
    if (obj->getClass() != context->getRuntime()->getStringClass())
    {
        return L"NOTASTRING";
    }
    return ((StringNative*)obj->m_nativeObject)->getString();
}

