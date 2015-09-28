
#undef DEBUG_STRING

#include "packages/system/lang/StringClass.h"
#include <b0rk/context.h>

using namespace std;

String::String()
    : Class(NULL, "system.lang.String")
{
    addField("data"); // 0
    addMethod("system.lang.String", new NativeFunction(this, (nativeFunction_t)&String::constructor));
    addMethod("operator+", new NativeFunction(this, (nativeFunction_t)&String::addOperator));
}

String::~String()
{
}

bool String::constructor(Context* context, Object* instance, int argCount)
{
#ifdef DEBUG_STRING
    printf("String::constructor: argCount=%d\n", argCount);
#endif
    if (argCount == 1)
    {
        Value arg = context->pop();
        Value v;
        if (arg.type == VALUE_OBJECT &&
            arg.object != NULL &&
            arg.object->getClass() == this)
        {
            v.pointer = strdup((const char*)(arg.object->getValue(0).pointer));
        }
        else
        {
            v.pointer = strdup(arg.toString().c_str());
        }
        instance->setValue(0, v);
    }

    // No result
    context->pushVoid();
    return true;
}

bool String::addOperator(Context* context, Object* instance, int argCount)
{
    Value rhs = context->pop();

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

    Value result;
    result.type = VALUE_OBJECT;
    result.object = createString(context, resultstr.c_str());
    context->push(result);
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


