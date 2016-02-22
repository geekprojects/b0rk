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

#ifndef __BSCRIPT_VALUE_H_
#define __BSCRIPT_VALUE_H_

#include <stdint.h>
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

    std::wstring toString();
};

};

#endif
