#include <b0rk/context.h>
#include <b0rk/compiler.h>

using namespace std;
using namespace b0rk;

Context::Context(Runtime* runtime)
{
    m_runtime = runtime;

    m_stackPos = 0;
    m_stackSize = 1024;
    m_stack = new Value[m_stackSize];
}

Context::~Context()
{
    delete[] m_stack;
}

string spaces(int c)
{
    string str = "";
    int i;
    for (i = 0; i < c; i++)
    {
        str += "=";
    }
    return str;
}

void Context::push(Value value)
{
#ifdef DEBUG_STACK
    printf("Context::push: %s%s\n", spaces(m_stack.size()).c_str(), value.toString().c_str());
#endif
    if (B0RK_UNLIKELY(m_stackPos >= m_stackSize))
    {
        fprintf(stderr, "Context::push: STACK OVERFLOW\n");
        return;
    }
    m_stack[m_stackPos++] = value;
}

void Context::pushVoid()
{
    Value voidValue;
    voidValue.type = VALUE_VOID;
    push(voidValue);
}

Value Context::pop()
{
    if (B0RK_UNLIKELY(m_stackPos <= 0))
    {
        Value voidValue;
        voidValue.type = VALUE_VOID;
        fprintf(stderr, "Context::push: STACK UNDERFLOW\n");
        return voidValue;
    }

    Value value = m_stack[--m_stackPos];
#ifdef DEBUG_STACK
    printf("Context::pop : %s%s\n", spaces(m_stack.size()).c_str(), value.toString().c_str());
#endif

    return value;
}

