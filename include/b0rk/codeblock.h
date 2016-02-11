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

#ifndef __BSCRIPT_CODEBLOCK_H_
#define __BSCRIPT_CODEBLOCK_H_

#include <vector>
#include <string>

#include <b0rk/value.h>

namespace b0rk
{

class ScriptFunction;
struct Expression;

struct CodeBlock
{
    ScriptFunction* m_function;
    CodeBlock* m_parent;
    int m_startingVarId;
    int m_maxVarId;

    ValueType* m_varTypes;

    std::vector<std::string> m_vars;
    std::vector<Expression*> m_code;
    std::vector<CodeBlock*> m_childBlocks;

    CodeBlock();
    ~CodeBlock();

    int setStartingVarId(int id);
    int getVarId(std::string var);
    bool setVarType(int id, ValueType type);
    ValueType getVarType(int id);

    std::string toString();
};

};

#endif
