
#include "assembler.h"
#include "runtime.h"

Assembler::Assembler(Runtime* runtime)
{
    m_runtime = runtime;
}

Assembler::~Assembler()
{
}

bool Assembler::assemble(CodeBlock* code, AssembledCode& asmCode)
{
    bool res;
    res = assembleBlock(code);
    if (!res)
    {
        return false;
    }

    asmCode.code = new uint64_t[m_code.size()];
    asmCode.size = m_code.size();

    unsigned int i;
    for (i = 0; i < m_code.size(); i++)
    {
        printf("%04x: 0x%llx\n", i, m_code[i]);
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
        res = assembleExpression(code->m_code[i]);
        if (!res)
        {
            return false;
        }
    }
    return true;
}


bool Assembler::assembleExpression(Expression* expr)
{
    bool res;

    switch (expr->type)
    {
        case EXPR_CALL:
        {
            CallExpression* callExpr = (CallExpression*)expr;
            std::vector<Expression*>::iterator it;
            for (it = callExpr->parameters.begin(); it != callExpr->parameters.end(); it++)
            {
                res = assembleExpression(*it);
                if (!res)
                {
                    return false;
                }
            }
            printf("Assembler::assembleExpression: CALL: %s\n", callExpr->function.toString().c_str());

            Function* function = findFunction(callExpr->function);
            printf("Assembler::assembleExpression: CALL: function=%p\n", function);
            if (function == NULL)
            {
                printf("Assembler::assembleExpression: CALL: Unknown function: %s\n", callExpr->function.toString().c_str());
                return false;
            }

            m_code.push_back(OPCODE_CALL);
            m_code.push_back((uint64_t)function);
        } break;

        case EXPR_OPER:
        {
            OperationExpression* opExpr = (OperationExpression*)expr;

            if (opExpr->right != NULL)
            {
                res = assembleExpression(opExpr->right);
                if (!res)
                {
                    return false;
                }
            }

            if (opExpr->operType != OP_SET && opExpr->operType != OP_INCREMENT)
            {
                res = assembleExpression(opExpr->left);
                if (!res)
                {
                    return false;
                }
            }

            switch (opExpr->operType)
            {
                case OP_ADD:
                    printf("Assembler::assembleExpression: OPER: ADD\n");
                    m_code.push_back(OPCODE_ADD);
                    break;

                case OP_SUB:
                    printf("Assembler::assembleExpression: OPER: SUB\n");
                    m_code.push_back(OPCODE_SUB);
                    break;

                case OP_LESS_THAN:
                    printf("Assembler::assembleExpression: OPER: LESS_THAN\n");
                    m_code.push_back(OPCODE_CMP);
                    m_code.push_back(OPCODE_PUSHCL);
                    break;

                case OP_INCREMENT:
                    printf("Assembler::assembleExpression: OPER: POST INCREMENT\n");
                    m_code.push_back(OPCODE_LOAD);
                    m_code.push_back(0);
                    m_code.push_back(OPCODE_PUSHI);
                    m_code.push_back(1);
                    m_code.push_back(OPCODE_ADD);

                    if (opExpr->left->type != EXPR_VAR)
                    {
                        printf("Assembler::assembleExpression: OPER: INCREMENT: Error: Left must be a variable!\n");
                        return false;
                    }
                    m_code.push_back(OPCODE_STORE);
                    m_code.push_back(0);
 
                    break;

                case OP_SET:
                    printf("Assembler::assembleExpression: OPER: SET\n");
                    if (opExpr->left->type != EXPR_VAR)
                    {
                        printf("Assembler::assembleExpression: OPER: SET: Error: Left must be a variable!\n");
                        return false;
                    }
                    m_code.push_back(OPCODE_STORE);
                    m_code.push_back(0);
                    break;

                default:
                    printf("Assembler::assembleExpression: OPER: Unhandled operation\n");
                    return false;
            }
        } break;

        case EXPR_FOR:
        {
            ForExpression* forExpr = (ForExpression*)expr;

            // Add the init
            assembleExpression(forExpr->initExpr);

            // Add the test
            int loopTop = m_code.size();
            assembleExpression(forExpr->testExpr);

            // Check whether the test expression was true
            m_code.push_back(OPCODE_PUSHI);
            m_code.push_back(0);
            m_code.push_back(OPCODE_CMP);

            // If true
            m_code.push_back(OPCODE_BEQ);
            m_code.push_back(0); // Will be filled in with the end address
            int bneToEndPos = m_code.size() - 1;

            // Add the body
            assembleBlock(forExpr->body);

            // Increment
            assembleExpression(forExpr->incExpr);

            // Jump back to the test above
            m_code.push_back(OPCODE_JMP);
            m_code.push_back(loopTop);

            m_code[bneToEndPos] = m_code.size();
        } break;

        case EXPR_VAR:
        {
            VarExpression* varExpr = (VarExpression*)expr;
            printf("Assembler::assembleExpression: VAR: PUSH ADDRESS OF %s\n", varExpr->var.toString().c_str());
            // TODO: Local? Class? Static?
            m_code.push_back(OPCODE_LOAD);
            m_code.push_back(0);
        } break;

        case EXPR_STRING:
        {
            StringExpression* strExpr = (StringExpression*)expr;
            printf("Assembler::assembleExpression: STRING: NEW STRING: %s\n", strExpr->str.c_str());
            m_code.push_back(OPCODE_NEW_STRING);
            m_code.push_back((uint64_t)strdup(strExpr->str.c_str()));
        } break;

        case EXPR_INTEGER:
        {
            IntegerExpression* intExpr = (IntegerExpression*)expr;
            printf("Assembler::assembleExpression: INTEGER: PUSH %d\n", intExpr->i);
            m_code.push_back(OPCODE_PUSHI);
            m_code.push_back(intExpr->i);
        } break;

        case EXPR_DOUBLE:
        {
            DoubleExpression* doubleExpr = (DoubleExpression*)expr;
            printf("Assembler::assembleExpression: DOUBLE: PUSH %0.2f\n", doubleExpr->d);
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

Function* Assembler::findFunction(Identifier id)
{
    if (id.identifier.size() > 1)
    {
        // Non-relative id
        Class* clazz = m_runtime->findClass(id.identifier[0]);

        printf("Assembler::findFunction: nexecute: identifier: clazz=%p\n", clazz);
        if (clazz != NULL)
        {
            return clazz->findMethod(id.identifier[1]);
        }
        else
        {
            printf("TODO: Assembler::findFunction: Not found object method id: %s\n", id.toString().c_str());
        }
    }
    else
    {
        printf("TODO: Assembler::findFunction: Not found relative id: %s\n", id.toString().c_str());
    }
    return NULL;
}

