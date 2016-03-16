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

#include <b0rk/context.h>
#include <b0rk/compiler.h>

using namespace std;
using namespace b0rk;

Context::Context(Runtime* runtime)
    : m_assembler(this)
{
    m_runtime = runtime;

    m_stackPos = 0;
    m_stackSize = 1024;
    m_stack = new Value[m_stackSize];

    m_exception = false;
}

Context::~Context()
{
    delete[] m_stack;
}

void Context::throwException(Object* exception)
{
    Value exceptionValue;
    exceptionValue.type = VALUE_OBJECT;
    exceptionValue.object = exception;
    throwException(exceptionValue);
}

void Context::throwException(Value exception)
{
    m_exception = true;
    m_exceptionValue = exception;
}

vector<wstring> Context::getStackTrace()
{
    vector<wstring> stacktrace;

    int pos;
    for (pos = m_stackPos - 1; pos >= 0; pos--)
    {
        if (m_stack[pos].type == VALUE_FRAME)
        {
            wstring entry;
            Frame* frame = (Frame*)m_stack[pos].pointer;
            if (frame != NULL)
            {
                entry = frame->code->function->getFullName();
            }
            else
            {
                entry = L"<unknown>";
            }
#if 0
            printf("Context::getStackTrace: %d: %ls\n", pos, entry.c_str());
#endif
            stacktrace.push_back(entry);
        }
    }

    return stacktrace;
}


string spaces(int c)
{
    string str = "";
    int i;
    for (i = 0; i < c; i++)
    {
        str += "=";
    }
    return str;
}

