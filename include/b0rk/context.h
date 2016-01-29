#ifndef __BSCRIPT_CONTEXT_H_
#define __BSCRIPT_CONTEXT_H_

#include <b0rk/runtime.h>
#include <b0rk/value.h>
#include <b0rk/compiler.h>
#include <b0rk/assembler.h>

#include <vector>

namespace b0rk
{

class Runtime;

struct Flags
{
    unsigned int zero:1;
    unsigned int sign:1;
    unsigned int overflow:1;
};

struct Frame
{
    AssembledCode* code;
    bool running;

    int pc;
    Flags flags;

    int localVarsCount;
    Value* localVars;

    inline uint64_t fetch()
    {
        return code->code[pc++];
    }
};

class Context
{
 private:
    Runtime* m_runtime;

    int m_stackPos;
    int m_stackSize;
    Value* m_stack;

 public:
    Context(Runtime* runtime);
    virtual ~Context();

    Runtime* getRuntime() { return m_runtime; }

    int getStackSize() { return m_stackSize; }
    Value* getStack() { return m_stack; }

    inline void push(Value value)
    {
#ifdef DEBUG_STACK
        printf("Context::push: %s%s\n", spaces(m_stack.size()).c_str(), value.toString().c_str());
#endif
        if (B0RK_UNLIKELY(m_stackPos >= m_stackSize))
        {
            fprintf(stderr, "Context::push: STACK OVERFLOW\n");
            return;
        }
        m_stack[m_stackPos++] = value;
    }

    inline void pushVoid()
    {
        Value voidValue;
        voidValue.type = VALUE_VOID;
        push(voidValue);
    }

    inline Value pop()
    {
        if (B0RK_UNLIKELY(m_stackPos <= 0))
        {
            Value voidValue;
            voidValue.type = VALUE_VOID;
            fprintf(stderr, "Context::push: STACK UNDERFLOW\n");
            return voidValue;
        }

        Value value = m_stack[--m_stackPos];
#ifdef DEBUG_STACK
        printf("Context::pop : %s%s\n", spaces(m_stack.size()).c_str(), value.toString().c_str());
#endif

        return value;
    }
};

};

#endif
