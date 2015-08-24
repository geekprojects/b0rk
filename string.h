#ifndef __BSCRIPT_STRING_H_
#define __BSCRIPT_STRING_H_

#include "class.h"

class Context;

class String : public Class
{
 private:

 public:
    String();
    ~String();

    static Object* createString(Context* context, const char* str);
    static std::string getString(Context* context, Object* obj);
};

#endif
