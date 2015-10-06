#ifndef __B0RK_SYSTEM_LANG_FUNCTION_H_
#define __B0RK_SYSTEM_LANG_FUNCTION_H_

#include <b0rk/class.h>

namespace b0rk
{

class FunctionClass : public Class
{
 private:

 public:
    FunctionClass();
    ~FunctionClass();

    bool constructor(Context* context, Object* instance, int argCount, Value* args, Value& result);
};

};

#endif
