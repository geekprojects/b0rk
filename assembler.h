#ifndef __BSCRIPT_ASSEMBLER_H_
#define __BSCRIPT_ASSEMBLER_H_

#include "expression.h"
#include "opcodes.h"

#include <stdint.h>

class Runtime;
class Function;
class ScriptFunction;
class Class;
struct CodeBlock;
struct Expression;

struct AssembledCode
{
    uint64_t* code;
    int size;
    int localVars;
};

class Assembler
{
 private:
    Runtime* m_runtime;
    ScriptFunction* m_function;

    std::vector<uint64_t> m_code;

    bool assemble(CodeBlock* block, AssembledCode& asmCode);
    bool assembleExpression(CodeBlock* block, Expression* expr, Expression* reference = NULL);
    bool assembleReference(CodeBlock* block, OperationExpression* expr);
    bool assembleBlock(CodeBlock* block);

    bool isVariable(CodeBlock* block, Object* context, std::string name);
    bool load(CodeBlock* block, Object* context, VarExpression* var);
    bool store(CodeBlock* block, Object* context, std::string name);

    Function* findFunction(CodeBlock* block, Identifier id);

 public:
    Assembler(Runtime* runtime);
    ~Assembler();

    bool assemble(ScriptFunction* func, AssembledCode& asmCode);
};

#endif
