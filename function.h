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
class Object;

class Function
{
 protected:
    bool m_static;
    Class* m_class;

 public:
    Function(Class* clazz);
    virtual ~Function();

    void setStatic(bool isStatic) { m_static = isStatic; }
    bool getStatic() { return m_static; }

    virtual bool execute(Context* context, Object* clazz);
};

typedef bool(Class::*nativeFunction_t)(Context*, Object* instance);

class NativeFunction : public Function
{
 private:
    nativeFunction_t m_native;

 public:
    NativeFunction(Class* clazz, nativeFunction_t func);

    virtual bool execute(Context* context, Object* instance);
};

class ScriptFunction : public Function
{
 private:
    CodeBlock* m_code;

    bool m_assembled;
    AssembledCode m_asmCode;

 public:
    ScriptFunction(Class* clazz);
    ScriptFunction(Class* clazz, CodeBlock* code);
    ~ScriptFunction();

    void setCode(CodeBlock* code);
    CodeBlock* getCode() { return m_code; }

    virtual bool execute(Context* context, Object* instance);
};

#endif
