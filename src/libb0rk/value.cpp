/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <b0rk/value.h>
#include <b0rk/object.h>
#include <b0rk/utils.h>

#include <wchar.h>

using namespace std;
using namespace b0rk;

wstring Value::toString()
{
    wchar_t buffer[128];
    switch (type)
    {
        case VALUE_VARIABLE:
            swprintf(buffer, 128, L"{VARIABLE:%p}", variable);
            return wstring(buffer);

        case VALUE_OBJECT:
            if (object != NULL)
            {
                if (object->getClass() != NULL)
                {
                    swprintf(buffer, 128, L"{OBJECT:%ls:%p}", object->getClass()->getName().c_str(), object);
                }
                else
                {
                    swprintf(buffer, 128, L"{OBJECT:UNKNOWN CLASS?:%p}", object);
                }
            }
            else
            {
                swprintf(buffer, 128, L"{OBJECT:NULL}");
            }
            return wstring(buffer);

        case VALUE_POINTER:
            swprintf(buffer, 128, L"{POINTER:%p}", pointer);
            return wstring(buffer);

        case VALUE_INTEGER:
            swprintf(buffer, 128, L"%lld", i);
            return wstring(buffer);

        case VALUE_DOUBLE:
            swprintf(buffer, 128, L"%0.2f", d);
            return wstring(buffer);

        case VALUE_FRAME:
            swprintf(buffer, 128, L"{FRAME:%p}", pointer);
            return wstring(buffer);

        case VALUE_VOID:
            return L"{VOID}";

        case VALUE_UNKNOWN:
        default:
            return L"{UNKNOWN}";
    }
}

