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

    Value* m_values;
    int m_valueCount;

 public:
    Object(Class* clazz);
    ~Object();

    Class* getClass() { return m_class; }

    int getValueCount() { return m_valueCount; }
    Value getValue(int slot) { return m_values[slot]; }
    void setValue(int slot, Value v) { m_values[slot] = v; }

};

#endif
