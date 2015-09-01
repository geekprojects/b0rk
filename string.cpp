
#include "string.h"
#include "context.h"

#include <string.h>

using namespace std;

String::String()
    : Class("String")
{
    addField("data"); // 0
    addMethod("operator+", new NativeFunction(this, (nativeFunction_t)&String::addOperator));
}

String::~String()
{
}

bool String::addOperator(Context* context, Object* instance)
{
    Value rhs = context->pop();

    string resultstr = getString(context, instance) + getString(context, rhs.object);
    Value result;
    result.type = VALUE_OBJECT;
    result.object = createString(context, resultstr.c_str());
    context->push(result);
    return true;
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


