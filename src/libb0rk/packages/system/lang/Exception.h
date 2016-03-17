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

#ifndef __B0RK_SYSTEM_LANG_EXCEPTION_H_
#define __B0RK_SYSTEM_LANG_EXCEPTION_H_

#include <b0rk/class.h>

#include <string>
#include <map>

namespace b0rk
{

struct StackTrace
{
    std::vector<std::wstring> stacktrace;
};

class Exception : public Class
{
 private:

 public:
    Exception();
    ~Exception();

    bool printStackTraceB(Context* context, Object* instance, int argCount, Value* args, Value& result);

    static Object* createException(Context* context, std::string str);
    static Object* createException(Context* context, Value& value);
    static Object* getExceptionValue(Context* context, Object* e);
    static std::wstring getExceptionString(Context* context, Object* e);
};

};

#endif
