#ifndef __BSCRIPT_VALUE_H_
#define __BSCRIPT_VALUE_H_

#include <string>

class Object;

enum ValueType
{
   VALUE_UNKNOWN,
   VALUE_VARIABLE, // 1
   VALUE_OBJECT,   // 2
   VALUE_POINTER,  // 3
   VALUE_INTEGER,  // 4
   VALUE_DOUBLE,   // 5
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
