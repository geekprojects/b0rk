#ifndef __BSCRIPT_VALUE_H_
#define __BSCRIPT_VALUE_H_

#include <string>

class Object;

enum ValueType
{
   VALUE_OBJECT,
   VALUE_POINTER
};

struct Value
{
    ValueType type;
    union
    {
        Object* object;
        void* pointer;
    } v;
};

#endif
