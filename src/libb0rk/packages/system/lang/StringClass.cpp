
#undef DEBUG_STRING

#include "packages/system/lang/StringClass.h"
#include <b0rk/context.h>

using namespace std;
using namespace b0rk;

String::String()
    : Class(NULL, "system.lang.String")
{
    addField("data"); // 0
    addMethod("String", new NativeFunction(this, (nativeFunction_t)&String::constructor));
    addMethod("operator+", new NativeFunction(this, (nativeFunction_t)&String::addOperator));
    addMethod("length", new NativeFunction(this, (nativeFunction_t)&String::length));
    addMethod("at", new NativeFunction(this, (nativeFunction_t)&String::at));
}

String::~String()
{
}

bool String::constructor(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
#ifdef DEBUG_STRING
    printf("String::constructor: argCount=%d\n", argCount);
#endif
    if (argCount == 1)
    {
        Value v;
        if (args[0].type == VALUE_OBJECT &&
            args[0].object != NULL &&
            args[0].object->getClass() == this)
        {
            v.pointer = strdup((const char*)(args[0].object->getValue(0).pointer));
        }
        else
        {
            v.pointer = strdup(args[0].toString().c_str());
        }
        instance->setValue(0, v);
    }

    return true;
}

bool String::addOperator(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    Value rhs = args[0];

    string rhsStr = "";
    if (rhs.type == VALUE_OBJECT && rhs.object != NULL && rhs.object->getClass() == this)
    {
        rhsStr =  getString(context, rhs.object);
#ifdef DEBUG_STRING
        printf("String::addOperator: rhs (String): %s\n", rhsStr.c_str());
#endif
    }
    else
    {
        rhsStr += rhs.toString();
#ifdef DEBUG_STRING
        printf("String::addOperator: rhs (other): %s\n", rhsStr.c_str());
#endif
    }

    string resultstr = getString(context, instance) + rhsStr;

    result.type = VALUE_OBJECT;
    result.object = createString(context, resultstr.c_str());

    return true;
}

bool String::length(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    int len = getString(context, instance).length();

    result.type = VALUE_INTEGER;
    result.i = len;

    return true;
}

bool String::at(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    if (argCount != 1)
    {
        return false;
    }
    Value idx = args[0];
    char c = getString(context, instance).at(idx.i);

    result.type = VALUE_INTEGER;
    result.i = c;

    return true;
}

Object* String::createString(Context* context, const char* str)
{
    Class* stringClass = context->getRuntime()->findClass(context, "system.lang.String");

    Object* object = context->getRuntime()->allocateObject(stringClass);
    if (object == NULL)
    {
        return NULL;
    }

    Value v;
    v.type = VALUE_POINTER;
    v.pointer = strdup(str);
    object->setValue(0, v);

    return object;
}

std::string String::getString(Context* context, Object* obj)
{
    if (obj == NULL)
    {
        return "INVALID";
    }
    if (obj->getClass()->getName() != "system.lang.String")
    {
        return "NOTASTRING";
    }
    if (obj->m_class->getFieldCount() == 0)
    {
        return "INVALID";
    }
    if (obj->getValue(0).pointer == NULL)
    {
        return "NULL";
    }
    return string((const char*)obj->getValue(0).pointer);
}


