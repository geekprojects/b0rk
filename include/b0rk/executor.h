#ifndef __BSCRIPT_EXECUTOR_H_
#define __BSCRIPT_EXECUTOR_H_

#include <b0rk/assembler.h>
#include <b0rk/context.h>

namespace b0rk
{

class Executor;

class Executor
{
 public:
    Executor();
    ~Executor();

    bool run(Context* context, Object* thisObj, AssembledCode* code, int argCount);
};


};

#endif
