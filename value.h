#ifndef __BSCRIPT_VALUE_H_
#define __BSCRIPT_VALUE_H_

#include <string>

class Object;

struct Value
{
    union
    {
        Object* object;
    } v;
};

#endif
