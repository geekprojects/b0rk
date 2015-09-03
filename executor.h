#ifndef __BSCRIPT_EXECUTOR_H_
#define __BSCRIPT_EXECUTOR_H_

#include "assembler.h"
#include "context.h"

class Executor
{
 private:

 public:
    Executor();

    bool run(Context* context, AssembledCode& code, int argCount);
};

#endif
