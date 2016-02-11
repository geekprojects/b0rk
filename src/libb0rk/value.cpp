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

using namespace std;
using namespace b0rk;

string Value::toString()
{
    char buffer[128];
    switch (type)
    {
        case VALUE_VARIABLE:
            snprintf(buffer, 128, "{VARIABLE:%p}", variable);
            return string(buffer);

        case VALUE_OBJECT:
            if (object != NULL)
            {
                if (object->getClass() != NULL)
                {
                    snprintf(buffer, 128, "{OBJECT:%s:%p}", object->getClass()->getName().c_str(), object);
                }
                else
                {
                    snprintf(buffer, 128, "{OBJECT:UNKNOWN CLASS?:%p}", object);
                }
            }
            else
            {
                snprintf(buffer, 128, "{OBJECT:NULL}");
            }
            return string(buffer);

        case VALUE_POINTER:
            snprintf(buffer, 128, "{POINTER:%p}", pointer);
            return string(buffer);

        case VALUE_INTEGER:
            snprintf(buffer, 128, "%lld", i);
            return string(buffer);

        case VALUE_DOUBLE:
            snprintf(buffer, 128, "%0.2f", d);
            return string(buffer);

        case VALUE_FRAME:
            snprintf(buffer, 128, "{FRAME:%p}", pointer);
            return string(buffer);

        case VALUE_VOID:
            return string("{VOID}");

        case VALUE_UNKNOWN:
        default:
            return string("{UNKNOWN}");
    }
}

