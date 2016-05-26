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

namespace b0rk
{

enum OpCode
{
    OPCODE_NOP                = 0x00,

    OPCODE_LOAD_VAR           = 0x01,
    OPCODE_STORE_VAR          = 0x02,
    OPCODE_LOAD_FIELD         = 0x03,
    OPCODE_STORE_FIELD        = 0x04,
    OPCODE_LOAD_FIELD_NAMED   = 0x05,
    OPCODE_STORE_FIELD_NAMED  = 0x06,
    OPCODE_LOAD_STATIC_FIELD  = 0x07,
    OPCODE_STORE_STATIC_FIELD = 0x08,
    OPCODE_LOAD_ARRAY         = 0x09,
    OPCODE_STORE_ARRAY        = 0x0a,
    OPCODE_INC_VAR            = 0x0b,

    // Arithmetic: Any value types (Objects etc)
    OPCODE_ADD         = 0x10,
    OPCODE_SUB         = 0x11,
    OPCODE_MUL         = 0x12,
    OPCODE_DIV         = 0x13,
    OPCODE_AND         = 0x14,
    OPCODE_NOT         = 0x15,

    // Arithmetic: Integers
    OPCODE_ADDI        = 0x20,
    OPCODE_SUBI        = 0x21,
    OPCODE_MULI        = 0x22,
    OPCODE_DIVI        = 0x23,
    OPCODE_ANDI        = 0x24,
    OPCODE_NOTI        = 0x25,

    // Arithmetic: Doubles
    OPCODE_ADDD        = 0x30,
    OPCODE_SUBD        = 0x31,
    OPCODE_MULD        = 0x32,
    OPCODE_DIVD        = 0x33,

    OPCODE_PUSHI       = 0x40,
    OPCODE_PUSHD       = 0x41,
    OPCODE_PUSHVAR     = 0x42,
    OPCODE_PUSHOBJ     = 0x43,
    OPCODE_DUP         = 0x44,
    OPCODE_POP         = 0x45,
    OPCODE_SWAP        = 0x46,

    OPCODE_PUSHCE      = 0x50, // Push condition: Equals
    OPCODE_PUSHCNE     = 0x51, // Push condition: Not Equals
    OPCODE_PUSHCL      = 0x52, // Push condition: Less than
    OPCODE_PUSHCLE     = 0x53, // Push condition: Less than or equal
    OPCODE_PUSHCG      = 0x54, // Push condition: Greater than
    OPCODE_PUSHCGE     = 0x55, // Push condition: Greater than or equal

    OPCODE_CALL        = 0x60,
    OPCODE_CALL_STATIC = 0x61,
    OPCODE_CALL_NAMED  = 0x62,
    OPCODE_CALL_OBJ    = 0x63,
    OPCODE_NEW         = 0x64,
    OPCODE_NEW_FUNCTION= 0x66,
    OPCODE_RETURN      = 0x67,

    OPCODE_CMP         = 0x70,
    OPCODE_CMPI        = 0x71,
    OPCODE_CMPD        = 0x72,

    OPCODE_JMP         = 0x80,
    OPCODE_BEQ         = 0x81,
    OPCODE_BNE         = 0x82,
    OPCODE_BL          = 0x83,
    OPCODE_BLE         = 0x84,
    OPCODE_BG          = 0x85,
    OPCODE_BGE         = 0x86,

    OPCODE_THROW       = 0x90,
    OPCODE_PUSHTRY     = 0x91,
    OPCODE_POPTRY      = 0x92,

    OPCODE_MAX         = 0x92
};

};

#endif
