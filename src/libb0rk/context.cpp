#include <b0rk/context.h>

using namespace std;
using namespace b0rk;

Context::Context(Runtime* runtime)
{
    m_runtime = runtime;
}

Context::~Context()
{
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
    m_stack.push_back(value);
}

void Context::pushVoid()
{
    Value voidValue;
    voidValue.type = VALUE_VOID;
    push(voidValue);
}

Value Context::pop()
{
    Value value = m_stack.back();
    m_stack.pop_back();
#ifdef DEBUG_STACK
    printf("Context::pop : %s%s\n", spaces(m_stack.size()).c_str(), value.toString().c_str());
#endif
    return value;
}

