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

    // Arithmetic: Any value types (Objects etc)
    OPCODE_ADD         = 0x10,
    OPCODE_SUB         = 0x11,
    OPCODE_MUL         = 0x12,
    OPCODE_AND         = 0x13,

    // Arithmetic: Integers
    OPCODE_ADDI        = 0x20,
    OPCODE_SUBI        = 0x21,
    OPCODE_MULI        = 0x22,
    OPCODE_ANDI        = 0x23,

    // Arithmetic: Doubles
    OPCODE_ADDD        = 0x30,
    OPCODE_SUBD        = 0x31,
    OPCODE_MULD        = 0x32,

    OPCODE_PUSHI       = 0x40,
    OPCODE_PUSHD       = 0x41,
    OPCODE_PUSHVAR     = 0x42,
    OPCODE_DUP         = 0x43,
    OPCODE_PUSHCE      = 0x44, // Push condition: Equals
    OPCODE_PUSHCL      = 0x45, // Push condition: Less than
    OPCODE_PUSHCLE     = 0x46, // Push condition: Less than or equal
    OPCODE_PUSHCG      = 0x47, // Push condition: Greater than
    OPCODE_PUSHCGE     = 0x48, // Push condition: Greater than or equal
    OPCODE_POP         = 0x49,

    OPCODE_CALL        = 0x50,
    OPCODE_CALL_STATIC = 0x51,
    OPCODE_CALL_NAMED  = 0x52,
    OPCODE_CALL_OBJ    = 0x53,
    OPCODE_NEW         = 0x54,
    OPCODE_NEW_STRING  = 0x55,
    OPCODE_NEW_FUNCTION= 0x56,
    OPCODE_RETURN      = 0x57,

    OPCODE_CMP         = 0x60,
    OPCODE_CMPI        = 0x61,
    OPCODE_CMPD        = 0x62,

    OPCODE_JMP         = 0x70,
    OPCODE_BEQ         = 0x71,
    OPCODE_BNE         = 0x72,
    OPCODE_BL          = 0x73,
    OPCODE_BLE         = 0x74,
    OPCODE_BG          = 0x75,
    OPCODE_BGE         = 0x76,

    OPCODE_MAX         = 0x77
};

};

#endif
