#ifndef __B0RK_SYSTEM_LANG_OBJECT_H_
#define __B0RK_SYSTEM_LANG_OBJECT_H_

#include <b0rk/class.h>

#include <string>
#include <map>

class ObjectClass : public Class
{
 private:

 public:
    ObjectClass();
    ~ObjectClass();

    //bool constructor(Context* context, Object* instance, int argCount);
    bool toString(Context* context, Object* instance, int argCount);
};

#endif
