
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <b0rk/lexer.h>
#include <b0rk/parser.h>
#include <b0rk/assembler.h>
#include <b0rk/executor.h>

using namespace std;
using namespace b0rk;

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
        printf("%s: Unable to find class %s\n", argv[0], className);
        return 0;
    }

    Function* mainFunc = clazz->findMethod("main");
    if (mainFunc == NULL)
    {
        printf("%s: No main method found in class\n", argv[0]);
        return 0;
    }
    if (!mainFunc->getStatic())
    {
        printf("%s: Main is not static!\n", argv[0]);
        return 0;
    }

    mainFunc->execute(context, NULL, 0);

    delete runtime;

    return 0;
}

