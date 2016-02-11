/*
 * b0rk - The b0rk Embeddable Runtime Environment
 * Copyright (C) 2015, 2016 GeekProjects.com
 *
 * This file is part of b0rk.
 *
 * b0rk is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * b0rk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */

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

    void* m_appData;

    int64_t m_newObjects;
    int64_t m_newBytes;
    int64_t m_currentObjects;
    int64_t m_currentBytes;
    int64_t m_collectedObjects;
    int64_t m_arenaTotal;

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
    Object* newObject(Context* context, std::string clazz, int argCount);
    Object* newObject(Context* context, Class* clazz, int argCount, Value* args);
    Object* newObject(Context* context, std::string clazz, int argCount, Value* args);
    bool isObjectValid(Object* obj);

    void setAppData(void* data) { m_appData = data; }
    void* getAppData() { return m_appData; }

    void gc();
    void gcStats();

    Executor* getExecutor();
};

};

#endif
