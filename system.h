#ifndef __BSCRIPT_BUILTIN_SYSTEM_H_
#define __BSCRIPT_BUILTIN_SYSTEM_H_

#include "class.h"

class System : public Class
{
 private:

 public:
    System();
    ~System();

    bool log(Context* context, Object* instance, int argCount);
};

#endif
