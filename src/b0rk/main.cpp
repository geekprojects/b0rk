/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include <cinttypes>

#include <b0rk/lexer.h>
#include <b0rk/parser.h>
#include <b0rk/assembler.h>
#include <b0rk/executor.h>
#include <b0rk/utils.h>

#include <geek/core-string.h>

#include "packages/system/lang/Array.h"
#include "packages/system/lang/StringClass.h"

using namespace std;
using namespace b0rk;

static struct option long_options[] =
{
    {"classpath", required_argument, 0, 'c'},
    {"help", no_argument, 0, 'h'},
    {"verbose", optional_argument, 0, 'v'},
    {0, 0, 0, 0}
};

void usage(int status, const char* argv0, const char* message = NULL)
{
    const char* name = strrchr(argv0, '/');
    if (name != NULL)
    {
        name++;
    }
    else
    {
        name = argv0;
    }
 
    if (message != NULL)
    {
        printf("%s: %s\n", name, message);
    }

    printf("usage: %s [options] <class> [arguments...]\n", name);
    printf("\n");
    printf("Options:\n");
    printf("  --classpath <cp>   List of entries to the classpath, separated by :\n");
    printf("  --help             This help text\n");
    printf("  --verbose[=<type>] Enable verbose messages, where type is a comma separated list of:\n");
    printf("                       gc  Garbage Collection statistics\n");
    printf("\n");

    exit(status);
}

int main(int argc, char** argv)
{
    Runtime* runtime = new Runtime();

    // Handle any b0rk arguments
    while (true)
    {
        int c;
        int option_index = 0;
        c = getopt_long (argc, argv, "c:hv:", long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
            case 'c':
            {
                vector<string> cpEntries = Geek::Core::splitString(string(optarg), ':');
                vector<string>::iterator it;
                for (it = cpEntries.begin(); it != cpEntries.end(); it++)
                {
                    wstring cpstr = Utils::string2wstring(*it);
                    runtime->addClasspath(cpstr);
                }
            } break;

            case 'h':
                usage(0, argv[0]);

            case 'v':
            {
                int verboseFlags = VERBOSE_ALL;
                if (optarg != NULL)
                {
                    verboseFlags = 0;

                    vector<string> v = Geek::Core::splitString(string(optarg), ',');
                    vector<string>::iterator it;
                    for (it = v.begin(); it != v.end(); it++)
                    {
                        if (*it == "gc")
                        {
                            verboseFlags |= VERBOSE_GC;
                        }
                        else
                        {
                            fprintf(stderr, "Unknown verbose flag: %s\n", (*it).c_str());
                        }
                    }
                }
                runtime->setVerboseFlags(verboseFlags);

            } break;

            default:
                usage(2, argv[0]);
        }
    }

    if (optind >= argc)
    {
        usage(2, argv[0], "No class specified");
    }

    Context* context = runtime->createContext();

    // Find the specified class
    string className = string(argv[optind++]);
    Class* clazz = runtime->findClass(context, Utils::string2wstring(className));
    if (clazz == NULL)
    {
        printf("%s: Unable to find class %s\n", argv[0], className.c_str());
        return 0;
    }

    // Find the main method...
    Function* mainFunc = clazz->findMethod(L"main");
    if (mainFunc == NULL)
    {
        printf("%s: No main method found in class\n", argv[0]);
        return 3;
    }
    if (!mainFunc->getStatic())
    {
        printf("%s: Main is not static!\n", argv[0]);
        return 3;
    }

    // Store any arguments in an array of Strings
    int argCount = argc - optind;
    Object* argsObj = runtime->newArray(context, argCount);
    int i;
    for (i = optind; i < argc; i++)
    {
        Value idx;
        idx.type = VALUE_INTEGER;
        idx.i = i - optind;
        Value value;
        value.type = VALUE_OBJECT;
        value.object = runtime->newString(context, Utils::string2wstring(string(argv[i])));
        ((Array*)runtime->getArrayClass())->store(context, argsObj, idx, value);
    }

    Value argsValue;
    argsValue.type = VALUE_OBJECT;
    argsValue.object = argsObj;

    // Execute the main method
    Value result;
    bool res;
    res = mainFunc->execute(context, NULL, 1, &argsValue, result);

    // Clean up
    delete runtime;

    if (!res)
    {
        return 1;
    }

    return 0;
}

