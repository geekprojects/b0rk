
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <b0rk/function.h>
#include <b0rk/executor.h>

using namespace std;

ScriptFunction::ScriptFunction(Class* clazz, vector<string> args)
    : Function(clazz, args)
{
    m_code = NULL;
    m_assembled = false;
}

ScriptFunction::ScriptFunction(Class* clazz, CodeBlock* code, vector<string> args)
    : Function(clazz, args)
{
    m_code = code;
}

ScriptFunction::~ScriptFunction()
{
    delete m_code;

    if (m_asmCode.code != NULL)
    {
        delete m_asmCode.code;
    }
}

void ScriptFunction::setCode(CodeBlock* code)
{
    m_code = code;
}

bool ScriptFunction::execute(Context* context, Object* instance, int argCount)
{
    if (m_assembled == false)
    {
        Assembler assembler(context);
        bool res = assembler.assemble(this, m_asmCode);
        if (!res)
        {
            return false;
        }
        m_assembled = true;
    }

    Executor* executor = context->getRuntime()->getExecutor();

    return executor->run(context, instance, m_asmCode, argCount);
}

