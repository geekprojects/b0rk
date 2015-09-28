#ifndef __BSCRIPT_CODEBLOCK_H_
#define __BSCRIPT_CODEBLOCK_H_

#include <vector>
#include <string>

#include <b0rk/value.h>

class ScriptFunction;
struct Expression;

struct CodeBlock
{
    ScriptFunction* m_function;
    CodeBlock* m_parent;
    int m_startingVarId;
    int m_maxVarId;

    ValueType* m_varTypes;

    std::vector<std::string> m_vars;
    std::vector<Expression*> m_code;
    std::vector<CodeBlock*> m_childBlocks;

    CodeBlock();
    ~CodeBlock();

    int setStartingVarId(int id);
    int getVarId(std::string var);
    bool setVarType(int id, ValueType type);
    ValueType getVarType(int id);

    std::string toString();
};

#endif
