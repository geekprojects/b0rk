#ifndef __BSCRIPT_BUILTIN_FILE_H_
#define __BSCRIPT_BUILTIN_FILE_H_

#include <b0rk/class.h>

namespace b0rk
{

class File : public Class
{
 private:

 public:
    File();
    ~File();

    bool write(Context* context, Object* instance, int argCount, Value* args, Value& result);
    bool init(Context* context, Object* instance, int argCount, Value* args, Value& result);
};
}

#endif
