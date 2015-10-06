
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "packages/system/lang/Array.h"

using namespace std;
using namespace b0rk;

Array::Array() : Class(NULL, "system.lang.Array")
{
    addField("Array");

    addMethod("Array", new NativeFunction(this, (nativeFunction_t)&Array::constructor));
}

Array::~Array()
{
}

bool Array::constructor(Context* context, Object* instance, int argCount, Value* args, Value& result)
{
    ArrayContainer* ac = new ArrayContainer();
    Value acValue;
    acValue.type = VALUE_POINTER;
    acValue.pointer = ac;

    instance->setValue(0, acValue);

    // No result
    result.type = VALUE_VOID;

    return true;
}

ArrayContainer* Array::getContainer(Object* array)
{
    Value acValue = array->getValue(0);
    if (acValue.type == VALUE_POINTER && acValue.pointer != NULL)
    {
        return (ArrayContainer*)acValue.pointer;
    }
    return NULL;
}

bool Array::load(Object* array, Value index, Value& value)
{
    ArrayContainer* ac = getContainer(array);
    string indexStr = index.toString();
    map<string, Value>::iterator it = ac->array.find(index.toString());
    if (it == ac->array.end())
    {
        printf("Array::load: %p[%s]: Not found!\n", array, index.toString().c_str());
        value.type = VALUE_VOID;
        value.i = 0;
        return true;
    }
    value = it->second;
    return true;
}

bool Array::store(Object* array, Value index, Value value)
{
    ArrayContainer* ac = getContainer(array);
    ac->array.insert(make_pair( index.toString(), value));
    return true;
}

