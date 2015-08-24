
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "parser.h"

using namespace std;

int main(int argc, char** argv)
{
    if (argc < 2)
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

    Lexer lexer;
    lexer.lexer(buffer, length);

    Parser parser;
    parser.parse(runtime, lexer.getTokens());

    delete[] buffer;

    Class* helloWorld = runtime->findClass("HelloWorld");
    printf("helloWorld=%p\n", helloWorld);

    Function* mainFunc = helloWorld->findMethod("main");
    printf("mainFunc=%p\n", mainFunc);
    if (!mainFunc->getStatic())
    {
        printf("Main is not static!\n");
        return 0;
    }

    Context* context = runtime->createContext();

    mainFunc->execute(context, NULL);

    return 0;
}

