#ifndef __BSCRIPT_EXECUTOR_H_
#define __BSCRIPT_EXECUTOR_H_

#include <b0rk/assembler.h>
#include <b0rk/context.h>

namespace b0rk
{

class Executor;

typedef bool(*OpFunc)(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame);

class Executor
{
 private:
    OpFunc m_operations[OPCODE_MAX];

 public:
    Executor();
    ~Executor();

    bool run(Context* context, Object* thisObj, AssembledCode* code, int argCount);
};


};

#endif
