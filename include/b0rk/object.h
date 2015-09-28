#ifndef __BSCRIPT_OBJECT_H_
#define __BSCRIPT_OBJECT_H_

#include <b0rk/class.h>
#include <b0rk/value.h>

#include <map>

class Class;

struct Object
{
    Class* m_class;
    size_t m_size;
    uint64_t m_gcMark;
    Value m_values[0];

    Class* getClass() { return m_class; }
    Value getValue(int slot) { return m_values[slot]; }
    void setValue(int slot, Value v) { m_values[slot] = v; }
};

#endif
