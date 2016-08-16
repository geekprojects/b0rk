/*
 * b0rk - The b0rk Embeddable Runtime Environment
 * Copyright (C) 2015, 2016 GeekProjects.com
 *
 * This file is part of b0rk.
 *
 * b0rk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * b0rk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BSCRIPT_OPCODES_H_
#define __BSCRIPT_OPCODES_H_

#include <b0rk/value.h>

#include <string>

namespace b0rk
{

/*
 *    gggg tttt nnnn
 *
 * g: Group
 *   0x0 None
 *   0x1 Variables
 *   0x2 Arithmetic
 *   0x3 Stack
 *   0x4 Flags
 *   0x5 Calls
 *   0x6 Comparison
 *   0x7 Jumps
 */

enum OpCodeType
{
    OPCODE_TYPE_NONE = 0x000,
    OPCODE_TYPE_VAR = 0x100,
    OPCODE_TYPE_ARITHMETIC = 0x200,
    OPCODE_TYPE_STACK = 0x300,
    OPCODE_TYPE_FLAGS = 0x400,
    OPCODE_TYPE_CALLS = 0x500,
    OPCODE_TYPE_COMPARISON = 0x600,
    OPCODE_TYPE_JUMPS = 0x700,
};

#define OPCODE_TYPE(_op) (((int)(_op)) & 0xf00)
#define OPCODE_IS_VAR(_op) (OPCODE_TYPE(_op) == OPCODE_TYPE_VAR)
#define OPCODE_IS_ARITHMETIC(_op) (OPCODE_TYPE(_op) == OPCODE_TYPE_ARITHMETIC)
#define OPCODE_IS_STACK(_op) (OPCODE_TYPE(_op) == OPCODE_TYPE_STACK)

enum OpCode
{
    OPCODE_NOP                = 0x00,

    OPCODE_LOAD_VAR           = 0x101,
    OPCODE_STORE_VAR          = 0x102,
    OPCODE_LOAD_FIELD         = 0x103,
    OPCODE_STORE_FIELD        = 0x104,
    OPCODE_LOAD_FIELD_NAMED   = 0x105,
    OPCODE_STORE_FIELD_NAMED  = 0x106,
    OPCODE_LOAD_STATIC_FIELD  = 0x107,
    OPCODE_STORE_STATIC_FIELD = 0x108,
    OPCODE_LOAD_ARRAY         = 0x109,
    OPCODE_STORE_ARRAY        = 0x10a,
    OPCODE_INC_VAR            = 0x10b,
    OPCODE_INC_VARD           = 0x12b,

    // Arithmetic: Any value types (Objects etc)
    OPCODE_ADD                = 0x200,
    OPCODE_SUB                = 0x201,
    OPCODE_MUL                = 0x202,
    OPCODE_DIV                = 0x203,
    OPCODE_AND                = 0x204,
    OPCODE_NOT                = 0x205,

    // Arithmetic: Integers
    OPCODE_ADDI               = 0x210,
    OPCODE_SUBI               = 0x211,
    OPCODE_MULI               = 0x212,
    OPCODE_DIVI               = 0x213,
    OPCODE_ANDI               = 0x214,
    OPCODE_NOTI               = 0x215,

    // Arithmetic: Doubles
    OPCODE_ADDD               = 0x220,
    OPCODE_SUBD               = 0x221,
    OPCODE_MULD               = 0x222,
    OPCODE_DIVD               = 0x223,

    // Stack Operations
    OPCODE_PUSHVAR            = 0x302,
    OPCODE_PUSHOBJ            = 0x303,
    OPCODE_DUP                = 0x304,
    OPCODE_POP                = 0x305,
    OPCODE_SWAP               = 0x306,
    OPCODE_PUSHI              = 0x310,
    OPCODE_PUSHD              = 0x320,

    OPCODE_PUSHCE             = 0x410, // Push condition: Equals
    OPCODE_PUSHCNE            = 0x411, // Push condition: Not Equals
    OPCODE_PUSHCL             = 0x412, // Push condition: Less than
    OPCODE_PUSHCLE            = 0x413, // Push condition: Less than or equal
    OPCODE_PUSHCG             = 0x414, // Push condition: Greater than
    OPCODE_PUSHCGE            = 0x415, // Push condition: Greater than or equal

    OPCODE_CALL               = 0x500,
    OPCODE_CALL_STATIC        = 0x501,
    OPCODE_CALL_NAMED         = 0x502,
    OPCODE_CALL_OBJ           = 0x503,
    OPCODE_NEW                = 0x504,
    OPCODE_NEW_FUNCTION       = 0x506,
    OPCODE_RETURN             = 0x507,

    OPCODE_CMP                = 0x600,
    OPCODE_CMPI               = 0x601,
    OPCODE_CMPD               = 0x602,

    OPCODE_JMP                = 0x700,
    OPCODE_BEQ                = 0x701,
    OPCODE_BNE                = 0x702,
    OPCODE_BL                 = 0x703,
    OPCODE_BLE                = 0x704,
    OPCODE_BG                 = 0x705,
    OPCODE_BGE                = 0x706,

    OPCODE_THROW               = 0x800,
    OPCODE_PUSHTRY             = 0x801,
    OPCODE_POPTRY              = 0x802,

    OPCODE_MAX                 = 0x803
};

struct OpCodeInfo
{
    std::string name;
    std::string format;
    int args;
    int address;

    OpCodeInfo()
    {
    }

    OpCodeInfo(std::string _name, std::string _format, int _args, int _address)
    {
        name = _name;
        format = _format;
        args = _args;
        address = _address;
    }

    static OpCodeInfo g_opCodeInfo[OPCODE_MAX];
    static void addOpcode(OpCode op, std::string name, std::string format, int args, int address = -1);
    static void init();
    static OpCodeInfo getOpcodeInfo(OpCode op);
};


};

#endif
