#ifndef __BSCRIPT_ASSEMBLER_H_
#define __BSCRIPT_ASSEMBLER_H_

#include "expression.h"
#include "opcodes.h"

#include <stdint.h>

class Assembler
{
 private:
    std::vector<uint64_t> m_code;

    bool assembleExpression(Expression* expr);

 public:
    Assembler();
    ~Assembler();

    bool assemble(CodeBlock* block);
};

#endif
