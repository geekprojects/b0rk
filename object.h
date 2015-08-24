#ifndef __BSCRIPT_OBJECT_H_
#define __BSCRIPT_OBJECT_H_

#include "class.h"
#include "value.h"

#include <map>

class Class;

class Object
{
 private:
    Class* m_class;

 public:
    Object(Class* clazz);
    ~Object();

    Class* getClass() { return m_class; }

void* data;
};

#endif
