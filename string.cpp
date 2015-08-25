
#include "string.h"
#include "context.h"

#include <string.h>

using namespace std;

String::String()
    : Class("String")
{
    addField("data"); // 0
}

String::~String()
{
}

Object* String::createString(Context* context, const char* str)
{
    Class* stringClass = context->getRuntime()->findClass("String");
    Object* object = new Object(stringClass);

    Value v;
    v.pointer = strdup(str);
    object->setValue(0, v);

    return object;
}

std::string String::getString(Context* context, Object* obj)
{
    return string((const char*)obj->getValue(0).pointer);
}

