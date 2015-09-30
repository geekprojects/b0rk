#ifndef __B0RK_SYSTEM_LANG_FUNCTION_H_
#define __B0RK_SYSTEM_LANG_FUNCTION_H_

#include <b0rk/class.h>

class FunctionClass : public Class
{
 private:

 public:
    FunctionClass();
    ~FunctionClass();

    bool constructor(Context* context, Object* instance, int argCount);
};

#endif
