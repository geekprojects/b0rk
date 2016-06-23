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
    addOpcode(OPCODE_LOAD_VAR, "LOAD_VAR", "v%d", 1);
    addOpcode(OPCODE_STORE_VAR, "STORE_VAR", "v%d", 1);
    addOpcode(OPCODE_LOAD_FIELD,  "LOAD_FIELD", "f%d", 1);
    addOpcode(OPCODE_STORE_FIELD, "STORE_FIELD", "f%d", 1);
    addOpcode(OPCODE_LOAD_FIELD_NAMED,  "LOAD_FIELD_NAMED", "<pop>", 0);
    addOpcode(OPCODE_STORE_FIELD_NAMED,  "STORE_FIELD_NAMED", "<pop>", 0);
    addOpcode(OPCODE_LOAD_STATIC_FIELD,  "LOAD_STATIC_FIELD", "class=%C, field=%d", 2);
    addOpcode(OPCODE_STORE_STATIC_FIELD,  "STORE_STATIC_FIELD", "class=%C, field=%d", 2);
    addOpcode(OPCODE_LOAD_ARRAY, "LOAD_ARRAY", "", 0);
    addOpcode(OPCODE_STORE_ARRAY, "STORE_ARRAY", "", 0);
    addOpcode(OPCODE_INC_VAR, "INC_VAR", "v%d, %d", 2);
    addOpcode(OPCODE_PUSHI, "PUSHI", "%d", 1);
    addOpcode(OPCODE_PUSHD, "PUSHD", "0x%x", 1);
    addOpcode(OPCODE_PUSHOBJ, "PUSHOBJ", "object=%O", 1);
    addOpcode(OPCODE_POP, "POP", "", 0);
    addOpcode(OPCODE_DUP, "DUP", "", 0);
    addOpcode(OPCODE_SWAP, "SWAP", "", 0);
    addOpcode(OPCODE_ADD, "ADD", "", 0);
    addOpcode(OPCODE_SUB, "SUB", "", 0);
    addOpcode(OPCODE_MUL, "MUL", "", 0);
    addOpcode(OPCODE_DIV, "DIV", "", 0);
    addOpcode(OPCODE_AND, "AND", "", 0);
    addOpcode(OPCODE_NOT, "NOT", "", 0);
    addOpcode(OPCODE_ADDI, "ADDI", "", 0);
    addOpcode(OPCODE_SUBI, "SUBI", "", 0);
    addOpcode(OPCODE_ADDD, "ADDD", "", 0);
    addOpcode(OPCODE_SUBD, "SUBD", "", 0);
    addOpcode(OPCODE_MULI, "MULI", "", 0);
    addOpcode(OPCODE_ANDI, "ANDI", "", 0);
    addOpcode(OPCODE_NOTI, "NOTI", "", 0);
    addOpcode(OPCODE_MULD, "MULD", "", 0);
    addOpcode(OPCODE_PUSHCE, "PUSHCE", "", 0);
    addOpcode(OPCODE_PUSHCL, "PUSHCL", "", 0);
    addOpcode(OPCODE_PUSHCLE, "PUSHCLE", "", 0);
    addOpcode(OPCODE_PUSHCG, "PUSHCG", "", 0);
    addOpcode(OPCODE_PUSHCGE, "PUSHCGE", "", 0);
    addOpcode(OPCODE_CALL, "CALL", "function=%F, args=%d", 2);
    addOpcode(OPCODE_CALL_STATIC, "CALL_STATIC", "function=%F, args=%d", 2);
    addOpcode(OPCODE_CALL_NAMED, "CALL_NAMED", "(%d args)", 1);
    addOpcode(OPCODE_CALL_OBJ, "CALL_OBJ", "(%d args)", 1);
    addOpcode(OPCODE_NEW, "NEW", "class=%C, args=%d", 2);
    addOpcode(OPCODE_NEW_FUNCTION, "NEW_FUNCTION", "function=%x", 1);
    addOpcode(OPCODE_RETURN, "RETURN", "", 0);
    addOpcode(OPCODE_CMP, "CMP", "", 0);
    addOpcode(OPCODE_CMPI, "CMPI", "", 0);
    addOpcode(OPCODE_CMPD, "CMPD", "", 0);
    addOpcode(OPCODE_JMP, "JMP", "0x%x", 1);
    addOpcode(OPCODE_BEQ, "BEQ", "0x%x", 1);
    addOpcode(OPCODE_BNE, "BNE", "0x%x", 1);
    addOpcode(OPCODE_BL, "BL", "0x%x", 1);
    addOpcode(OPCODE_BLE, "BLE", "0x%x", 1);
    addOpcode(OPCODE_BG, "BG", "0x%x", 1);
    addOpcode(OPCODE_BGE, "BGE", "0x%x", 1);
    addOpcode(OPCODE_THROW, "THROW", "", 0);
    addOpcode(OPCODE_PUSHTRY, "PUSHTRY", "v%d, 0x%x", 2);
    addOpcode(OPCODE_POPTRY, "POPTRY", "", 0);
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
        map<int, OpCodeInfo>::iterator it = m_opcodes.find(opcode);
        if (it == m_opcodes.end())
        {
            fprintf(fp, "Disassembler::disassemble: 0x%x: Unknown opcode: 0x%x\n", opcodePos, opcode);
            break;
        }

        OpCodeInfo info = it->second;
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

