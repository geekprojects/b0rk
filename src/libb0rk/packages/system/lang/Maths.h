#ifndef __B0RK_SYSTEM_LANG_MATHS_H_
#define __B0RK_SYSTEM_LANG_MATHS_H_

#include <geek/core-random.h>
#include <b0rk/class.h>

#include <string>
#include <map>

namespace b0rk
{

class Maths : public Class
{
 private:
    Geek::Core::Random m_random;

 public:
    Maths();
    ~Maths();

    bool randomInt(Context* context, Object* instance, int argCount, Value* args, Value& result);
    bool round(Context* context, Object* instance, int argCount, Value* args, Value& result);
};

};

#endif
