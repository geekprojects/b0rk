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

#ifndef __BSCRIPT_FUNCTION_H_
#define __BSCRIPT_FUNCTION_H_

#include <b0rk/class.h>
#include <b0rk/context.h>
#include <b0rk/expression.h>
#include <b0rk/assembler.h>

#include <vector>

namespace b0rk
{

class Context;
class Class;
class NativeObject;
struct Object;

class Function
{
 protected:
    std::wstring m_name;
    bool m_static;
    Class* m_class;
    std::vector<std::wstring> m_args;

 public:
    Function(Class* clazz);
    Function(Class* clazz, std::vector<std::wstring> args);
    virtual ~Function();

    Class* getClass() { return m_class; }
    void setName(std::wstring name) { m_name = name; }
    std::wstring getFullName();

    void setStatic(bool isStatic) { m_static = isStatic; }
    bool getStatic() { return m_static; }

    int getArgId(std::wstring arg);

    bool execute(Context* context, Object* clazz, int argCount, Value* argv, Value& result);
    virtual bool execute(Context* context, Object* clazz, int argCount);
};

typedef bool(Class::*nativeFunction_t)(Context*, Object* instance, int argCount, Value* args, Value& result);

class NativeFunction : public Function
{
 private:
    nativeFunction_t m_native;

 public:
    NativeFunction(Class* clazz, nativeFunction_t func, bool isStatic = false);

    virtual bool execute(Context* context, Object* instance, int argCount);
};

typedef bool(NativeObject::*nativeObjectFunction_t)(Context*, int argCount, Value* args, Value& result);

class NativeObjectFunction : public Function
{
 private:
    nativeObjectFunction_t m_native;

 public:
    NativeObjectFunction(Class* clazz, nativeObjectFunction_t func);

    virtual bool execute(Context* context, Object* instance, int argCount);
};


class ScriptFunction : public Function
{
 private:
    CodeBlock* m_code;

    bool m_assembled;
    AssembledCode m_asmCode;

 public:
    ScriptFunction(Class* clazz, std::vector<std::wstring> args);
    ScriptFunction(Class* clazz, CodeBlock* code, std::vector<std::wstring> args);
    ~ScriptFunction();

    void setCode(CodeBlock* code);
    CodeBlock* getCode() { return m_code; }


    virtual bool execute(Context* context, Object* instance, int argCount);
};

};

#endif
