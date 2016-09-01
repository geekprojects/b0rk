/*
 * b0rk - The b0rk Embeddable Runtime Environment
 * Copyright (C) 2015, 2016 GeekProjects.com
 *
 * This file is part of b0rk.
 *
 * b0rk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * b0rk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BSCRIPT_OBJECT_H_
#define __BSCRIPT_OBJECT_H_

#include <b0rk/class.h>
#include <b0rk/value.h>

#include <stdio.h>
#include <stdint.h>

#include <map>

#define B0RK_GC_EXTERNAL ((uint64_t)-1ll)

namespace b0rk
{

class Class;

class NativeObject
{
 protected:
    Object* m_object;

 public:
    NativeObject(Object* object) { m_object = object; }
    virtual ~NativeObject() {}

    Object* getB0rkObject() { return m_object; }
};

struct Object
{
    Class* m_class;
    size_t m_size;
    uint64_t m_gcMark;
    Value m_values[0];

    inline Class* getClass() const { return m_class; }
    Value getValue(int slot) const { return m_values[slot]; }
    void setValue(int slot, Value v) { m_values[slot] = v; }

    void setNativeObject(Class* clazz, NativeObject* object)
    {
        int nativeField = clazz->getFieldCount() - 1;
        m_values[nativeField].pointer = object;
    }

    NativeObject* getNativeObject(Class* clazz)
    {
        int nativeField = clazz->getFieldCount() - 1;
        NativeObject* no = (NativeObject*)(m_values[nativeField].pointer);
        return no;
    }

    void setExternalGC() { m_gcMark = B0RK_GC_EXTERNAL; }
    void clearExternalGC() { m_gcMark = 0; }
};

};

#endif
