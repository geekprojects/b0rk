#ifndef __BSCRIPT_VALUE_H_
#define __BSCRIPT_VALUE_H_

#include <string.h>
#include <string>

namespace b0rk
{

struct Object;

enum ValueType
{
   VALUE_UNKNOWN,
   VALUE_VOID,     // 1
   VALUE_VARIABLE, // 2
   VALUE_OBJECT,   // 3
   VALUE_POINTER,  // 4
   VALUE_INTEGER,  // 5
   VALUE_DOUBLE,   // 6
   VALUE_FRAME = 0x1000,
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

/*
    Value()
    {
        type = VALUE_UNKNOWN;
    }
*/

    std::string toString();
};

};

#endif
