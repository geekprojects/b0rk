#ifndef __BSCRIPT_RUNTIME_H_
#define __BSCRIPT_RUNTIME_H_

#include <b0rk/class.h>
#include <b0rk/context.h>
#include <b0rk/object.h>

#include <map>
#include <vector>
#include <list>
#include <string>

namespace b0rk
{

class Context;
class Class;
struct Object;
class Executor;

struct Arena
{
    uint64_t m_start;
    Object* m_head;
    size_t m_size;
    std::list<Object*> m_freeList;
};

class Runtime
{
 private:
    std::vector<std::string> m_classpath;
    std::map<std::string, Class*> m_classes;

    Class* m_objectClass;

    std::vector<Context*> m_contexts;

    Arena m_arena;

    Executor* m_executor;

    int64_t m_newObjects;
    int64_t m_newBytes;
    int64_t m_currentObjects;
    int64_t m_currentBytes;
    int64_t m_collectedObjects;

    bool m_gcEnabled;
    int64_t m_gcLastAlloc;
    uint64_t m_gcTime;

    int64_t gcArena(Arena* arena, uint64_t mark);
    void gcMarkObject(Object* obj, uint64_t mark);

    bool callConstructor(Context* context, Object* obj, Class* clazz, int argCount);

    Class* loadClass(Context* context, std::string name, bool addToExisting = false);

 public:
    Runtime();
    ~Runtime();

    bool addClass(Context* context, Class* clazz, bool findScript = false);
    Class* findClass(Context* context, std::string name, bool load = true);
    Class* getObjectClass() { return m_objectClass; }

    Context* createContext();

    Object* allocateObject(Class* clazz);
    Object* newObject(Context* context, Class* clazz, int argCount);
    bool isObjectValid(Object* obj);

    void gc();
    void gcStats();

    Executor* getExecutor();
};

};

#endif
