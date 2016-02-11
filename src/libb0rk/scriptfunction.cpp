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


#include <stdio.h>
#include <string.h>
#include <math.h>

#include <b0rk/function.h>
#include <b0rk/executor.h>

using namespace std;
using namespace b0rk;

ScriptFunction::ScriptFunction(Class* clazz, vector<string> args)
    : Function(clazz, args)
{
    m_code = NULL;
    m_assembled = false;
}

ScriptFunction::ScriptFunction(Class* clazz, CodeBlock* code, vector<string> args)
    : Function(clazz, args)
{
    m_code = code;
}

ScriptFunction::~ScriptFunction()
{
    delete m_code;

    if (m_assembled && m_asmCode.code != NULL)
    {
        delete m_asmCode.code;
    }
}

void ScriptFunction::setCode(CodeBlock* code)
{
    m_code = code;
}

bool ScriptFunction::execute(Context* context, Object* instance, int argCount)
{
    if (m_assembled == false)
    {
        bool res = context->getAssembler().assemble(this, m_asmCode);
        if (!res)
        {
            return false;
        }
        m_assembled = true;
    }

    Executor* executor = context->getRuntime()->getExecutor();

    return executor->run(context, instance, &m_asmCode, argCount);
}

