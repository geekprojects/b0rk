#ifndef __BSCRIPT_BUILTIN_FILE_H_
#define __BSCRIPT_BUILTIN_FILE_H_

#include "class.h"

class File : public Class
{
 private:

 public:
    File();
    ~File();

    bool write(Context* context, Object* instance, int argCount);
    bool init(Context* context, Object* instance, int argCount);
};

#endif
