#ifndef __BSCRIPT_CONTEXT_H_
#define __BSCRIPT_CONTEXT_H_

#include "runtime.h"
#include "value.h"

#include <vector>

class Runtime;

struct Frame
{
    Value* variables;
};

class Context
{
 private:
    Runtime* m_runtime;

    std::vector<Value> m_stack;

 public:
    Context(Runtime* runtime);
    ~Context();

    Runtime* getRuntime() { return m_runtime; }

    void push(Value value);
    Value pop();
};

#endif
