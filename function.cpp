
#include <stdio.h>
#include <string.h>

#include "function.h"

using namespace std;

Function::Function(Class* clazz)
{
    m_class = clazz;
    m_static = false;
}

Function::~Function()
{
}

bool Function::execute(Context* context, Object* instance)
{
    return false;
}

NativeFunction::NativeFunction(Class* clazz, nativeFunction_t func)
    : Function(clazz)
{
    m_native = func;
}

bool NativeFunction::execute(Context* context, Object* instance)
{
    printf("NativeFunction::execute: Here!\n");
    return (m_class->*m_native)(context, instance);
}

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

                    Class* stringClass = context->getRuntime()->findClass("String");
                    Object* strObj = new Object(stringClass);
                    strObj->data = (void*)strdup(strExpr->str.c_str());
                    printf("ScriptFunction::execute: Created string: %s\n", (char*)strObj->data);
                    Value v;
                    v.v.object = strObj;
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
    }
    return true;
}


