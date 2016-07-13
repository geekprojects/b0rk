/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#undef DEBUG_ASSEMBLER
#define DEBUG_OPTIMISER

#include <b0rk/assembler.h>
#include <b0rk/runtime.h>
#include <b0rk/function.h>
#include <b0rk/disassembler.h>
#include <b0rk/utils.h>

#include "packages/system/lang/StringClass.h"

#include <stdarg.h>

#include <set>

using namespace std;
using namespace b0rk;

Assembler::Assembler(Context* context)
{
    m_context = context;

    OpCodeInfo::init();

    // TODO: Config
    m_disassembler = new Disassembler();
}

Assembler::~Assembler()
{
}

bool Assembler::assemble(ScriptFunction* function, AssembledCode& asmCode)
{

    m_function = function;
    asmCode.function = function;
    return assemble(function->getCode(), asmCode);
}

bool Assembler::assemble(CodeBlock* code, AssembledCode& asmCode)
{
    unsigned int i;
    bool res;

    res = assembleBlock(code);
    if (!res)
    {
        return false;
    }

    pushInstruction(OPCODE_PUSHI, 0);
    pushInstruction(OPCODE_RETURN);

    asmCode.localVars = code->m_maxVarId + 1;

    optimise(asmCode);

    asmCode.size = 0;
    vector<Instruction>::iterator it;
    asmCode.size = 0;
    for (it = m_code.begin(); it != m_code.end(); it++)
    {
        (*it).address = asmCode.size;
        asmCode.size++;
        asmCode.size += (*it).args.size();
    }

    asmCode.code = new uint64_t[asmCode.size];

    i = 0;
    for (it = m_code.begin(); it != m_code.end(); it++)
    {
        Instruction inst = *it;
        OpCodeInfo opCodeInfo = OpCodeInfo::getOpcodeInfo(inst.op);
        asmCode.code[i++] = inst.op;

        if (opCodeInfo.args != inst.args.size())
        {
            printf(
                "Assembler::assemble: ERROR: Wrong number of arguments for op 0x%x, got %lu expected %d\n",
                (int)inst.op,
                inst.args.size(),
                opCodeInfo.args);
            return false;
        }

        int j;
        for (j = 0; j < opCodeInfo.args; j++)
        {
            uint64_t arg = inst.args[j];
            if (j == opCodeInfo.address)
            {
                if (arg >= m_code.size())
                {
                    printf(
                        "Assembler::assemble: ERROR: Instruction address is out of range!\n");
                    return false;
                }

                // Map an instruction address to it's assembled code address
                arg = m_code[arg].address;
            }
            asmCode.code[i++] = arg;
        }
    }

    m_disassembler->disassemble(asmCode);

    m_code.clear();

    return true;
}

#define IS_REFERENCE(_expr) ((_expr) != NULL && (_expr)->type == EXPR_OPER && ((OperationExpression*)(_expr))->operType == OP_REFERENCE)
bool Assembler::assembleBlock(CodeBlock* code)
{
   unsigned int i;
    for (i = 0; i < code->m_code.size(); i++)
    {
        Expression* expr = code->m_code[i];

#ifdef DEBUG_ASSEMBLER
        printf("Assembler::assembleBlock: expr: %ls\n", expr->toString().c_str());
#endif

        bool res;
        res = assembleExpression(code, expr, NULL, false);
        if (!res)
        {
            printf("Assembler::assembleBlock: ERROR: Failed to assemble expr: %ls\n", expr->toString().c_str());
            return false;
        }

        if (IS_REFERENCE(expr))
        {
            Expression* pos = expr;
            while (IS_REFERENCE(pos))
            {
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleBlock: Checking reference: %ls\n", pos->toString().c_str());
#endif
                pos = ((OperationExpression*)pos)->right;
            }
            if (pos != NULL && pos->type == EXPR_CALL)
            {
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleBlock:  -> reference to a call!!\n");
#endif
            }
        }
    }
    return true;
}

void Assembler::pushOperator(OpCode opcode, ValueType type)
{
    switch (type)
    {
        case VALUE_INTEGER:
            pushInstruction((OpCode)((int)opcode + 0x10));
            break;
        case VALUE_DOUBLE:
            pushInstruction((OpCode)((int)opcode + 0x20));
            break;
        default:
            pushInstruction(opcode);
            break;
    }
}

void Assembler::pushCMP(ValueType type)
{
    switch (type)
    {
        case VALUE_INTEGER:
            pushInstruction(OPCODE_CMPI);
            break;
        case VALUE_DOUBLE:
            pushInstruction(OPCODE_CMPD);
            break;
        default:
            pushInstruction(OPCODE_CMP);
            break;
    }
}


/*
 * An expression should always leave exactly one item on the stack
 * after execution
 */
