
#include "context.h"

using namespace std;

Context::Context(Runtime* runtime)
{
    m_runtime = runtime;
}

Context::~Context()
{
}

void Context::push(Value value)
{
    m_stack.push_back(value);
}

Value Context::pop()
{
    Value v = m_stack.back();
    m_stack.pop_back();
    return v;
}

