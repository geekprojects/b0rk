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

#include <cinttypes>

using namespace std;
using namespace b0rk;

void int642wstr(int64_t v, wchar_t *s, int base)
{
    if (v == 0)
    {
        s[0] = '0';
        s[1] = '\0';
    }
    else
    {
        wchar_t s1[80];
        int64_t v1;
        int i = 0, j = 0;
        if (v < 0)
        {
            s[j++] = '-';
            v = -v;
        }

        while (v > 0)
        {
            v1 = v % base;
            if (v1 < 10)
            {
                s1[i++] = v1 + '0';
            }
            else
            {
                s1[i++] = v1 - 10 + 'a';
            }
            v /= base;
        }
        while (i > 0)
        {
            s[j++] = s1[--i];
        }
        s[j] = '\0';
    }
}

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
            int642wstr(i, buffer, 10);
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

