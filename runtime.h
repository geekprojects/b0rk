#ifndef __BSCRIPT_RUNTIME_H_
#define __BSCRIPT_RUNTIME_H_

#include "class.h"
#include "context.h"
#include "object.h"

#include <map>
#include <string>

class Context;
class Class;
class Object;

class Runtime
{
 private:
    std::map<std::string, Class*> m_classes;

 public:
    Runtime();
    ~Runtime();

    void addClass(Class* clazz);
    Class* findClass(std::string name);

    Context* createContext();

    Object* newObject(std::string clazz);
};

#endif
