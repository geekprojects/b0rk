#ifndef __BSCRIPT_DISASSEMBLER_H_
#define __BSCRIPT_DISASSEMBLER_H_

#include <b0rk/opcodes.h>
#include <b0rk/assembler.h>

#include <map>

#include <stdint.h>

namespace b0rk
{

struct AssembledCode;

struct OpCodeInfo
{
    std::string name;
    std::string format;
    int args;
};

class Disassembler
{
 private:
    std::map<int, OpCodeInfo> m_opcodes;

    void addOpcode(int opcode, std::string name, std::string format, int args)
    {
        OpCodeInfo info;
        info.name = name;
        info.format = format;
        info.args = args;
         m_opcodes.insert(std::make_pair(opcode, info));
    }

 public:
    Disassembler();
    ~Disassembler();

    bool disassemble(AssembledCode& asmCode);
};

};

#endif
