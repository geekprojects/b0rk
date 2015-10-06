#ifndef __B0RK_SYSTEM_LANG_ARRAY_H_
#define __B0RK_SYSTEM_LANG_ARRAY_H_

#include <b0rk/class.h>

#include <string>
#include <map>

namespace b0rk
{

// TODO: HACK HACK HACK THIS WORKS BUT ISN'T RIGHT!
struct ArrayContainer
{
    std::map<std::string, Value> array;
};

class Array : public Class
{
 private:

 public:
    Array();
    ~Array();

    bool constructor(Context* context, Object* instance, int argCount, Value* args, Value& result);

    static ArrayContainer* getContainer(Object* array);
    static bool load(Object* array, Value index, Value& value);
    static bool store(Object* array, Value index, Value value);
};

};

#endif
