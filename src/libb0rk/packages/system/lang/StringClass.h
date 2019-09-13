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

#ifndef __BSCRIPT_STRING_H_
#define __BSCRIPT_STRING_H_

#include <string>
#include <b0rk/class.h>
#include <b0rk/object.h>

namespace b0rk
{

class Context;

class StringNative : public NativeObject
{
 private:
    std::wstring m_string;

 public:
    StringNative(Object* object, std::string str);
    StringNative(Object* object, std::wstring str);

    bool addOperator(Context* context, int argCount, Value* args, Value& result);
    bool eqOperator(Context* context, int argCount, Value* args, Value& result);
    bool length(Context* context, int argCount, Value* args, Value& result);
    bool at(Context* context, int argCount, Value* args, Value& result);

    std::wstring getString() const { return m_string; }
};

class String : public Class
{
 private:

 public:
    String();
    ~String();

    bool constructor(Context* context, Object* instance, int argCount, Value* args, Value& result);

    static Object* createString(Context* context, std::string str);
    static Object* createString(Context* context, std::wstring str);
    static std::wstring getString(Context* context, Value& value);
    static std::wstring getString(Context* context, Object* obj);
};

};

#endif
