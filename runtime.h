#ifndef __BSCRIPT_RUNTIME_H_
#define __BSCRIPT_RUNTIME_H_

#include "class.h"
#include "context.h"
#include "object.h"

#include <map>
#include <vector>
#include <list>
#include <string>

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
    std::map<std::string, Class*> m_classes;

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

    Class* loadClass(Context* context, std::string name);

 public:
    Runtime();
    ~Runtime();

    bool addClass(Context* context, Class* clazz);
    Class* findClass(Context* context, std::string name, bool load = true);

    Context* createContext();

    Object* allocateObject(Class* clazz);
    Object* newObject(Context* context, Class* clazz, int argCount);
    bool isObjectValid(Object* obj);

    void gc();
    void gcStats();

    Executor* getExecutor();
};

#endif
