
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "function.h"
#include "string.h"

using namespace std;

ScriptFunction::ScriptFunction(Class* clazz)
    : Function(clazz)
{
    m_code = NULL;
}

ScriptFunction::ScriptFunction(Class* clazz, CodeBlock* code)
    : Function(clazz)
{
    m_code = code;
}

ScriptFunction::~ScriptFunction()
{
}

void ScriptFunction::setCode(CodeBlock* code)
{
    m_code = code;
}

#define DOUBLE_VALUE(_v) (((_v).type == VALUE_DOUBLE) ? (_v.d) : (double)(_v.i))
#define INTEGER_VALUE(_v) (((_v).type == VALUE_INTEGER) ? (_v.i) : (int)(floor(_v.d)))

bool ScriptFunction::executeExpression(Context* context, Expression* expr)
{
    bool res;
        printf("ScriptFunction::execute: expression: %s\n", expr->toString().c_str());

        if (expr->type == EXPR_CALL)
        {
            CallExpression* callExpr = (CallExpression*)expr;

            vector<Value> args;
            vector<Expression*>::iterator paramIt;
            for (paramIt = callExpr->parameters.begin(); paramIt != callExpr->parameters.end(); paramIt++)
            {
                Expression* paramExpr = *paramIt;
                if (paramExpr->type == EXPR_STRING)
                {
                    StringExpression* strExpr = (StringExpression*)paramExpr;

                    Object* strObj = String::createString(context, strExpr->str.c_str());

                    Value v;
                    v.object = strObj;
                    args.push_back(v);
                    context->push(v);
                }
                else
                {
                    printf("ScriptFunction::execute: Unhandled parameter expression: %d\n", paramExpr->type);
                    return false;
                }
            }

            if (callExpr->function.identifier.size() > 1)
            {
                Class* clazz = context->getRuntime()->findClass(callExpr->function.identifier[0]);
                printf("ScriptFunction::execute: identifier: clazz=%p\n", clazz);
                if (clazz != NULL)
                {
                    Function* function = clazz->findMethod(callExpr->function.identifier[1]);
                    printf("ScriptFunction::execute: identifier: clazz function=%p\n", function);

                    function->execute(context, NULL);
                }
                else
                {
                    // Look for a global variable
                }
            }
        }
    else if (expr->type == EXPR_OPER)
    {
        OperationExpression* opExpr = (OperationExpression*)expr;
        printf("ScriptFunction::execute: Operation: %d\n", opExpr->operType);

        if (opExpr->operType != OP_SET)
        {
            res = executeExpression(context, opExpr->left);
            if (!res)
            {
                return false;
            }
        }

        res = executeExpression(context, opExpr->right);
        if (!res)
        {
            return false;
        }

        switch (opExpr->operType)
        {
            case OP_NONE:
                printf("ScriptFunction::execute: NONE: Shouldn't happen!\n");
                return false;

            case OP_SET:
            {
                Value value = context->pop();
                printf("ScriptFunction::execute: EQUALS: value=%s\n", value.toString().c_str());
                if (opExpr->left->type != EXPR_VAR)
                {
                    printf("ScriptFunction::execute: EQUALS: Left hand side is not a variable!\n");
                    return false;
                }

                VarExpression* varExpr = (VarExpression*)opExpr->left;
Value* var = findVariable(context, varExpr->var);
                printf("ScriptFunction::execute: EQUALS: variable=%s\n", varExpr->var.toString().c_str());
                printf("ScriptFunction::execute: EQUALS: var value=%p\n", var);
            } break;

            case OP_ADD:
            {
                Value rightRes = context->pop();
                Value leftRes = context->pop();

                printf("ScriptFunction::execute: ADD: left=%s, right=%s\n", leftRes.toString().c_str(), rightRes.toString().c_str());
                if (leftRes.type == VALUE_DOUBLE || rightRes.type == VALUE_DOUBLE)
                {
                    double l = DOUBLE_VALUE(leftRes);
                    double r = DOUBLE_VALUE(rightRes);
                    printf("ScriptFunction::execute: ADD: DOUBLE: %0.2f + %0.2f\n", l, r);
                    Value result;
                    result.type = VALUE_DOUBLE;
                    result.d = l + r;
                    context->push(result);
                }
                else
                {
                    printf("ScriptFunction::execute: ADD: INTEGER: %lld + %lld\n", leftRes.i, rightRes.i);
                    Value result;
                    result.type = VALUE_INTEGER;
                    result.i = leftRes.i + rightRes.i;
                    context->push(result);
                }

            } break;

            case OP_SUB:
            {
                Value rightRes = context->pop();
                Value leftRes = context->pop();
                printf("ScriptFunction::execute: MINUS: left=%s, right=%s\n", leftRes.toString().c_str(), rightRes.toString().c_str());
            } break;
default:
                printf("ScriptFunction::execute: Unhandled op!\n");
return false;
        }

    }
    else if (expr->type == EXPR_INTEGER)
    {
        Value result;
        result.type = VALUE_INTEGER;
        result.i = ((IntegerExpression*)expr)->i;
        context->push(result);
    }
    else if (expr->type == EXPR_DOUBLE)
    {
        Value result;
        result.type = VALUE_DOUBLE;
        result.d = ((DoubleExpression*)expr)->d;
        context->push(result);
    }
    else
    {
        printf("ScriptFunction::execute: Unknown expression type: %d\n", expr->type);
        return false;
    }
    return true;
}

bool ScriptFunction::execute(Context* context, Object* instance)
{
    return execute(context, instance, m_code);
}

bool ScriptFunction::execute(Context* context, Object* instance, CodeBlock* code)
{
    unsigned int i;
    for (i = 0; i < code->m_code.size(); i++)
    {
        Expression* expr = code->m_code[i];
        executeExpression(context, expr);
    }
    return true;
}

Value* ScriptFunction::findVariable(Context* context, Identifier id)
{
printf("ScriptFunction::findVariable: Looking for var: %s\n", id.toString().c_str());

return NULL;
}

