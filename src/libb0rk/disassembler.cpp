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


#include <b0rk/disassembler.h>
#include <b0rk/function.h>
#include <b0rk/utils.h>
#include <b0rk/object.h>

#include <sys/stat.h>

#include <stdio.h>
#include <wchar.h>

using namespace std;
using namespace b0rk;

Disassembler::Disassembler()
{
}

Disassembler::~Disassembler()
{
}

bool Disassembler::disassemble(AssembledCode& code)
{
    int pos = 0;
    mkdir("asm", 0755);
    string filename = "asm/" + Utils::wstring2string(code.function->getFullName()) + ".asm";
    FILE* fp = fopen(filename.c_str(), "w");
    if (fp == NULL)
    {
        return false;
    }

    fprintf(fp, "%ls:\n", code.function->getFullName().c_str());
    while (pos < code.size)
    {
        int opcodePos = pos++;
        int opcode = code.code[opcodePos];

        OpCodeInfo info = OpCodeInfo::getOpcodeInfo((OpCode)opcode);
        fprintf(fp, "\t0x%04x: %s ", opcodePos, info.name.c_str());

        wstring output = L"";
        unsigned int p = 0;
        int arg = 0;
        for (p = 0; p < info.format.length(); p++)
        {
            char c = info.format[p];
            if (c == '%')
            {
                wchar_t buffer[128];
                p++;
                char c = info.format[p];
                switch (c)
                {
                    case 'd':
                        swprintf(buffer, 32, L"%lld", code.code[pos + arg++]);
                        output += buffer;
                        break;

                    case 'x':
                        swprintf(buffer, 32, L"%llx", code.code[pos + arg++]);
                        output += buffer;
                        break;

                    case 'f':
                    {
	                uint64_t di = code.code[pos + arg++];
                        double* dp = (double*)&di;
                        swprintf(buffer, 32, L"%0.5f", *dp);
                        output += buffer;
                    } break;

                    case 'C':
                    {
                        Class* cls = (Class*)code.code[pos + arg++];
                        output += cls->getName();
                    } break;

                    case 'F':
                    {
                        Function* f = (Function*)code.code[pos + arg++];
                        output += f->getFullName();
                    } break;

                    case 'O':
                    {
                        Object* object = (Object*)code.code[pos + arg++];
                        swprintf(buffer, 128, L"{OBJECT:%ls:%p}", object->getClass()->getName().c_str(), object);
                        output += buffer;
                    } break;
                }
            }
            else
            {
                output += c;
            }
        }
        fprintf(fp, "%ls\n", output.c_str());
        pos += info.args;
    }

    fclose(fp);
    return true;
}

