
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "parser.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        return 0;
    }

    FILE* fp = fopen(argv[1], "r");
    fseek(fp, 0, SEEK_END);
    size_t length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = new char[length + 128];
    fread(buffer, 1, length, fp);
    fclose(fp);

    Runtime* runtime = new Runtime();
    bool res;

    Lexer lexer;
    res = lexer.lexer(buffer, length);
if (!res)
{
return 0;
}

    Parser parser;
    res = parser.parse(runtime, lexer.getTokens());

    delete[] buffer;
if (!res)
{
return 0;
}

const char* className = argv[2];

    Class* clazz = runtime->findClass(className);
    printf("main: %s = %p\n", className, clazz);
    if (clazz == NULL)
    {
        return 0;
    }

    Function* mainFunc = clazz->findMethod("main");
    printf("main: main Func=%p\n", mainFunc);
    if (mainFunc == NULL)
    {
        return 0;
    }
    if (!mainFunc->getStatic())
    {
        printf("Main is not static!\n");
        return 0;
    }

    Context* context = runtime->createContext();
    mainFunc->execute(context, NULL);

    return 0;
}

