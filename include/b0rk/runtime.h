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
#include <b0rk/value.h>

#include <map>
#include <vector>
#include <list>
#include <deque>
#include <string>

namespace b0rk
{

class Context;
class Class;
struct Object;
class Executor;

enum RuntimeVerboseFlags
{
    VERBOSE_GC  = 0x0001,
    VERBOSE_ALL = 0xffff
};

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
    std::deque<std::wstring> m_classpath;
    std::map<std::wstring, Class*> m_classes;

    int m_optionVerboseFlags;
    bool m_optionDisableOptimiser;

    Class* m_objectClass;
    Class* m_stringClass;
    Class* m_exceptionClass;
    Class* m_arrayClass;
    Class* m_arrayDataClass;

    std::vector<Context*> m_contexts;

    std::map<std::wstring, Object*> m_stringTable;

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

    Class* loadClass(Context* context, std::wstring name, bool addToExisting = false);

 public:
    Runtime();
    ~Runtime();

    void setVerboseFlags(int verbose) { m_optionVerboseFlags = verbose; }
    int getVerboseFlags() { return m_optionVerboseFlags; }
    void setDisableOptimiser(bool disable) { m_optionDisableOptimiser = disable; }
    bool getDisableOptimiser() { return m_optionDisableOptimiser; }

    bool addClass(Context* context, Class* clazz, bool findScript = false);
    //Class* findClass(Context* context, std::string name, bool load = true);
    Class* findClass(Context* context, std::wstring name, bool load = true);

    void addClasspath(std::wstring classpath);

    inline Class* getObjectClass() const { return m_objectClass; }
    inline Class* getStringClass() const { return m_stringClass; }
    inline Class* getExceptionClass() const { return m_exceptionClass; }
    inline Class* getArrayClass() const { return m_arrayClass; }
    inline Class* getArrayDataClass() const { return m_arrayDataClass; }

    Context* createContext();

    Object* allocateObject(Class* clazz, unsigned int extraValues = 0);
    Object* newObject(Context* context, Class* clazz, int argCount);
    Object* newObject(Context* context, std::wstring clazz, int argCount);
    Object* newObject(Context* context, Class* clazz, int argCount, Value* args);
    Object* newObject(Context* context, std::wstring clazz, int argCount, Value* args);
    Object* newArray(Context* context, int size);
    Object* newString(Context* context, std::wstring string);
    bool isObjectValid(Object* obj);

    void setAppData(void* data) { m_appData = data; }
    void* getAppData() { return m_appData; }

    void gc(bool force = false);
    void gcStats();
    void gcDump();

    Executor* getExecutor();
};

};

#endif
