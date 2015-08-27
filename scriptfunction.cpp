
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "function.h"
#include "executor.h"

using namespace std;

ScriptFunction::ScriptFunction(Class* clazz)
    : Function(clazz)
{
    m_code = NULL;
    m_assembled = false;
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
    if (m_assembled == false)
    {
        Assembler assembler(context->getRuntime());
        bool res = assembler.assemble(this, m_asmCode);
        if (!res)
        {
            return false;
        }
        m_assembled = true;
    }

    Executor* executor = context->getRuntime()->getExecutor();
    return executor->run(context, m_asmCode);
}

