
#undef DEBUG_ASSEMBLER

#include <b0rk/assembler.h>
#include <b0rk/runtime.h>
#include <b0rk/function.h>

using namespace std;
using namespace b0rk;

Assembler::Assembler(Context* context)
{
    m_context = context;
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

    m_code.push_back(OPCODE_PUSHI);
    m_code.push_back(0);
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

#define IS_REFERENCE(_expr) ((_expr) != NULL && (_expr)->type == EXPR_OPER && ((OperationExpression*)(_expr))->operType == OP_REFERENCE)
bool Assembler::assembleBlock(CodeBlock* code)
{
   unsigned int i;
    for (i = 0; i < code->m_code.size(); i++)
    {
        Expression* expr = code->m_code[i];

        bool res;
        res = assembleExpression(code, expr);
        if (!res)
        {
            return false;
        }

        bool isRefCall = false;
        if (IS_REFERENCE(expr))
        {
            Expression* pos = expr;
            while (IS_REFERENCE(pos))
            {
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleBlock: Checking reference: %s\n", pos->toString().c_str());
#endif
                pos = ((OperationExpression*)pos)->left;
            }
            if (pos != NULL && pos->type == EXPR_CALL)
            {
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleBlock:  -> reference to a call!!\n");
#endif
                isRefCall = true;
            }
        }

        if (isRefCall || code->m_code[i]->type == EXPR_CALL || code->m_code[i]->type == EXPR_NEW)
        {
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::assembleBlock: Call outside of expression!\n");
#endif
            m_code.push_back(OPCODE_POP);
        }
    }
    return true;
}

/*
 * An expression should always leave exactly one item on the stack
 * after execution
 */
bool Assembler::assembleExpression(CodeBlock* block, Expression* expr, Expression* reference)
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
            printf("Assembler::assembleExpression: CALL: %s (class=%p)\n", callExpr->function.c_str(), callExpr->clazz);
#endif

            if (reference != NULL)
            {
                res = assembleReference(block, (OperationExpression*)reference);
                if (!res)
                {
                    return false;
                }
            }

            string id = callExpr->function;

            if (callExpr->clazz != NULL)
            {
                // Static class method!
                Function* func = callExpr->clazz->findMethod(id);
#ifdef DEBUG_ASSEMBLER
                printf("Assembler::assembleExpression: CALL: Static call: %s.%s\n", callExpr->clazz->getName().c_str(), id.c_str());
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
                        printf("Assembler::assembleExpression: CALL: Parent is not null but not a reference!?\n");
                        return false;
                    }
                }

                if (inReference)
                {
                    m_code.push_back(OPCODE_NEW_STRING);
                    m_code.push_back((uint64_t)strdup(callExpr->function.c_str()));
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
                        load(block, NULL, &varExpr);
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

                        m_code.push_back(OPCODE_LOAD_VAR);
                        m_code.push_back(0); // this

                        m_code.push_back(OPCODE_CALL);
                        m_code.push_back((uint64_t)func);
                        m_code.push_back(count);
                    }
                }
           }
        } break;

        case EXPR_OPER:
        {
            OperationExpression* opExpr = (OperationExpression*)expr;

            if (opExpr->operType == OP_REFERENCE && reference == NULL)
            {

                Expression* refChild = expr;
                while (IS_REFERENCE(refChild))
                {
                    refChild = ((OperationExpression*)refChild)->left;
                }

                if (refChild == NULL)
                {
                    printf("Assembler::assembleExpression: OPER: Reference isn't used!?\n");
                    return false;
                }
                return assembleExpression(block, refChild, opExpr);
            }

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
                    if (opExpr->left->type != EXPR_VAR && opExpr->left->type != EXPR_ARRAY)
                    {
                        printf("Assembler::assembleExpression: OPER: INCREMENT: Error: Left must be a variable or array!\n");
                        return false;
                    }


#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: POST INCREMENT %d", 0);
#endif
                    VarExpression* varExpr = (VarExpression*)(opExpr->left);
                    res = load(block, NULL, varExpr);
                    if (!res)
                    {
                        return false;
                    }
                    m_code.push_back(OPCODE_PUSHI);
                    m_code.push_back(1);
                    m_code.push_back(OPCODE_ADDI);
                    res = store(block, NULL, varExpr->var);
                    if (!res)
                    {
                        return false;
                    }
                } break;

                case OP_SET:
                {
#ifdef DEBUG_ASSEMBLER
                    printf("Assembler::assembleExpression: OPER: SET\n");
                    printf("Assembler::assembleExpression: OPER: -> left=%p\n", opExpr->left);
                    printf("Assembler::assembleExpression: OPER: -> left=%s\n", opExpr->left->toString().c_str());
#endif

                    if (opExpr->left->type == EXPR_VAR)
                    {
                        VarExpression* varExpr = (VarExpression*)(opExpr->left);
                        res = store(block, NULL, varExpr->var);
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
                        res = load(block, NULL, arrExpr);
                        if (!res)
                        {
                            return false;
                        }

                        res = assembleExpression(block, arrExpr->indexExpr);
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

            // Add the test
            res = assembleExpression(block, ifExpr->testExpr);
            if (!res)
            {
                return false;
            }

            // Check whether the test expression was true
            m_code.push_back(OPCODE_PUSHI);
            m_code.push_back(0);
            m_code.push_back(OPCODE_CMP);

            // If false
            m_code.push_back(OPCODE_BEQ);
            m_code.push_back(0); // Will be filled in with the end address
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
            res = load(block, NULL, varExpr);
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
            res = load(block, NULL, arrExpr);
            if (!res)
            {
                return false;
            }

            res = assembleExpression(block, arrExpr->indexExpr);
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
                printf("Assembler::assembleExpression: Unknown class: %s\n", newExpr->clazz.toString().c_str());
                return false;
            }

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
            printf("Assembler::assembleExpression: Unhandled expression: %d: %s\n", expr->type, expr->toString().c_str());
            return false;
    }
    return true;
}


