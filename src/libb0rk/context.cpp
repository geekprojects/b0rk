#include <b0rk/context.h>
#include <b0rk/compiler.h>

using namespace std;
using namespace b0rk;

Context::Context(Runtime* runtime)
    : m_assembler(this)
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