bool Assembler::assembleExpression(CodeBlock* block, Expression* expr, OperationExpression* reference, bool needResult)
{
    bool res;

    switch (expr->type)
    {
        case EXPR_CALL:
        {
            CallExpression* callExpr = (CallExpression*)expr;
            std::vector<Expression*>::iterator it;
            int count = 0;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: CALL: Arguments...\n");
#endif
            for (it = callExpr->parameters.begin(); it != callExpr->parameters.end(); it++)
            {
                Expression* param = *it;
                res = assembleExpression(block, param, NULL, true);
                if (!res)
                {
                    printf("Assembler::assembleExpression: ERROR: Failed to assemble expr: %ls\n", expr->toString().c_str());
                    return false;
                }
                count++;
            }
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: CALL: %ls (class=%p)\n", callExpr->function.c_str(), callExpr->clazz);
#endif

            if (reference != NULL)
            {
                res = assembleReference(block, reference);
                if (!res)
                {
                    printf("Assembler::assembleExpression: ERROR: Failed to reference expr: %ls\n", expr->toString().c_str());
                    return false;
                }
            }

            wstring id = callExpr->function;

            if (callExpr->clazz != NULL)
            {
                // Static class method!
                Function* func = callExpr->clazz->findMethod(id);
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleExpression: CALL: Static call: %ls.%ls\n", callExpr->clazz->getName().c_str(), id.c_str());
#endif
                pushInstruction(OPCODE_CALL_STATIC, (uint64_t)func, count);
            }
            else
            {

#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleExpression: CALL: Named call?\n");
#endif
                bool inReference = false;
                if (callExpr->parent != NULL)
                {
                    if (callExpr->parent->type == EXPR_OPER &&
                        ((OperationExpression*)(callExpr->parent))->operType == OP_REFERENCE)
                    {
#ifdef DEBUG_ASSEMBLER
                        printf("Assembler::assembleExpression: CALL: Parent is a reference\n");
#endif
                        inReference = true;
                    }
                    else
                    {
#ifdef DEBUG_ASSEMBLER
                        printf("Assembler::assembleExpression: CALL: Parent is not null but not a reference!?\n");
                        printf("Assembler::assembleExpression: CALL: parent=%p\n", callExpr->parent);
                        printf("Assembler::assembleExpression: CALL: parent=%ls\n", callExpr->parent->toString().c_str());
#endif
                    }
                }

                if (inReference)
                {
                    Object* strObj = m_context->getRuntime()->newString(m_context, callExpr->function);
                    strObj->setExternalGC();

                    pushInstruction(OPCODE_PUSHOBJ, (uint64_t)strObj);
                    pushInstruction(OPCODE_CALL_NAMED, count);
                }
                else
                {
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: CALL: Not a reference, must be THIS\n");
#endif

                    bool isVar = isVariable(block, NULL, callExpr->function);
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: CALL: %ls: isVar=%d\n", callExpr->function.c_str(), isVar);
#endif
                    if (isVar)
                    {
                        VarExpression varExpr(block);
                        varExpr.clazz = NULL;
                        varExpr.var = callExpr->function;
                        load(block, &varExpr, reference);
                        pushInstruction(OPCODE_CALL_OBJ, count);
                    }
                    else
                    {
                        // Try class names?
                        Function* func = m_function->getClass()->findMethod(callExpr->function);
                        if (func == NULL)
                        {
                            printf("Assembler::assembleExpression: CALL: Function on this not found\n");
                            return false;
                        }

                        if (func->getStatic())
                        {
                            pushInstruction(OPCODE_CALL_STATIC, (uint64_t)func, count);
                        }
                        else
                        {
                            pushInstruction(OPCODE_LOAD_VAR, 0); // Load "this"

                            pushInstruction(OPCODE_CALL, (uint64_t)func, count);
                        }
                    }
                }
            }
            if (!needResult)
            {
                pushInstruction(OPCODE_POP);
                expr->resultOnStack = false;
            }
        } break;

        case EXPR_OPER:
        {
            OperationExpression* opExpr = (OperationExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: OPER: operType=%d\n", opExpr->operType);
#endif

            if (opExpr->operType == OP_REFERENCE && reference == NULL)
            {

                Expression* refChild = expr;
                while (IS_REFERENCE(refChild))
                {
                    refChild = ((OperationExpression*)refChild)->right;
                }

                if (refChild == NULL)
                {
                    printf("Assembler::assembleExpression: OPER: Reference isn't used!?\n");
                    return false;
                }
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleExpression: OPER: Reference: Assembling nearest non-ref child: %ls\n", refChild->toString().c_str());
#endif
                return assembleExpression(block, refChild, opExpr, needResult);
            }

            if (opExpr->right != NULL)
            {
                res = assembleExpression(block, opExpr->right, NULL, true);
                if (!res)
                {
                    return false;
                }
            }

            if (opExpr->operType != OP_SET &&
                opExpr->operType != OP_INCREMENT &&
                opExpr->operType != OP_DECREMENT &&
                opExpr->operDesc.hasLeftExpr)
            {
                res = assembleExpression(block, opExpr->left, NULL, true);
                if (!res)
                {
                    return false;
                }
            }

            switch (opExpr->operType)
            {
                case OP_ADD:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: ADD: type=%d\n", opExpr->operType);
#endif
                    pushOperator(OPCODE_ADD, opExpr->valueType);
                    break;

                case OP_SUB:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: SUB\n");
#endif
                    pushOperator(OPCODE_SUB, opExpr->valueType);
                    break;

                case OP_MULTIPLY:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: MULTIPLY\n");
#endif
                    pushOperator(OPCODE_MUL, opExpr->valueType);
                    break;

                case OP_DIVIDE:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: DIVIDE\n");
#endif
                    pushOperator(OPCODE_DIV, opExpr->valueType);
                    break;

                case OP_NOT:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: NOT\n");
#endif
                    pushOperator(OPCODE_NOT, opExpr->valueType);
                    break;

                case OP_LOGICAL_AND:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: LOGICAL_AND (type=%d)\n", opExpr->valueType);
#endif
                    pushOperator(OPCODE_AND, opExpr->valueType);
                    break;

                case OP_EQUALS:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: EQUALS\n");
#endif
                    pushCMP(opExpr->valueType);
                    pushInstruction(OPCODE_PUSHCE);
                    break;

                case OP_NOT_EQUAL:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: NOT_EQUAL\n");
#endif
                    pushCMP(opExpr->valueType);
                    pushInstruction(OPCODE_PUSHCNE);
                    break;


                case OP_LESS_THAN:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: LESS_THAN\n");
#endif
                    pushCMP(opExpr->valueType);
                    pushInstruction(OPCODE_PUSHCL);
                    break;

                case OP_LESS_THAN_EQUAL:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: LESS_THAN_EQUAL\n");
#endif
                    pushCMP(opExpr->valueType);
                    pushInstruction(OPCODE_PUSHCLE);
                    break;


                case OP_GREATER_THAN:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: GREATER_THAN\n");
#endif
                    pushCMP(opExpr->valueType);
                    pushInstruction(OPCODE_PUSHCG);
                    break;

                case OP_GREATER_THAN_EQUAL:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: GREATER_THAN\n");
#endif
                    pushCMP(opExpr->valueType);
                    pushInstruction(OPCODE_PUSHCGE);
                    break;

                case OP_INCREMENT:
                case OP_DECREMENT:
                {
                    if (opExpr->left->type != EXPR_VAR && opExpr->left->type != EXPR_ARRAY)
                    {
                        printf("Assembler::assembleExpression: OPER: xCREMENT: Error: Left must be a variable or array!\n");
                        return false;
                    }


#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: POST xCREMENT %d", 0);
#endif
                    VarExpression* varExpr = (VarExpression*)(opExpr->left);

                    int id = block->getVarId(varExpr->var);
                    if (!needResult && id != -1)
                    {
#ifdef DEBUG_ASSEMBLER
                        printf("Assembler::assembleExpression: OPER: POST xCREMENT: Use INC_VAR!\n");
#endif

                        uint64_t inc;
                        if (opExpr->operType == OP_INCREMENT)
                        {
                            inc = 1;
                        }
                        else
                        {
                            inc = -1;
                        }
                        pushInstruction(OPCODE_INC_VAR, id, inc);
                        expr->resultOnStack = false;
                    }
                    else
                    {
                        res = load(block, varExpr, reference);
                        if (!res)
                        {
                            return false;
                        }

                        if (needResult)
                        {
                            pushInstruction(OPCODE_DUP);
                            pushInstruction(OPCODE_PUSHI, 1);
                            if (opExpr->operType == OP_DECREMENT)
                            {
                                // Order matters!
                                pushInstruction(OPCODE_SWAP);
                            }
                        }
                        else
                        {
                            pushInstruction(OPCODE_PUSHI, 1);
                        }

                        expr->resultOnStack = needResult;

                        if (opExpr->operType == OP_INCREMENT)
                        {
                            pushInstruction(OPCODE_ADDI);
                        }
                        else
                        {
                            pushInstruction(OPCODE_SUBI);
                        }
                        res = store(block, varExpr, reference);
                        if (!res)
                        {
                            return false;
                        }
                    }
                } break;

                case OP_SET:
                {

#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: SET\n");
                    printf("Assembler::assembleExpression: OPER: -> left=%p\n", opExpr->left);
                    printf("Assembler::assembleExpression: OPER: -> left=%ls\n", opExpr->left->toString().c_str());
#endif

                    if (needResult)
                    {
                        pushInstruction(OPCODE_DUP);
                    }

                    expr->resultOnStack = needResult;

                    ExpressionType type = opExpr->left->type;
                    Expression* dest = opExpr->left;
                    if (IS_REFERENCE(dest))
                    {
#ifdef DEBUG_ASSEMBLER
                        printf("Assembler::assembleExpression: OPER: -> Dest is reference\n");
#endif
                        reference = (OperationExpression*)dest;
                        type = EXPR_VAR;
                        dest = opExpr->left;
                        while (IS_REFERENCE(dest))
                        {
                            dest = ((OperationExpression*)dest)->right;
                        }
#ifdef DEBUG_ASSEMBLER
                        printf("Assembler::assembleExpression: OPER: -> first non reference=%ls\n", dest->toString().c_str());
#endif
                    }

                    if (type == EXPR_VAR)
                    {
                        VarExpression* varExpr = (VarExpression*)dest;
                        res = store(block, varExpr, reference);
                        if (!res)
                        {
                            return false;
                        }
                    }
                    else if (opExpr->left->type == EXPR_ARRAY)
                    {
                        ArrayExpression* arrExpr = (ArrayExpression*)(opExpr->left);
#ifdef DEBUG_ASSEMBLER
                        printf("Assembler::assembleExpression: OPER: SET ARRAY!\n");
#endif
                        // Load the array object
                        res = load(block, arrExpr, reference);
                        if (!res)
                        {
                            return false;
                        }

                        res = assembleExpression(block, arrExpr->indexExpr, NULL, true);
                        if (!res)
                        {
                            return false;
                        }

                        pushInstruction(OPCODE_STORE_ARRAY);
                    }
                    else
                    {
                        printf("Assembler::assembleExpression: OPER: SET: Error: Left must be a variable or array! expr=%ls\n", opExpr->toString().c_str());
                        return false;
                    }
                } break;

                case OP_REFERENCE:
                {
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: Reference: Nothing to actually do?\n");
#endif
                } break;

               default:
                    printf("Assembler::assembleExpression: OPER: Unhandled operation\n");
                    return false;
            }
        } break;

        case EXPR_IF:
        {
            IfExpression* ifExpr = (IfExpression*)expr;

            // Add the condition test
            res = assembleTest(block, ifExpr->testExpr);
            int bneToFalsePos = m_code.size() - 1;

            // Add the body
            res = assembleBlock(ifExpr->trueBlock);
            if (!res)
            {
                return false;
            }

            int jmpToEndPos = -1;
            if (ifExpr->falseBlock != NULL)
            {
                // Jump to end
                pushInstruction(OPCODE_JMP, 0);
                jmpToEndPos = m_code.size() - 1;
            }

            m_code[bneToFalsePos].args[0] = m_code.size();
            if (ifExpr->falseBlock != NULL)
            {

                // Add the false body
                res = assembleBlock(ifExpr->falseBlock);
                if (!res)
                {
                    return false;
                }

                m_code[jmpToEndPos].args[0] = m_code.size();
            }
        } break;

        case EXPR_FOR:
        {
            ForExpression* forExpr = (ForExpression*)expr;

#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FOR: init...\n");
#endif
            // Add the init
            if (forExpr->initExpr != NULL)
            {
                res = assembleExpression(block, forExpr->initExpr, NULL, false);
                if (!res)
                {
                    return false;
                }
            }

#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FOR: test...\n");
#endif
            // Add the test
            int loopTop = m_code.size();
            res = assembleTest(block, forExpr->testExpr);
            int bneToEndPos = m_code.size() - 1;

#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FOR: body...\n");
#endif
            // Add the body
            res = assembleBlock(forExpr->body);
            if (!res)
            {
                return false;
            }

#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FOR: increment...\n");
#endif
            // Increment
            if (forExpr->incExpr != NULL)
            {
                res = assembleExpression(block, forExpr->incExpr, NULL, false);
                if (!res)
                {
                    return false;
                }
            }

#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FOR: jump...\n");
#endif
            // Jump back to the test above
            pushInstruction(OPCODE_JMP, loopTop);
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FOR: done!\n");
#endif

            m_code[bneToEndPos].args[0] = m_code.size();
        } break;

        case EXPR_RETURN:
        {
            ReturnExpression* retExpr = (ReturnExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: RETURN\n");
#endif
            if (retExpr->returnValue == NULL)
            {
                pushInstruction(OPCODE_PUSHI, 0);
            }
            else
            {
                res = assembleExpression(block, retExpr->returnValue, NULL, true);
                if (!res)
                {
                    return false;
                }
            }
            pushInstruction(OPCODE_RETURN);
        } break;

        case EXPR_TRY:
        {
            // Do or do not, there is no.. Oh wait.
            TryExpression* tryExpr = (TryExpression*)expr;

            int excepVar = tryExpr->catchBlock->getVarId(tryExpr->exceptionVar);
            if (excepVar == -1)
            {
                printf("Assembler::assembleExpression: TRY: Exception var %ls not found!\n", tryExpr->exceptionVar.c_str());
                return false;
            }

            // Tell the executor what to do if there's an exception
            pushInstruction(OPCODE_PUSHTRY, excepVar, 0); // Will be filled in with the end address
            unsigned int tryCatchPos = m_code.size() - 1;

            // Add the body
            res = assembleBlock(tryExpr->tryBlock);
            if (!res)
            {
                return false;
            }
            pushInstruction(OPCODE_POPTRY);

            // No exceptions! Jump beyond handler
            pushInstruction(OPCODE_JMP, 0); // Will be filled in with the end address
            unsigned int jmpToEndPos = m_code.size() - 1;
            unsigned int catchPos = m_code.size();
            m_code[tryCatchPos].args[1] = catchPos;

            res = assembleBlock(tryExpr->catchBlock);
            if (!res)
            {
                return false;
            }
            m_code[jmpToEndPos].args[0] = m_code.size();
        } break;

        case EXPR_THROW:
        {
            ThrowExpression* throwExpr = (ThrowExpression*)expr;

            res = assembleExpression(block, throwExpr->throwValue, NULL, true);
            if (!res)
            {
                return false;
            }
            pushInstruction(OPCODE_THROW);
        } break;

        case EXPR_VAR:
        {
            VarExpression* varExpr = (VarExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: VAR: var=%ls\n", varExpr->var.c_str());
#endif
            expr->resultOnStack = true;

            res = load(block, varExpr, reference);
            if (!res)
            {
                return false;
            }
        } break;

        case EXPR_ARRAY:
        {
            ArrayExpression* arrExpr = (ArrayExpression*)expr;

#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: ARRAY!\n");
#endif

            // Load the array object
            res = load(block, arrExpr, reference);
            if (!res)
            {
                return false;
            }

            res = assembleExpression(block, arrExpr->indexExpr, NULL, true);
            if (!res)
            {
                return false;
            }

            pushInstruction(OPCODE_LOAD_ARRAY);
        } break;

        case EXPR_NEW:
        {
            NewExpression* newExpr = (NewExpression*)expr;
            Class* clazz = m_context->getRuntime()->findClass(m_context, newExpr->clazz.toString());
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: NEW: %ls = %p\n", newExpr->clazz.toString().c_str(), clazz);
#endif

            if (clazz == NULL)
            {
                printf("Assembler::assembleExpression: Unknown class: %ls\n", newExpr->clazz.toString().c_str());
                return false;
            }

            int count = 0;
            std::vector<Expression*>::iterator it;
            for (it = newExpr->parameters.begin(); it != newExpr->parameters.end(); it++)
            {
                res = assembleExpression(block, *it, NULL, true);
                if (!res)
                {
                    return false;
                }
                count++;
            }

            pushInstruction(OPCODE_NEW, (uint64_t)clazz, count);
        } break;

        case EXPR_STRING:
        {
            StringExpression* strExpr = (StringExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: STRING: NEW STRING: %ls\n", strExpr->str.c_str());
#endif

            Object* strObj = m_context->getRuntime()->newString(m_context, strExpr->str);
            strObj->setExternalGC();
            pushInstruction(OPCODE_PUSHOBJ, (uint64_t)strObj);
        } break;

        case EXPR_INTEGER:
        {
            IntegerExpression* intExpr = (IntegerExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: INTEGER: PUSH %lld\n", intExpr->i);
#endif
            pushInstruction(OPCODE_PUSHI, intExpr->i);
        } break;

        case EXPR_DOUBLE:
        {
            DoubleExpression* doubleExpr = (DoubleExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: DOUBLE: PUSH %0.2f\n", doubleExpr->d);
#endif
            int64_t* dtoi = (int64_t*)(&(doubleExpr->d));
            pushInstruction(OPCODE_PUSHD, *dtoi);
        } break;

        case EXPR_FUNCTION:
        {
            FunctionExpression* funcExpr = (FunctionExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FUNCTION: NEW FUNCTION %p\n", funcExpr->function);
#endif
            pushInstruction(OPCODE_NEW_FUNCTION, (uint64_t)funcExpr->function);
        } break;

        default:
            printf("Assembler::assembleExpression: Unhandled expression: %d: %ls\n", expr->type, expr->toString().c_str());
            return false;
    }

#ifdef DEBUG_ASSEMBLER
    if (needResult && !expr->resultOnStack)
    {
        printf("Assembler::assembleExpression: ERROR: Result required, but none returned by expression!\n");
        printf("Assembler::assembleExpression: ERROR:  -> %ls\n", expr->toString().c_str());
    }
    else if (!needResult && expr->resultOnStack)
    {
        printf("Assembler::assembleExpression: ERROR: Result not required, but returned by expression!\n");
        printf("Assembler::assembleExpression: ERROR:  -> %ls\n", expr->toString().c_str());
    }
#endif

    return true;
}

bool Assembler::assembleTest(CodeBlock* block, Expression* testExpr)
{
    bool res = assembleExpression(block, testExpr, NULL, true);
    if (!res)
    {
        return false;
    }

    OpCode branchOpcode = OPCODE_MAX;
    if (m_code.size() > 2)
    {
        // Optimisation: Can we collapse the compare in the test expression and our own CMP?
        uint64_t last0 = m_code[m_code.size() - 2].op;

        if (last0 == OPCODE_CMP || last0 == OPCODE_CMPI || OPCODE_CMPD)
        {
            uint64_t last1 = m_code[m_code.size() - 1].op;

            // Note that we flip the condition as we want to branch if
            // the condition is false, i.e. skip the true block
            switch (last1)
            {
                case OPCODE_PUSHCE:
#ifdef DEBUG_ASSEMBLER
                    fprintf(stderr, "Assembler::assembleTest: Test was CMP/PUSHCE!\n");
#endif
                    m_code.pop_back();
                    branchOpcode = OPCODE_BNE;
                    break;
                case OPCODE_PUSHCL:
#ifdef DEBUG_ASSEMBLER
                    fprintf(stderr, "Assembler::assembleTest: Test was CMP/PUSHCL!\n");
#endif
                    m_code.pop_back();
                    branchOpcode = OPCODE_BGE;
                    break;
                case OPCODE_PUSHCLE:
#ifdef DEBUG_ASSEMBLER
                    fprintf(stderr, "Assembler::assembleExpression: Test was CMP/PUSHCLE!\n");
#endif
                    m_code.pop_back();
                    branchOpcode = OPCODE_BG;
                    break;
            }
        }
    }

    if (branchOpcode == OPCODE_MAX)
    {
        // Check whether the test expression was *false*
        pushInstruction(OPCODE_PUSHI, 0);
        pushInstruction(OPCODE_CMPI);
        branchOpcode = OPCODE_BEQ;
    }

    // If false, skip to the end
    pushInstruction(branchOpcode, 0); // Will be filled in with the end address

    return true;
}

bool Assembler::assembleReference(CodeBlock* block, OperationExpression* expr)
{
    bool res;
#ifdef DEBUG_ASSEMBLER
    printf("Assembler::assembleReference: Assembling Reference: %ls\n", expr->toString().c_str());
#endif

    res = assembleExpression(block, expr->left, NULL, true);
    if (!res)
    {
        return false;
    }

    if (IS_REFERENCE(expr->right))
    {
        return assembleReference(block, (OperationExpression*)(expr->right));
    }

    return true;
}

bool Assembler::isVariable(CodeBlock* block, Object* context, wstring name)
{
    int id = block->getVarId(name);
#ifdef DEBUG_ASSEMBLER
    printf("Assembler::isVariable: Local variable %ls (%d)\n", name.c_str(), id);
#endif
    if (id != -1)
    {
        // Local variable!
        return true;
    }
    else
    {
        id = m_function->getClass()->getFieldId(name);

#ifdef DEBUG_ASSEMBLER
        printf("Assembler::isVariable: Object field %ls (%d)\n", name.c_str(), id);
#endif
        if (id != -1)
        {
            // object field!
            return true;
        }
        else
        {
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::isVariable: Unknown variable: %ls\n", name.c_str());
#endif
            return false;
        }
    }
    return false;
}

bool Assembler::load(CodeBlock* block, VarExpression* varExpr, OperationExpression* reference)
{
    bool res;
    if (reference != NULL)
    {
        res = assembleReference(block, (OperationExpression*)reference);
        if (!res)
        {
            return false;
        }

        Object* strObj = m_context->getRuntime()->newString(m_context, varExpr->var);
        strObj->setExternalGC();

        pushInstruction(OPCODE_PUSHOBJ, (uint64_t)strObj);
        pushInstruction(OPCODE_LOAD_FIELD_NAMED);
        return true;
    }

    if (varExpr->clazz != NULL)
    {

#ifdef DEBUG_ASSEMBLER
        printf("Assembler::load: Looking for static field %ls %ls\n", varExpr->clazz->getName().c_str(), varExpr->var.c_str());
#endif

        int id = varExpr->clazz->getStaticFieldId(varExpr->var);
        if (id != -1)
        {
            pushInstruction(OPCODE_LOAD_STATIC_FIELD, (uint64_t)varExpr->clazz, id);
            return true;
        }

        printf("Assembler::load: ERROR: Failed to find static field %ls %ls\n", varExpr->clazz->getName().c_str(), varExpr->var.c_str());
        return false;
    }

    int id;
    if (varExpr->var == L"this")
    {
        id = 0;
    }
    else
    {
        id = block->getVarId(varExpr->var);
    }

#ifdef DEBUG_ASSEMBLER
    printf("Assembler::load: Local variable %ls (%d)\n", varExpr->var.c_str(), id);
#endif
    if (id != -1)
    {
        // Local variable!
        pushInstruction(OPCODE_LOAD_VAR, id);
        return true;
    }
    else
    {
        if (m_function->getClass() != NULL)
        {
            id = m_function->getClass()->getFieldId(varExpr->var);
        }

#ifdef DEBUG_ASSEMBLER
        printf("Assembler::load: Object field %ls (%d)\n", varExpr->var.c_str(), id);
#endif
        if (id != -1)
        {
            // object field!
            pushInstruction(OPCODE_LOAD_VAR, 0); // Load "this"
            pushInstruction(OPCODE_LOAD_FIELD, id);
            return true;
        }
        else
        {
            id = m_function->getClass()->getStaticFieldId(varExpr->var);
            if (id != -1)
            {
                // static class field!
                pushInstruction(OPCODE_LOAD_STATIC_FIELD, (uint64_t)m_function->getClass(), id);
                return true;
            }
            else
            {
 
                printf("Assembler::load: Error: Unknown variable: %ls\n", varExpr->var.c_str());
                if (m_function->getClass() != NULL)
                {
                    printf("Assembler::load:  -> class=%ls\n", m_function->getClass()->getName().c_str());
                }
                else
                {
                    printf("Assembler::load:  -> class=NULL\n");
                }
                return false;
            }
        }
    }
}

bool Assembler::store(CodeBlock* block, VarExpression* varExpr, OperationExpression* reference)
{
    if (reference != NULL)
    {
        bool res = assembleReference(block, reference);
        if (!res)
        {
            return false;
        }
#ifdef DEBUG_ASSEMBLER
        printf("Assembler::store: Var from reference: %ls\n", varExpr->var.c_str());
#endif

        Object* strObj = m_context->getRuntime()->newString(m_context, varExpr->var);
        strObj->setExternalGC();

        pushInstruction(OPCODE_PUSHOBJ, (uint64_t)strObj);
        pushInstruction(OPCODE_STORE_FIELD_NAMED);

        return true;
    }

    wstring name = varExpr->var;

    int id;
    if (name == L"this")
    {
        id = 0;
    }
    else
    {
        id = block->getVarId(name);
    }

#ifdef DEBUG_ASSEMBLER
    printf("Assembler::store: Local variable %ls (%d)\n", name.c_str(), id);
#endif
    if (id != -1)
    {
        // Local variable!
        pushInstruction(OPCODE_STORE_VAR, id);
        return true;
    }
    else
    {
        id = m_function->getClass()->getFieldId(name);

#ifdef DEBUG_ASSEMBLER
        printf("Assembler::store: Object field %ls (%d)\n", name.c_str(), id);
#endif
        if (id != -1)
        {
            // object field!
            pushInstruction(OPCODE_LOAD_VAR, 0); // Load "this"
            pushInstruction(OPCODE_STORE_FIELD, id);
            return true;
        }
        else
        {
            id = m_function->getClass()->getStaticFieldId(name);
            if (id != -1)
            {
                // static class field!
                pushInstruction(OPCODE_STORE_STATIC_FIELD, (uint64_t)m_function->getClass(), id);
                return true;
            }
            else
            {
                printf("Assembler::store: Error: Unknown variable: %ls\n", name.c_str());
                return false;
            }
        }
    }
}

void Assembler::pushInstruction(OpCode op, ...)
{
    va_list vl;
    va_start(vl, op);

    Instruction inst;
    inst.op = op;

    OpCodeInfo opInfo = OpCodeInfo::getOpcodeInfo(op);

    int i;
    for (i = 0; i < opInfo.args; i++)
    {
        uint64_t arg = va_arg(vl, uint64_t);
        inst.args.push_back(arg);
    }
    m_code.push_back(inst);
    va_end(vl);
}

struct VarInfo
{
    int type;
    int loaded;
    int stored;
    Value lastValue;
};

bool Assembler::optimise(AssembledCode& code)
{
set<int> removeList;
    VarInfo vars[code.localVars];
    int i;

    // Set up vars array
    for (i = 0; i < code.localVars; i++)
    {
        vars[i].type = 0;
        vars[i].loaded = 0;
        vars[i].stored = (i <= m_function->getArgCount());
        vars[i].lastValue.type = VALUE_UNKNOWN;
    }

    int prevType = 0;
    Value lastValue;
    lastValue.type = VALUE_UNKNOWN;

    /*
     * Pass 1: Infer types, update any arithmetic to use these types if
     * possible and any other obvious optimisations (Double loads etc)
     */
    vector<Instruction>::iterator it;
    for (it = m_code.begin(); it != m_code.end(); it++)
    {
        OpCode op = it->op;

        int type = (op & 0xf0) >> 4;
#ifdef DEBUG_OPTIMISER
        fprintf(stderr, "Assembler::optimise: %ls: Opcode 0x%x: Type: %d\n", m_function->getFullName().c_str(), op, type);
#endif

        if (op == OPCODE_LOAD_VAR)
        {
            int v = (*it).args[0];
            prevType = vars[v].type;
            vars[v].loaded++;
#ifdef DEBUG_OPTIMISER
            fprintf(stderr, "Assembler::optimise: %ls: LOAD_VAR: v=%d, type=%d\n", m_function->getFullName().c_str(), v, prevType);
            if (vars[v].lastValue.type != VALUE_UNKNOWN)
            {
                fprintf(stderr, "Assembler::optimise: %ls: LOAD_VAR:  -> Loading known value: %ls\n", m_function->getFullName().c_str(), vars[v].lastValue.toString().c_str());
            }
#endif
            lastValue = vars[v].lastValue;
        }
        else if (op == OPCODE_STORE_VAR)
        {
            int v = (*it).args[0];
#ifdef DEBUG_OPTIMISER
            fprintf(stderr, "Assembler::optimise: %ls: STORE_VAR: v=%d, prevType=%d\n", m_function->getFullName().c_str(), v, prevType);
#endif
            vars[v].stored++;
            vars[v].type = prevType;

            if (lastValue.type != VALUE_UNKNOWN)
            {
#ifdef DEBUG_OPTIMISER
                fprintf(
                    stderr,
                    "Assembler::optimise: %ls: STORE_VAR:  -> Storing known value: %ls\n",
                    m_function->getFullName().c_str(),
                    lastValue.toString().c_str());
#endif
            }
            vars[v].lastValue = lastValue;
        }
        else if (op == OPCODE_INC_VAR)
        {
            int v = (*it).args[0];
            vars[v].type = 0;
            vars[v].loaded++;
            vars[v].stored++;
        }
        else
        {
            if (prevType != 0 && type == 0 && (op & 0xf00) == 0x200)
            {
                OpCode newOp = (OpCode)((int)op | (prevType << 4));
#ifdef DEBUG_OPTIMISER
                fprintf(
                    stderr,
                    "Assembler::optimise: %ls: ADDING TYPE TO ARITHMETIC OP 0x%x -> 0x%x\n",
                    m_function->getFullName().c_str(),
                    op,
                    newOp);
#endif
                (*(it)).op = newOp;
            }
            else if (op == OPCODE_DUP)
            {
                // Same as the previous!
            }
            else
            {
                prevType = type;
            }

            if (op == OPCODE_PUSHI)
            {
                lastValue.type = VALUE_INTEGER;
                lastValue.i = (*it).args[0];
            }
            else if (op == OPCODE_PUSHD)
            {
                lastValue.type = VALUE_DOUBLE;
                uint64_t id = (*it).args[0];
                double* dp = (double*)&id;
                lastValue.d = *dp;
            }
            else
            {
                lastValue.type = VALUE_UNKNOWN;
                lastValue.i = 0;
            }
        }

        if ((it + 1) != m_code.end())
        {
            // Optimisations that require looking at the next var
            if ((*it).op == OPCODE_LOAD_VAR && (*(it + 1)).op == OPCODE_LOAD_VAR)
            {
                /*
                 * LOAD_VAR vX
                 * LOAD_VAR vX
                 *  to
                 * LOAD_VAR vX
                 * DUP
                 */
                if ((*it).args.at(0) == (*(it + 1)).args.at(0))
                {
#ifdef DEBUG_OPTIMISER
                    fprintf(
                        stderr,
                        "Assembler::optimise: %ls: Optimised out two LOAD_VARs\n",
                        m_function->getFullName().c_str());
#endif
                    (*(it + 1)).op = OPCODE_DUP;
                    (*(it + 1)).args.clear();
                }
            }
        }
    }

    /*
     * Check variables
     * Sanity check that all variables are correctly written etc.
     * If we only store a var once or it's never read, we'll need another
     * pass to deal with them
     */
    bool varPass = false;
    for (i = 0; i < code.localVars; i++)
    {
#ifdef DEBUG_OPTIMISER
        fprintf(
            stderr,
            "Assembler::optimise: %ls: v%d: type=%d, loaded=%d, stored=%d\n",
            m_function->getFullName().c_str(),
            i,
            vars[i].type,
            vars[i].loaded,
            vars[i].stored);
#endif
        if (vars[i].stored == 1 && vars[i].lastValue.type != VALUE_UNKNOWN)
        {
            varPass = true;
        }
        if (vars[i].loaded == 0)
        {
            varPass = true;
        }
    }

    /*
     * Pass 2: Replace known variable loads with a PUSHx
     */
    if (varPass)
    {
        for (it = m_code.begin(); it != m_code.end(); it++)
        {
            OpCode op = it->op;
            if (op == OPCODE_LOAD_VAR)
            {
                int v = it->args[0];
                if (vars[v].stored == 1 && vars[v].lastValue.type != VALUE_UNKNOWN)
                {
#ifdef DEBUG_OPTIMISER
                    fprintf(
                        stderr,
                        "Assembler::optimise: %ls: VAR PASS: LOAD_VAR:  -> Loading known value: %ls\n",
                        m_function->getFullName().c_str(),
                        vars[v].lastValue.toString().c_str());
#endif

                    if (vars[v].type == 1)
                    {
                        (*it).op = OPCODE_PUSHI;
                        (*it).args[0] = vars[v].lastValue.i;
#ifdef DEBUG_OPTIMISER
                        fprintf(
                            stderr,
                            "Assembler::optimise: %ls: LOAD_VAR -> PUSHI %lld\n",
                            m_function->getFullName().c_str(),
                            vars[v].lastValue.i);
#endif
                        vars[v].loaded--;
                    }
                    else if (vars[v].type == 2)
                    {
                        (*it).op = OPCODE_PUSHD;
                        int64_t* dtoi = (int64_t*)(&(vars[v].lastValue.d));
                        (*it).args[0] = *dtoi;
#ifdef DEBUG_OPTIMISER
                        fprintf(
                            stderr,
                            "Assembler::optimise: %ls: LOAD_VAR -> PUSHD %0.2f\n",
                            m_function->getFullName().c_str(),
                            vars[v].lastValue.d);
#endif
                        vars[v].loaded--;
                    }
                }
            }
        }
    }

    /*
     * Another variable check. If variables are never loaded
     * (Either through optimisation or otherwise), we'll need
     * a pass to get rid of them
     */
    bool removePass = false;
    for (i = 0; i < code.localVars; i++)
    {
#ifdef DEBUG_OPTIMISER
        fprintf(
            stderr,
            "Assembler::optimise: %ls: v%d: type=%d, loaded=%d, stored=%d\n",
            m_function->getFullName().c_str(),
            i,
            vars[i].type,
            vars[i].loaded,
            vars[i].stored);
#endif
        if (vars[i].loaded == 0)
        {
            removePass = true;
        }
    }

    /*
     * Pass 3: Check for operations that we can ditch
     */
    if (removePass)
    {
        int pos;
        OpCode prevOp = OPCODE_NOP;
        for (it = m_code.begin(), pos = 0; it != m_code.end(); it++, pos++)
        {
            OpCode op = it->op;
            if (op == OPCODE_STORE_VAR)
            {
                int v = (*it).args[0];
                if (vars[v].loaded == 0 && (prevOp == OPCODE_PUSHI || prevOp == OPCODE_PUSHD))
                {
                    fprintf(
                        stderr,
                        "Assembler::optimise: %ls: REMOVE: STORE_VAR v%d: Removing push and store!\n",
                        m_function->getFullName().c_str(),
                        v);
                    removeList.insert(pos - 1);
                    removeList.insert(pos);
                }
            }

            prevOp = op;
        }

        /*
         * Do the removal!
         */
        set<int>::iterator rIt;
        for (rIt = removeList.begin(), pos = 0; rIt != removeList.end(); rIt++, pos++)
        {
            removeInstruction(*rIt - pos);
        }
    }

    return true;
}

bool Assembler::removeInstruction(int pos)
{
    int i;
    vector<Instruction>::iterator it;

    // Remove the instruction...
    for (it = m_code.begin(), i = 0; it != m_code.end(); it++, i++)
    {
        if (i == pos)
        {
            m_code.erase(it);
            break;
        }
    }

    // Update addresses
    for (it = m_code.begin(), i = 0; it != m_code.end(); it++, i++)
    {
        OpCode op = it->op;
        OpCodeInfo info = OpCodeInfo::getOpcodeInfo(op);
        if (info.address != -1)
        {
            uint64_t addr = it->args[info.address];
            if (addr > pos)
            {
                addr--;
            }
            it->args[info.address] = addr;
        }
    }
    return true;
}

