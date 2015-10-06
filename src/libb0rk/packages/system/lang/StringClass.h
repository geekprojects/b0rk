#ifndef __BSCRIPT_STRING_H_
#define __BSCRIPT_STRING_H_

#include <string>
#include <b0rk/class.h>

namespace b0rk
{

class Context;

class String : public Class
{
 private:

 public:
    String();
    ~String();

    bool constructor(Context* context, Object* instance, int argCount, Value* args, Value& result);
    bool addOperator(Context* context, Object* instance, int argCount, Value* args, Value& result);
    bool length(Context* context, Object* instance, int argCount, Value* args, Value& result);
    bool at(Context* context, Object* instance, int argCount, Value* args, Value& result);

    static Object* createString(Context* context, const char* str);
    static std::string getString(Context* context, Object* obj);
};

};

#endif
