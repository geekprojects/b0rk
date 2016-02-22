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

#include <b0rk/assembler.h>
#include <b0rk/runtime.h>
#include <b0rk/function.h>
#include <b0rk/disassembler.h>
#include <b0rk/utils.h>

#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;

Assembler::Assembler(Context* context)
{
    m_context = context;

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

    m_code.push_back(OPCODE_PUSHI);
    m_code.push_back(0);
    m_code.push_back(OPCODE_RETURN);

    asmCode.code = new uint64_t[m_code.size()];
    asmCode.size = m_code.size();
    asmCode.localVars = code->m_maxVarId + 1;

    for (i = 0; i < m_code.size(); i++)
    {
        asmCode.code[i] = m_code[i];
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
        printf("Assembler::assembleBlock: expr: %s\n", expr->toString().c_str());
#endif

        bool res;
        res = assembleExpression(code, expr, NULL, false);
        if (!res)
        {
            return false;
        }

        if (IS_REFERENCE(expr))
        {
            Expression* pos = expr;
            while (IS_REFERENCE(pos))
            {
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleBlock: Checking reference: %s\n", pos->toString().c_str());
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
            m_code.push_back((int)opcode + 0x10);
            break;
        case VALUE_DOUBLE:
            m_code.push_back((int)opcode + 0x20);
            break;
        default:
            m_code.push_back(opcode);
            break;
    }
}

void Assembler::pushCMP(ValueType type)
{
    switch (type)
    {
        case VALUE_INTEGER:
            m_code.push_back(OPCODE_CMPI);
            break;
        case VALUE_DOUBLE:
            m_code.push_back(OPCODE_CMPD);
            break;
        default:
            m_code.push_back(OPCODE_CMP);
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
                    return false;
                }
                count++;
            }
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: CALL: %s (class=%p)\n", callExpr->function.c_str(), callExpr->clazz);
#endif

            if (reference != NULL)
            {
                res = assembleReference(block, reference);
                if (!res)
                {
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
                m_code.push_back(OPCODE_CALL_STATIC);
                m_code.push_back((uint64_t)func);
                m_code.push_back(count);
            }
            else
            {

#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleExpression: CALL: Named call?\n");
#endif
                bool inReference = false;
                if (callExpr->parent != NULL)
                {
                    if (callExpr->parent->type == EXPR_OPER && ((OperationExpression*)(callExpr->parent))->operType == OP_REFERENCE)
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
                        printf("Assembler::assembleExpression: CALL: parent=%s\n", callExpr->parent->toString().c_str());
#endif
                    }
                }

                if (inReference)
                {
                    m_code.push_back(OPCODE_PUSHOBJ);
                    Object* strObj = String::createString(m_context, callExpr->function);
                    m_code.push_back((uint64_t)strObj);
                    strObj->setExternalGC();
                    m_code.push_back(OPCODE_CALL_NAMED);
                    m_code.push_back(count);
                }
                else
                {
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: CALL: Not a reference, must be THIS\n");
#endif

                    bool isVar = isVariable(block, NULL, callExpr->function);
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: CALL: %s: isVar=%d\n", callExpr->function.c_str(), isVar);
#endif
                    if (isVar)
                    {
                        VarExpression varExpr(block);
                        varExpr.clazz = NULL;
                        varExpr.var = callExpr->function;
                        load(block, &varExpr, reference);
                        m_code.push_back(OPCODE_CALL_OBJ);
                        m_code.push_back(count);
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
                            m_code.push_back(OPCODE_CALL_STATIC);
                            m_code.push_back((uint64_t)func);
                            m_code.push_back(count);
                        }
                        else
                        {
                            m_code.push_back(OPCODE_LOAD_VAR);
                            m_code.push_back(0); // this

                            m_code.push_back(OPCODE_CALL);
                            m_code.push_back((uint64_t)func);
                            m_code.push_back(count);
                        }
                    }
                }
            }
            if (!needResult)
            {
                m_code.push_back(OPCODE_POP);
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
                printf("Assembler::assembleExpression: OPER: Reference: Assembling nearest non-ref child: %s\n", refChild->toString().c_str());
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

            if (opExpr->operType != OP_SET && opExpr->operType != OP_INCREMENT && opExpr->operType != OP_DECREMENT)
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
                    m_code.push_back(OPCODE_PUSHCE);
                    break;

                case OP_LESS_THAN:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: LESS_THAN\n");
#endif
                    pushCMP(opExpr->valueType);
                    m_code.push_back(OPCODE_PUSHCL);
                    break;

                case OP_LESS_THAN_EQUAL:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: LESS_THAN_EQUAL\n");
#endif
                    pushCMP(opExpr->valueType);
                    m_code.push_back(OPCODE_PUSHCLE);
                    break;


                case OP_GREATER_THAN:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: GREATER_THAN\n");
#endif
                    pushCMP(opExpr->valueType);
                    m_code.push_back(OPCODE_PUSHCG);
                    break;

                case OP_GREATER_THAN_EQUAL:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: GREATER_THAN\n");
#endif
                    pushCMP(opExpr->valueType);
                    m_code.push_back(OPCODE_PUSHCGE);
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
                        m_code.push_back(OPCODE_INC_VAR);
                        m_code.push_back(id);
                        if (opExpr->operType == OP_INCREMENT)
                        {
                            m_code.push_back(1);
                        }
                        else
                        {
                            m_code.push_back(-1);
                        }
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
                            m_code.push_back(OPCODE_DUP);
                        }

                        expr->resultOnStack = needResult;

                        m_code.push_back(OPCODE_PUSHI);
                        m_code.push_back(1);

                        if (opExpr->operType == OP_INCREMENT)
                        {
                            m_code.push_back(OPCODE_ADDI);
                        }
                        else
                        {
                            m_code.push_back(OPCODE_SUBI);
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
                    printf("Assembler::assembleExpression: OPER: -> left=%s\n", opExpr->left->toString().c_str());
#endif

                    if (needResult)
                    {
                        m_code.push_back(OPCODE_DUP);
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
                        printf("Assembler::assembleExpression: OPER: -> first non reference=%s\n", dest->toString().c_str());
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

                        m_code.push_back(OPCODE_STORE_ARRAY);
                    }
                    else
                    {
                        printf("Assembler::assembleExpression: OPER: SET: Error: Left must be a variable or array!\n");
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
                m_code.push_back(OPCODE_JMP);
                m_code.push_back(0);
                jmpToEndPos = m_code.size() - 1;
            }

            m_code[bneToFalsePos] = m_code.size();
            if (ifExpr->falseBlock != NULL)
            {

                // Add the body
                res = assembleBlock(ifExpr->falseBlock);
                if (!res)
                {
                    return false;
                }

                m_code[jmpToEndPos] = m_code.size();
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
            m_code.push_back(OPCODE_JMP);
            m_code.push_back(loopTop);
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FOR: done!\n");
#endif

            m_code[bneToEndPos] = m_code.size();
        } break;

        case EXPR_RETURN:
        {
            ReturnExpression* retExpr = (ReturnExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: RETURN\n");
#endif
            if (retExpr->returnValue == NULL)
            {
                m_code.push_back(OPCODE_PUSHI);
                m_code.push_back(0);
            }
            else
            {
                res = assembleExpression(block, retExpr->returnValue, NULL, true);
                if (!res)
                {
                    return false;
                }
            }
            m_code.push_back(OPCODE_RETURN);
        } break;

        case EXPR_VAR:
        {
            VarExpression* varExpr = (VarExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: VAR: var=%s\n", varExpr->var.c_str());
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
            printf("Assembler::assembleExpression: ARRAY!\n");
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

            m_code.push_back(OPCODE_LOAD_ARRAY);
        } break;

        case EXPR_NEW:
        {
            NewExpression* newExpr = (NewExpression*)expr;
            Class* clazz = m_context->getRuntime()->findClass(m_context, newExpr->clazz.toString());
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: NEW: %s = %p\n", newExpr->clazz.toString().c_str(), clazz);
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

            m_code.push_back(OPCODE_NEW);
            m_code.push_back((uint64_t)clazz);
            m_code.push_back(count);
        } break;

        case EXPR_STRING:
        {
            StringExpression* strExpr = (StringExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: STRING: NEW STRING: %s\n", strExpr->str.c_str());
#endif

            m_code.push_back(OPCODE_PUSHOBJ);
            Object* strObj = String::createString(m_context, strExpr->str);
            m_code.push_back((uint64_t)strObj);
            strObj->setExternalGC();
        } break;

        case EXPR_INTEGER:
        {
            IntegerExpression* intExpr = (IntegerExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: INTEGER: PUSH %d\n", intExpr->i);
#endif
            m_code.push_back(OPCODE_PUSHI);
            m_code.push_back(intExpr->i);
        } break;

        case EXPR_DOUBLE:
        {
            DoubleExpression* doubleExpr = (DoubleExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: DOUBLE: PUSH %0.2f\n", doubleExpr->d);
#endif
            m_code.push_back(OPCODE_PUSHD);
            int64_t* dtoi = (int64_t*)(&(doubleExpr->d));
            m_code.push_back(*dtoi);
        } break;

        case EXPR_FUNCTION:
        {
            FunctionExpression* funcExpr = (FunctionExpression*)expr;
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: FUNCTION: NEW FUNCTION %p\n", funcExpr->function);
#endif
            m_code.push_back(OPCODE_NEW_FUNCTION);
            m_code.push_back((uint64_t)funcExpr->function);
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

    int branchOpcode = -1;
    if (m_code.size() > 2)
    {
        // Optimisation: Can we collapse the compare in the test expression and our own CMP?
        uint64_t last0 = m_code[m_code.size() - 2];

        if (last0 == OPCODE_CMP || last0 == OPCODE_CMPI || OPCODE_CMPD)
        {
            uint64_t last1 = m_code[m_code.size() - 1];

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

    if (branchOpcode == -1)
    {
        // Check whether the test expression was *false*
        m_code.push_back(OPCODE_PUSHI);
        m_code.push_back(0);
        m_code.push_back(OPCODE_CMPI);
        branchOpcode = OPCODE_BEQ;
    }

    // If false, skip to the end
    m_code.push_back(branchOpcode);
    m_code.push_back(0); // Will be filled in with the end address

    return true;
}

bool Assembler::assembleReference(CodeBlock* block, OperationExpression* expr)
{
    bool res;
#ifdef DEBUG_ASSEMBLER
    printf("Assembler::assembleReference: Assembling Reference: %s\n", expr->toString().c_str());
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

        m_code.push_back(OPCODE_PUSHOBJ);
        Object* strObj = String::createString(m_context, varExpr->var);
        m_code.push_back((uint64_t)strObj);
        strObj->setExternalGC();

        m_code.push_back(OPCODE_LOAD_FIELD_NAMED);
        return true;
    }

    if (varExpr->clazz != NULL)
    {

#ifdef DEBUG_ASSEMBLER
        printf("Assembler::load: Looking for static field %s %s\n", varExpr->clazz->getName().c_str(), varExpr->var.c_str());
#endif

        int id = varExpr->clazz->getStaticFieldId(varExpr->var);
        if (id != -1)
        {
            m_code.push_back(OPCODE_LOAD_STATIC_FIELD);
            m_code.push_back((uint64_t)varExpr->clazz);
            m_code.push_back(id);
            return true;
        }
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
        m_code.push_back(OPCODE_LOAD_VAR);
        m_code.push_back(id);
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
            m_code.push_back(OPCODE_LOAD_VAR);
            m_code.push_back(0); // this
            m_code.push_back(OPCODE_LOAD_FIELD);
            m_code.push_back(id);
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

bool Assembler::store(CodeBlock* block, VarExpression* varExpr, OperationExpression* reference)
{
    if (reference != NULL)
    {
        bool res = assembleReference(block, reference);
        if (!res)
        {
            return false;
        }
        printf("Assembler::store: Var from reference: %ls\n", varExpr->var.c_str());

        m_code.push_back(OPCODE_PUSHOBJ);
        Object* strObj = String::createString(m_context, varExpr->var);
        m_code.push_back((uint64_t)strObj);
        strObj->setExternalGC();

        m_code.push_back(OPCODE_STORE_FIELD_NAMED);
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
        m_code.push_back(OPCODE_STORE_VAR);
        m_code.push_back(id);
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
            m_code.push_back(OPCODE_LOAD_VAR);
            m_code.push_back(0); // this
            m_code.push_back(OPCODE_STORE_FIELD);
            m_code.push_back(id);
            return true;
        }
        else
        {
            printf("Assembler::store: Error: Unknown variable: %ls\n", name.c_str());
            return false;
        }
    }
}

