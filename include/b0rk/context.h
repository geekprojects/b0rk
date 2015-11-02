#ifndef __BSCRIPT_CONTEXT_H_
#define __BSCRIPT_CONTEXT_H_

#include <b0rk/runtime.h>
#include <b0rk/value.h>

#include <vector>

namespace b0rk
{

class Runtime;

struct Frame
{
    int localVarsCount;
    Value* localVars;
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
    ~Context();

    Runtime* getRuntime() { return m_runtime; }

    int getStackSize() { return m_stackSize; }
    Value* getStack() { return m_stack; }

    void push(Value value);
    void pushVoid();
    Value pop();
};

};

#endif
