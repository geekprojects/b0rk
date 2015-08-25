
#include "assembler.h"

Assembler::Assembler()
{
}

Assembler::~Assembler()
{
}

bool Assembler::assemble(CodeBlock* code)
{
   unsigned int i;
    for (i = 0; i < code->m_code.size(); i++)
    {
        assembleExpression(code->m_code[i]);
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
m_code.push_back(OPCODE_CALL);

        } break;

        case EXPR_OPER:
        {
            OperationExpression* opExpr = (OperationExpression*)expr;

            res = assembleExpression(opExpr->right);
            if (!res)
            {
                return false;
            }

            res = assembleExpression(opExpr->left);
            if (!res)
            {
                return false;
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

                case OP_SET:
                    printf("Assembler::assembleExpression: OPER: SET\n");
m_code.push_back(OPCODE_SET);
                    break;

                default:
                    printf("Assembler::assembleExpression: OPER: Unhandled operation\n");
                    return false;
            }
        } break;

        case EXPR_VAR:
        {
            VarExpression* varExpr = (VarExpression*)expr;
            printf("Assembler::assembleExpression: VAR: PUSH ADDRESS OF %s\n", varExpr->var.toString().c_str());
            // TODO: Local? Class? Static?
            m_code.push_back(OPCODE_PUSHV);
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

