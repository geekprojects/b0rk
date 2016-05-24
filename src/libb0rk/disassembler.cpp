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

#include <sys/stat.h>

using namespace std;
using namespace b0rk;

Disassembler::Disassembler()
{
    addOpcode(OPCODE_LOAD_VAR, "LOAD_VAR", "v%lld", 1);
    addOpcode(OPCODE_STORE_VAR, "STORE_VAR", "v%lld", 1);
    addOpcode(OPCODE_LOAD_FIELD,  "LOAD_FIELD", "f%lld", 1);
    addOpcode(OPCODE_LOAD_FIELD_NAMED,  "LOAD_FIELD_NAMED", "<pop>", 0);
    addOpcode(OPCODE_STORE_FIELD_NAMED,  "STORE_FIELD_NAMED", "<pop>", 0);
    addOpcode(OPCODE_LOAD_STATIC_FIELD,  "LOAD_STATIC_FIELD", "class=0x%llx, field=%lld", 2);
    addOpcode(OPCODE_STORE_FIELD, "STORE_FIELD", "f%lld", 1);
    addOpcode(OPCODE_LOAD_ARRAY, "LOAD_ARRAY", "", 0);
    addOpcode(OPCODE_STORE_ARRAY, "STORE_ARRAY", "", 0);
    addOpcode(OPCODE_INC_VAR, "INC_VAR", "v%lld, %lld", 2);
    addOpcode(OPCODE_PUSHI, "PUSHI", "%lld", 1);
    addOpcode(OPCODE_PUSHD, "PUSHD", "0x%llx", 1);
    addOpcode(OPCODE_PUSHOBJ, "PUSHOBJ", "object=0x%llx", 1);
    addOpcode(OPCODE_POP, "POP", "", 0);
    addOpcode(OPCODE_DUP, "DUP", "", 0);
    addOpcode(OPCODE_SWAP, "SWAP", "", 0);
    addOpcode(OPCODE_ADD, "ADD", "", 0);
    addOpcode(OPCODE_SUB, "SUB", "", 0);
    addOpcode(OPCODE_MUL, "MUL", "", 0);
    addOpcode(OPCODE_DIV, "DIV", "", 0);
    addOpcode(OPCODE_AND, "AND", "", 0);
    addOpcode(OPCODE_ADDI, "ADDI", "", 0);
    addOpcode(OPCODE_SUBI, "SUBI", "", 0);
    addOpcode(OPCODE_ADDD, "ADDD", "", 0);
    addOpcode(OPCODE_SUBD, "SUBD", "", 0);
    addOpcode(OPCODE_MULI, "MULI", "", 0);
    addOpcode(OPCODE_MULD, "MULD", "", 0);
    addOpcode(OPCODE_PUSHCE, "PUSHCE", "", 0);
    addOpcode(OPCODE_PUSHCL, "PUSHCL", "", 0);
    addOpcode(OPCODE_PUSHCLE, "PUSHCLE", "", 0);
    addOpcode(OPCODE_PUSHCG, "PUSHCG", "", 0);
    addOpcode(OPCODE_PUSHCGE, "PUSHCGE", "", 0);
    addOpcode(OPCODE_CALL, "CALL", "function=0x%llx, args=%lld", 2);
    addOpcode(OPCODE_CALL_STATIC, "CALL_STATIC", "function=0x%llx, args=%lld", 2);
    addOpcode(OPCODE_CALL_NAMED, "CALL_NAMED", "(%lld args)", 1);
    addOpcode(OPCODE_CALL_OBJ, "CALL_OBJ", "(%lld args)", 1);
    addOpcode(OPCODE_NEW, "NEW", "class=0x%llx, args=%lld", 2);
    addOpcode(OPCODE_NEW_FUNCTION, "NEW_FUNCTION", "function=%llx", 1);
    addOpcode(OPCODE_RETURN, "RETURN", "", 0);
    addOpcode(OPCODE_CMP, "CMP", "", 0);
    addOpcode(OPCODE_CMPI, "CMPI", "", 0);
    addOpcode(OPCODE_CMPD, "CMPD", "", 0);
    addOpcode(OPCODE_JMP, "JMP", "0x%llx", 1);
    addOpcode(OPCODE_BEQ, "BEQ", "0x%llx", 1);
    addOpcode(OPCODE_BNE, "BNE", "0x%llx", 1);
    addOpcode(OPCODE_BL, "BL", "0x%llx", 1);
    addOpcode(OPCODE_BLE, "BLE", "0x%llx", 1);
    addOpcode(OPCODE_BG, "BG", "0x%llx", 1);
    addOpcode(OPCODE_BGE, "BGE", "0x%llx", 1);
    addOpcode(OPCODE_THROW, "THROW", "", 0);
    addOpcode(OPCODE_PUSHTRY, "PUSHTRY", "v%lld, 0x%llx", 2);
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

        switch (info.args)
        {
            case 1:
            {
                uint64_t arg = code.code[pos++];
                fprintf(fp, info.format.c_str(), arg);
            } break;

            case 2:
            {
                uint64_t arg0 = code.code[pos++];
                uint64_t arg1 = code.code[pos++];
                fprintf(fp, info.format.c_str(), arg0, arg1);
            } break;
        }

        fprintf(fp, "\n");

    }

    fclose(fp);
    return true;
}

