#ifndef __BSCRIPT_STRING_H_
#define __BSCRIPT_STRING_H_

#include <string>
#include <b0rk/class.h>

namespace b0rk
{

class Context;

class StringNative : public NativeObject
{
 private:
    std::string m_string;

 public:
    StringNative(Object* object, std::string str);

    bool addOperator(Context* context, int argCount, Value* args, Value& result);
    bool length(Context* context, int argCount, Value* args, Value& result);
    bool at(Context* context, int argCount, Value* args, Value& result);

    std::string getString() { return m_string; }
};

class String : public Class
{
 private:

 public:
    String();
    ~String();

    bool constructor(Context* context, Object* instance, int argCount, Value* args, Value& result);

    static Object* createString(Context* context, std::string str);
    static std::string getString(Context* context, Value& value);
    static std::string getString(Context* context, Object* obj);
};

};

#endif
