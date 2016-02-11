/* * b0rk - The b0rk Embeddable Runtime Environment
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

#ifndef __B0RK_SYSTEM_LANG_ARRAY_H_
#define __B0RK_SYSTEM_LANG_ARRAY_H_

#include <b0rk/class.h>

#include <string>
#include <map>

namespace b0rk
{

// TODO: HACK HACK HACK THIS WORKS BUT ISN'T RIGHT!
struct ArrayContainer
{
    std::map<std::string, Value> array;
};

class Array : public Class
{
 private:

 public:
    Array();
    ~Array();

    bool constructor(Context* context, Object* instance, int argCount, Value* args, Value& result);

    static ArrayContainer* getContainer(Object* array);
    static bool load(Object* array, Value index, Value& value);
    static bool store(Object* array, Value index, Value value);
};

};

#endif
