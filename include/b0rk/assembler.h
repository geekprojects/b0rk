/*
 * b0rk - The b0rk Embeddable Runtime Environment
 * Copyright (C) 2015, 2016 GeekProjects.com
 *
 * This file is part of b0rk.
 *
 * b0rk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * b0rk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */

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
class Disassembler;
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
    Disassembler* m_disassembler;

    std::vector<uint64_t> m_code;

    void pushOperator(OpCode opcode, ValueType type);
    void pushCMP(ValueType type);

    bool assemble(CodeBlock* block, AssembledCode& asmCode);
    bool assembleExpression(CodeBlock* block, Expression* expr, OperationExpression* reference = NULL, bool needResult = false);
    bool assembleTest(CodeBlock* block, Expression* testExpr);
    bool assembleReference(CodeBlock* block, OperationExpression* expr);
    bool assembleBlock(CodeBlock* block);

    bool isVariable(CodeBlock* block, Object* context, std::string name);
    bool load(CodeBlock* block, VarExpression* var, OperationExpression* reference);
    bool store(CodeBlock* block, VarExpression* var, OperationExpression* reference);

    Function* findFunction(CodeBlock* block, Identifier id);

 public:
    Assembler(Context* context);
    ~Assembler();

    bool assemble(ScriptFunction* func, AssembledCode& asmCode);
};

};

#endif
