
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <b0rk/lexer.h>
#include <b0rk/parser.h>
#include <b0rk/assembler.h>
#include <b0rk/executor.h>

using namespace std;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return 0;
    }

    Runtime* runtime = new Runtime();
    Context* context = runtime->createContext();

    const char* className = argv[1];
    Class* clazz = runtime->findClass(context, className);
    if (clazz == NULL)
    {
        return 0;
    }

    Function* mainFunc = clazz->findMethod("main");
    if (mainFunc == NULL)
    {
        return 0;
    }
    if (!mainFunc->getStatic())
    {
        printf("Main is not static!\n");
        return 0;
    }

    mainFunc->execute(context, NULL, 0);

    delete runtime;

    return 0;
}

