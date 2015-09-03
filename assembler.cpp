
#include "assembler.h"
#include "runtime.h"
#include "function.h"

Assembler::Assembler(Runtime* runtime)
{
    m_runtime = runtime;
}

Assembler::~Assembler()
{
}

bool Assembler::assemble(ScriptFunction* function, AssembledCode& asmCode)
{
    m_function = function;
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
    m_code.push_back(OPCODE_RETURN);

    asmCode.code = new uint64_t[m_code.size()];
    asmCode.size = m_code.size();
    asmCode.localVars = code->m_maxVarId + 1;

    for (i = 0; i < m_code.size(); i++)
    {
#if 0
        printf("%04x: 0x%llx\n", i, m_code[i]);
#endif
        asmCode.code[i] = m_code[i];
    }

    return true;
}

bool Assembler::assembleBlock(CodeBlock* code)
{
   unsigned int i;
    for (i = 0; i < code->m_code.size(); i++)
    {
        bool res;
        res = assembleExpression(code, code->m_code[i]);
        if (!res)
        {
            return false;
        }
    }
    return true;
}


bool Assembler::assembleExpression(CodeBlock* block, Expression* expr)
{
    bool res;

    switch (expr->type)
    {
        case EXPR_CALL:
        {
            CallExpression* callExpr = (CallExpression*)expr;
            std::vector<Expression*>::iterator it;
            int count = 0;
            for (it = callExpr->parameters.begin(); it != callExpr->parameters.end(); it++)
            {
                res = assembleExpression(block, *it);
                if (!res)
                {
                    return false;
                }
                count++;
            }
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: CALL: %s\n", callExpr->function.toString().c_str());
#endif

            Identifier id = callExpr->function;
            if (id.identifier.size() > 1)
            {
                // TODO: Handle more than 2 parts!

                // Does it start with a local object?
                int varId = block->getVarId(id.identifier[0]);
                if (varId != -1)
                {
                    m_code.push_back(OPCODE_NEW_STRING);
                    m_code.push_back((uint64_t)strdup(id.identifier[1].c_str()));
                    m_code.push_back(OPCODE_LOAD);
                    m_code.push_back(varId);
                    m_code.push_back(OPCODE_CALL_NAMED);
                    m_code.push_back(count);
                }
                else
                {
                    // Try class names?
                    Class* clazz = m_runtime->findClass(id.identifier[0]);

#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::findFunction: nexecute: identifier: clazz=%p\n", clazz);
#endif
                    if (clazz != NULL)
                    {
                        Function* func = clazz->findMethod(id.identifier[1]);
                        m_code.push_back(OPCODE_CALL_STATIC);
                        m_code.push_back((uint64_t)func);
                        m_code.push_back(count);
                    }
                    else
                    {
                        printf("TODO: Assembler::findFunction: Not found object method id: %s\n", id.toString().c_str());
                        return false;
                    }
                }
            }
            else
            {
                // No qualifier, just a function name

                // TODO: Try local variables

                // Try methods of the same class
                Function* func = m_function->getClass()->findMethod(id.identifier[0]);
                if (func != NULL)
                {
                    printf("TODO: Assembler::findFunction: Found method for %s: %p\n", id.toString().c_str(), func);
                    m_code.push_back(OPCODE_CALL_STATIC);
                    m_code.push_back((uint64_t)func);
                    m_code.push_back(count);
                }
                else
                {
                    printf("TODO: Assembler::findFunction: Not found relative id: %s\n", id.toString().c_str());
                    return false;
                }
            }
        } break;

        case EXPR_OPER:
        {
            OperationExpression* opExpr = (OperationExpression*)expr;

            if (opExpr->right != NULL)
            {
                res = assembleExpression(block, opExpr->right);
                if (!res)
                {
                    return false;
                }
            }

            if (opExpr->operType != OP_SET && opExpr->operType != OP_INCREMENT)
            {
                res = assembleExpression(block, opExpr->left);
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
                    switch (opExpr->valueType)
                    {
                        case VALUE_INTEGER:
                            m_code.push_back(OPCODE_ADDI);
                            break;
                        case VALUE_DOUBLE:
                            m_code.push_back(OPCODE_ADDD);
                            break;
                        default:
                            m_code.push_back(OPCODE_ADD);
                            break;
                    }
                    break;

                case OP_SUB:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: SUB\n");
#endif
                    switch (opExpr->valueType)
                    {
                        case VALUE_INTEGER:
                            m_code.push_back(OPCODE_SUBI);
                            break;
                        case VALUE_DOUBLE:
                            m_code.push_back(OPCODE_SUBD);
                            break;
                        default:
                            m_code.push_back(OPCODE_SUB);
                            break;
                    }
                    break;

                case OP_MULTIPLY:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: MULTIPLY\n");
#endif
                    switch (opExpr->valueType)
                    {
                        case VALUE_INTEGER:
                            m_code.push_back(OPCODE_MULI);
                            break;
                        case VALUE_DOUBLE:
                            m_code.push_back(OPCODE_MULD);
                            break;
                        default:
                            m_code.push_back(OPCODE_MUL);
                            break;
                    }
                    break;

                case OP_LOGICAL_AND:
                {
#ifdef DEBUG_ASSEMBLER
                    ValueType leftType = opExpr->left->valueType;
                    ValueType rightType = opExpr->right->valueType;
                    printf("Assembler::assembleExpression: OPER: LOGICAL_AND (type=%d)\n", opExpr->valueType);
                    printf("Assembler::assembleExpression: OPER: LOGICAL_AND left=%d, right=%d\n", leftType, rightType);
#endif
                    switch (opExpr->valueType)
                    {
                        case VALUE_INTEGER:
                            m_code.push_back(OPCODE_ANDI);
                            break;
                        default:
                            m_code.push_back(OPCODE_AND);
                            break;
                    }
                } break;

                case OP_LESS_THAN:
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: LESS_THAN\n");
#endif
                    switch (opExpr->valueType)
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
 
                    m_code.push_back(OPCODE_PUSHCL);
                    break;

                case OP_INCREMENT:
                {
                    if (opExpr->left->type != EXPR_VAR)
                    {
                        printf("Assembler::assembleExpression: OPER: INCREMENT: Error: Left must be a variable!\n");
                        return false;
                    }

                    VarExpression* varExpr = (VarExpression*)(opExpr->left);
                    int id = block->getVarId(varExpr->var.identifier[0]);

#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: POST INCREMENT (%d)\n", id);
#endif
                    m_code.push_back(OPCODE_LOAD);
                    m_code.push_back(id);
                    m_code.push_back(OPCODE_PUSHI);
                    m_code.push_back(1);
                    m_code.push_back(OPCODE_ADDI);

                    m_code.push_back(OPCODE_STORE);
                    m_code.push_back(id);
 
                    } break;

                case OP_SET:
                {
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: SET\n");
                    printf("Assembler::assembleExpression: OPER: -> left=%p\n", opExpr->left);
                    printf("Assembler::assembleExpression: OPER: -> left=%s\n", opExpr->left->toString().c_str());
#endif
                    if (opExpr->left->type != EXPR_VAR)
                    {
                        printf("Assembler::assembleExpression: OPER: SET: Error: Left must be a variable!\n");
                        return false;
                    }
                    VarExpression* varExpr = (VarExpression*)(opExpr->left);
                    int id = block->getVarId(varExpr->var.identifier[0]);
                    m_code.push_back(OPCODE_STORE);
                    m_code.push_back(id);
                } break;

                default:
                    printf("Assembler::assembleExpression: OPER: Unhandled operation\n");
                    return false;
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
                res = assembleExpression(block, forExpr->initExpr);
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
            res = assembleExpression(block, forExpr->testExpr);
            if (!res)
            {
                return false;
            }

            // Check whether the test expression was true
            m_code.push_back(OPCODE_PUSHI);
            m_code.push_back(0);
            m_code.push_back(OPCODE_CMP);

            // If true
            m_code.push_back(OPCODE_BEQ);
            m_code.push_back(0); // Will be filled in with the end address
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
                res = assembleExpression(block, forExpr->incExpr);
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
                res = assembleExpression(block, retExpr->returnValue);
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
            int id = block->getVarId(varExpr->var.identifier[0]);
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: VAR: PUSH ADDRESS OF %s (%d)\n", varExpr->var.toString().c_str(), id);
#endif
            // TODO: Local? Class? Static?

            m_code.push_back(OPCODE_LOAD);
            m_code.push_back(id);
        } break;

        case EXPR_NEW:
        {
            NewExpression* newExpr = (NewExpression*)expr;
            Class* clazz = m_runtime->findClass(newExpr->clazz.identifier[0]);
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleExpression: NEW: %s = %p\n", newExpr->clazz.toString().c_str(), clazz);
#endif

            int count = 0;
            std::vector<Expression*>::iterator it;
            for (it = newExpr->parameters.begin(); it != newExpr->parameters.end(); it++)
            {
                res = assembleExpression(block, *it);
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
            m_code.push_back(OPCODE_NEW_STRING);
            m_code.push_back((uint64_t)strdup(strExpr->str.c_str()));
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

        default:
            printf("Assembler::assembleExpression: Unhandled expression: %d: %s\n", expr->type, expr->toString().c_str());
            return false;
    }
    return true;
}

