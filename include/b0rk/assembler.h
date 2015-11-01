#ifndef __BSCRIPT_ASSEMBLER_H_
#define __BSCRIPT_ASSEMBLER_H_

#include <b0rk/expression.h>
#include <b0rk/opcodes.h>

#include <stdint.h>

namespace b0rk
{

class Runtime;
class Function;
class ScriptFunction;
class Class;
class Context;
struct CodeBlock;
struct Expression;

struct AssembledCode
{
    ScriptFunction* function;
    uint64_t* code;
    int size;
    int localVars;
};

class Assembler
{
 private:
    Context* m_context;
    ScriptFunction* m_function;

    std::vector<uint64_t> m_code;

    void pushOperator(OpCode opcode, ValueType type);

    bool assemble(CodeBlock* block, AssembledCode& asmCode);
    bool assembleExpression(CodeBlock* block, Expression* expr, OperationExpression* reference = NULL);
    bool assembleReference(CodeBlock* block, OperationExpression* expr);
    bool assembleBlock(CodeBlock* block);

    bool isVariable(CodeBlock* block, Object* context, std::string name);
    bool load(CodeBlock* block, VarExpression* var, OperationExpression* reference);
    bool store(CodeBlock* block, Object* context, std::string name);

    Function* findFunction(CodeBlock* block, Identifier id);

 public:
    Assembler(Context* context);
    ~Assembler();

    bool assemble(ScriptFunction* func, AssembledCode& asmCode);
};

};

#endif