bool Assembler::assembleReference(CodeBlock* block, OperationExpression* expr)
{
    bool res;

    res = assembleExpression(block, expr->right);
    if (!res)
    {
        return false;
    }

    if (IS_REFERENCE(expr->left))
    {
        return assembleReference(block, (OperationExpression*)(expr->left));
    }

    return true;
}

bool Assembler::isVariable(CodeBlock* block, Object* context, string name)
{
    int id = block->getVarId(name);
#ifdef DEBUG_ASSEMBLER
    printf("Assembler::isVariable: Local variable %s (%d)\n", name.c_str(), id);
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
        printf("Assembler::isVariable: Object field %s (%d)\n", name.c_str(), id);
#endif
        if (id != -1)
        {
            // object field!
            return true;
        }
        else
        {
#ifdef DEBUG_ASSEMBLER
            printf("Assembler::isVariable: Unknown variable: %s\n", name.c_str());
#endif
            return false;
        }
    }
    return false;
}

bool Assembler::load(CodeBlock* block, Object* context, VarExpression* varExpr)
{
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
    if (varExpr->var == "this")
    {
        id = 0;
    }
    else
    {
        id = block->getVarId(varExpr->var);
    }

#ifdef DEBUG_ASSEMBLER
    printf("Assembler::load: Local variable %s (%d)\n", varExpr->var.c_str(), id);
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
        printf("Assembler::load: Object field %s (%d)\n", varExpr->var.c_str(), id);
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
            printf("Assembler::load: Error: Unknown variable: %s\n", varExpr->var.c_str());
            return false;
        }
    }
}

bool Assembler::store(CodeBlock* block, Object* context, string name)
{
    int id;
    if (name == "this")
    {
        id = 0;
    }
    else
    {
        id = block->getVarId(name);
    }

#ifdef DEBUG_ASSEMBLER
    printf("Assembler::store: Local variable %s (%d)\n", name.c_str(), id);
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
        printf("Assembler::store: Object field %s (%d)\n", name.c_str(), id);
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
            printf("Assembler::store: Error: Unknown variable: %s\n", name.c_str());
            return false;
        }
    }
}

