#ifndef __BSCRIPT_VALUE_H_
#define __BSCRIPT_VALUE_H_

#include <string>

class Object;

enum ValueType
{
   VALUE_VARIABLE,
   VALUE_OBJECT,
   VALUE_POINTER,
   VALUE_INTEGER,
   VALUE_DOUBLE,
};

struct Value
{
    ValueType type;
    union
    {
        Value* variable;
        Object* object;
        void* pointer;
        int64_t i;
        double d;
    };

    std::string toString();
};

#endif
