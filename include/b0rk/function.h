#ifndef __BSCRIPT_FUNCTION_H_
#define __BSCRIPT_FUNCTION_H_

#include <b0rk/class.h>
#include <b0rk/context.h>
#include <b0rk/expression.h>
#include <b0rk/object.h>
#include <b0rk/assembler.h>

#include <vector>

namespace b0rk
{

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

    bool execute(Context* context, Object* clazz, int argCount, Value* argv, Value& result);
    virtual bool execute(Context* context, Object* clazz, int argCount);
};

typedef bool(Class::*nativeFunction_t)(Context*, Object* instance, int argCount, Value* args, Value& result);

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

};

#endif
