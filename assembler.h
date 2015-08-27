#ifndef __BSCRIPT_ASSEMBLER_H_
#define __BSCRIPT_ASSEMBLER_H_

#include "expression.h"
#include "opcodes.h"

#include <stdint.h>

class Runtime;
class Function;

struct AssembledCode
{
    uint64_t* code;
    int size;
};

class Assembler
{
 private:
    Runtime* m_runtime;
    std::vector<uint64_t> m_code;

    bool assembleExpression(Expression* expr);
    bool assembleBlock(CodeBlock* block);

    Function* findFunction(Identifier id);

 public:
    Assembler(Runtime* runtime);
    ~Assembler();

    bool assemble(CodeBlock* block, AssembledCode& asmCode);

    
};

#endif
