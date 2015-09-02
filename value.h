#ifndef __BSCRIPT_VALUE_H_
#define __BSCRIPT_VALUE_H_

#include <string>

class Object;

enum ValueType
{
   VALUE_UNKNOWN,
   VALUE_VOID,     // 1
   VALUE_VARIABLE, // 2
   VALUE_OBJECT,   // 3
   VALUE_POINTER,  // 4
   VALUE_INTEGER,  // 5
   VALUE_DOUBLE,   // 6
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
