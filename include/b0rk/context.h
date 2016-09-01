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

#ifndef __BSCRIPT_CONTEXT_H_
#define __BSCRIPT_CONTEXT_H_

#include <b0rk/runtime.h>
#include <b0rk/value.h>
#include <b0rk/compiler.h>
#include <b0rk/assembler.h>

#include <stdio.h>

#include <vector>

#define B0RK_STACK_MARGIN 4

namespace b0rk
{

class Runtime;

struct Flags
{
    unsigned int zero:1;
    unsigned int sign:1;
    unsigned int overflow:1;
};

struct ExceptionHandler
{
    uint64_t excepVar;
    uint64_t handlerPC;
};

struct Frame
{
    AssembledCode* code;
    bool running;

    int pc;
    Flags flags;

    int localVarsCount;
    Value* localVars;

    std::vector<ExceptionHandler> handlerStack;

    inline uint64_t fetch()
    {
        return code->code[pc++];
    }
};

class Context
{
 private:
    Runtime* m_runtime;
    Assembler m_assembler;

    int m_stackPos;
    int m_stackSize;
    Value* m_stack;

    bool m_exception;
    Value m_exceptionValue;

    Value m_null;

 public:
    Context(Runtime* runtime);
    virtual ~Context();

    Runtime* getRuntime() { return m_runtime; }
    Assembler& getAssembler() { return m_assembler; }

    int getStackSize() { return m_stackSize; }
    int getStackPos() { return m_stackPos; }
    Value* getStack() { return m_stack; }

    inline bool checkStack() const
    {
        return B0RK_LIKELY(m_stackPos >= 0 && m_stackPos < m_stackSize);
    }

    inline void push(Value& value)
    {
#ifdef DEBUG_STACK
        printf("Context::push: %s%s\n", spaces(m_stack.size()).c_str(), value.toString().c_str());
#endif
        m_stack[m_stackPos++] = value;
    }

    inline void pushVoid()
    {
        m_stack[m_stackPos++].type = VALUE_VOID;
    }

    inline void pushInt(int64_t i)
    {
        m_stack[m_stackPos].type = VALUE_INTEGER;
        m_stack[m_stackPos].i = i;
        m_stackPos++;
    }

    inline void pushDouble(double d)
    {
        m_stack[m_stackPos].type = VALUE_DOUBLE;
        m_stack[m_stackPos].d = d;
        m_stackPos++;
    }

    inline Value pop()
    {
        Value value = m_stack[--m_stackPos];

#ifdef DEBUG_STACK
        printf("Context::pop : %s%s\n", spaces(m_stack.size()).c_str(), value.toString().c_str());
#endif

        return value;
    }

    inline int64_t popInt()
    {
        return m_stack[--m_stackPos].i;
    }

    inline double popDouble()
    {
        return m_stack[--m_stackPos].d;
    }

    inline Value peek() const
    {
        return m_stack[m_stackPos - 1];
    }

    void throwException(Value exception);
    void throwException(Object* exception);
    void clearException() { m_exception = false; }
    bool hasException() { return m_exception; }
    Value& getExceptionValue() { return m_exceptionValue; }

    std::vector<std::wstring> getStackTrace();
};

};

#endif
