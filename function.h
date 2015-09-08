#ifndef __BSCRIPT_FUNCTION_H_
#define __BSCRIPT_FUNCTION_H_

#include "class.h"
#include "context.h"
#include "expression.h"
#include "object.h"
#include "assembler.h"

#include <vector>

class Context;
class Class;
struct Object;

class Function
{
 protected:
    bool m_static;
    Class* m_class;
    std::vector<std::string> m_args;

 public:
    Function(Class* clazz);
    Function(Class* clazz, std::vector<std::string> args);
    virtual ~Function();

    Class* getClass() { return m_class; }

    void setStatic(bool isStatic) { m_static = isStatic; }
    bool getStatic() { return m_static; }

    int getArgId(std::string arg);

    virtual bool execute(Context* context, Object* clazz, int argCount);
};

typedef bool(Class::*nativeFunction_t)(Context*, Object* instance, int argCount);

class NativeFunction : public Function
{
 private:
    nativeFunction_t m_native;

 public:
    NativeFunction(Class* clazz, nativeFunction_t func);

    virtual bool execute(Context* context, Object* instance, int argCount);
};

class ScriptFunction : public Function
{
 private:
    CodeBlock* m_code;

    bool m_assembled;
    AssembledCode m_asmCode;

 public:
    ScriptFunction(Class* clazz, std::vector<std::string> args);
    ScriptFunction(Class* clazz, CodeBlock* code, std::vector<std::string> args);
    ~ScriptFunction();

    void setCode(CodeBlock* code);
    CodeBlock* getCode() { return m_code; }


    virtual bool execute(Context* context, Object* instance, int argCount);
};

#endif
