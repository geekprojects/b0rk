#ifndef __BSCRIPT_OPCODES_H_
#define __BSCRIPT_OPCODES_H_

enum OpCode
{
    OPCODE_LOAD        = 0x0100,
    OPCODE_STORE       = 0x0101,

    OPCODE_ADD         = 0x0200,
    OPCODE_SUB         = 0x0201,

    OPCODE_PUSHI       = 0x0300,
    OPCODE_PUSHD       = 0x0301,
    OPCODE_PUSHVAR     = 0x0302,
    OPCODE_PUSHCL      = 0x0310, // Push condition: Less than

    OPCODE_CALL        = 0x0400,
    OPCODE_CALL_STATIC = 0x0401,
    OPCODE_NEW         = 0x0402,
    OPCODE_NEW_STRING  = 0x0403,

    OPCODE_CMP         = 0x0500,

    OPCODE_JMP         = 0x0600,
    OPCODE_BEQ         = 0x0601,
    OPCODE_BNE         = 0x0602
};

#endif
