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

