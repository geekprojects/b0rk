#ifndef __BSCRIPT_OPCODES_H_
#define __BSCRIPT_OPCODES_H_

enum OpCode
{
    OPCODE_LOAD        = 0x0100,
    OPCODE_STORE       = 0x0101,

    // Arithmetic: Any value types (Objects etc)
    OPCODE_ADD         = 0x0200,
    OPCODE_SUB         = 0x0201,
    OPCODE_MUL         = 0x0202,
    OPCODE_AND         = 0x020a,

    // Arithmetic: Integers
    OPCODE_ADDI        = 0x0210,
    OPCODE_SUBI        = 0x0211,
    OPCODE_MULI        = 0x0212,
    OPCODE_ANDI        = 0x021a,

    // Arithmetic: Doubles
    OPCODE_ADDD        = 0x0220,
    OPCODE_SUBD        = 0x0221,
    OPCODE_MULD        = 0x0222,

    OPCODE_PUSHI       = 0x0300,
    OPCODE_PUSHD       = 0x0301,
    OPCODE_PUSHVAR     = 0x0302,
    OPCODE_PUSHCL      = 0x0310, // Push condition: Less than

    OPCODE_CALL        = 0x0400,
    OPCODE_CALL_STATIC = 0x0401,
    OPCODE_CALL_NAMED  = 0x0402,
    OPCODE_NEW         = 0x0410,
    OPCODE_NEW_STRING  = 0x0411,
    OPCODE_RETURN      = 0x0420,

    OPCODE_CMP         = 0x0500,
    OPCODE_CMPI        = 0x0510,
    OPCODE_CMPD        = 0x0520,

    OPCODE_JMP         = 0x0600,
    OPCODE_BEQ         = 0x0601,
    OPCODE_BNE         = 0x0602
};

#endif
