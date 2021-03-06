/*
 *  b0rk - The b0rk Embeddable Runtime Environment
 *  Copyright (C) 2015, 2016 GeekProjects.com
 *
 *  This file is part of b0rk.
 *
 *  b0rk is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  b0rk is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with b0rk.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <b0rk/runtime.h>
#include <b0rk/executor.h>
#include <b0rk/lexer.h>
#include <b0rk/parser.h>
#include <b0rk/compiler.h>
#include <b0rk/utils.h>

#include "packages/system/lang/Object.h"
#include "packages/system/lang/Array.h"
#include "packages/system/lang/StringClass.h"
#include "packages/system/lang/Function.h"
#include "packages/system/lang/Maths.h"
#include "packages/system/lang/Exception.h"
#include "packages/system/lang/RuntimeClass.h"
#include "packages/system/lang/TimeClass.h"
#include "packages/system/io/File.h"

#include <cinttypes>

using namespace std;
using namespace b0rk;

#undef DEBUG_GC
#undef DEBUG_RUNTIME_NEW

#ifndef USEC_PER_SEC
#define USEC_PER_SEC	1000000ull	/* microseconds per second */
#endif

uint64_t getTimestamp()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t ts;
    ts = tv.tv_sec * USEC_PER_SEC;
    ts += tv.tv_usec;
    return ts;
}

Runtime::Runtime()
{
    m_objectClass = NULL;
    m_classpath.push_back(L".");
    m_classpath.push_back(DATA_PATH L"/packages");

    m_newObjects = 0;
    m_newBytes = 0;
    m_currentObjects = 0;
    m_currentBytes = 0;
    m_collectedObjects = 0;
    m_arenaTotal = 0;

    m_gcEnabled = true;
    m_gcLastAlloc = 0;
    m_gcTime = 0;

    m_arena.m_size = 16 * 1024 * 1024;
    m_arena.m_start = (uint64_t)malloc(m_arena.m_size);
    memset((void*)m_arena.m_start, 0, m_arena.m_size);
    m_arena.m_head = NULL;

    Object* freeObj = (Object*)m_arena.m_start;
    freeObj->m_size = m_arena.m_size;
    m_arena.m_freeList.push_back(freeObj);
    m_arenaTotal += m_arena.m_size;

    Context* initContext = new Context(this);

    // system.lang classes
    addClass(initContext, new RuntimeClass(), true);
    addClass(initContext, m_objectClass = new ObjectClass(), true);
    addClass(initContext, m_arrayClass = new Array(), true);
    addClass(initContext, m_arrayDataClass = new ArrayData(), true);
    addClass(initContext, m_stringClass = new String(), true);
    addClass(initContext, new FunctionClass(), true);
    addClass(initContext, new Maths(), true);
    addClass(initContext, new Time(), true);
    addClass(initContext, m_exceptionClass = new Exception(), true);

    // system.io classes
    addClass(initContext, new File(), true);

    delete initContext;

    m_executor = new Executor();
}

Runtime::~Runtime()
{
    gc(true);
    if (m_optionVerboseFlags & VERBOSE_GC)
    {
        gcStats();
    }

    // Get rid of classes
    map<wstring, Class*>::iterator it;
    for (it = m_classes.begin(); it != m_classes.end(); it++)
    {
        Class* clazz = it->second;
        if (clazz->deleteOnExit())
        {
            delete clazz;
        }
    }

    vector<Context*>::iterator ctxIt;
    for (ctxIt = m_contexts.begin(); ctxIt != m_contexts.end(); ctxIt++)
    {
        delete *ctxIt;
    }

    delete m_executor;
    free((void*)m_arena.m_start);
}

void Runtime::addClasspath(wstring cp)
{
    m_classpath.push_front(cp);
}

bool Runtime::addClass(Context* context, Class* clazz, bool findScript)
{

    Class* existing = findClass(context, clazz->getName(), false);
    if (existing != NULL)
    {
        printf("Runtime::addClass: Class already loaded: %ls\n", clazz->getName().c_str());
        return false;
    }

    if (findScript && m_objectClass != NULL && clazz != m_objectClass)
    {
        clazz->setSuperClass(m_objectClass);
    }

    m_classes.insert(make_pair(clazz->getName(), clazz));

    Function* initFunction = clazz->findMethod(L"<staticinit>");

    if (initFunction != NULL)
    {
        bool res = initFunction->execute(context, NULL, 0);
        if (!res)
        {
            return false;
        }
    }

    if (findScript)
    {
        loadClass(context, clazz->getName(), true);
    }

    return true;
}

Class* Runtime::findClass(Context* context, wstring name, bool load)
{
    map<wstring, Class*>::iterator it;
    it = m_classes.find(name);
    if (it != m_classes.end())
    {
        return it->second;
    }

    if (load)
    {
        return loadClass(context, name);
    }
    return NULL;
}

Class* Runtime::loadClass(Context* context, wstring name, bool addToExisting)
{
    wstring path;
    size_t i;
    for (i = 0; i < name.length(); i++)
    {
        char c = name[i];
        if (c == '.')
        {
            c = '/';
        }
        path += c;
    }
    path += L".bs";

#if 0
    fprintf(stderr, "Runtime::loadClass: Attempting to load class %s from file: %s\n", name.c_str(), path.c_str());
#endif

    FILE* fp = NULL;
    deque<wstring>::iterator cpit;

    for (cpit = m_classpath.begin(); cpit != m_classpath.end(); cpit++)
    {
        wstring cppath = *cpit + L"/" + path;
        fp = fopen(Utils::wstring2string(cppath).c_str(), "r");
#if 0
        fprintf(stderr, "Runtime::loadClass: %s: %p\n", cppath.c_str(), fp);
#endif
        if (fp != NULL)
        {
            break;
        }
    }

    if (fp == NULL)
    {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = new char[length + 128];
    memset(buffer + length, 0, 128);
    size_t read = fread(buffer, 1, length, fp);
    fclose(fp);

    if (read != length)
    {
        return NULL;
    }

    Lexer lexer;
    bool res = lexer.lexer(buffer, length);
    if (!res)
    {
        delete[] buffer;
        return NULL;
    }

    Parser parser(context);
    res = parser.parse(lexer.getTokens(), addToExisting);

    delete[] buffer;
    if (!res)
    {
        return 0;
    }

    map<wstring, Class*>::iterator it;
    it = m_classes.find(name);
    if (it != m_classes.end())
    {
        return it->second;
    }

    printf("Runtime::loadClass: ERROR: class %ls not found in file: %ls\n", name.c_str(), path.c_str());

    return NULL;
}

Object* Runtime::allocateObject(Class* clazz, unsigned int extraValues)
{

    if (m_currentBytes >= (m_arenaTotal / 4) * 3)
    {
#ifdef DEBUG_GC
        printf("Runtime::allocateObject: Space Check: %" PRId64 "/%" PRId64 " = %0.2f%% used\n", m_currentBytes, m_arenaTotal, ((float)m_currentBytes / (float)m_arenaTotal) * 100);
#endif
        if (m_gcEnabled)
        {
            // Check available space before we disable GC
#ifdef DEBUG_GC
            printf("Runtime::allocateObject: Less than 25%% of arena available, GCing\n");
#endif
            gc(true);
        }
#ifdef DEBUG_GC
        else
        {
            printf("Runtime::allocateObject: Less than 25%% of arena available, but GC is DISABLED\n");
        }
#endif
    }

    // Disable GC otherwise it could collect the new object before we're finished!
    bool enabled = m_gcEnabled;
    m_gcEnabled = false;

    int valueCount = clazz->getFieldCount();
    valueCount += extraValues;

    unsigned objSize = sizeof(Object) + (valueCount * sizeof(Value));

    Object* freeObj = NULL;
    list<Object*>::iterator it;
    for (it = m_arena.m_freeList.begin(); it != m_arena.m_freeList.end(); it++)
    {
        Object* o = *it;
#ifdef DEBUG_GC
        printf("Runtime::allocateObject: Free: %p (size=%ld, requested=%u)\n", o, o->m_size, objSize);
#endif
        if (o->m_size >= objSize)
        {
            freeObj = o;
            m_arena.m_freeList.erase(it);
            break;
        }
    }

    if (freeObj == NULL)
    {
        printf("Runtime::allocateObject: ERROR: No suitable free space!\n");
exit(-1);
        return NULL;
    }

    // Create our new object from the top of the free object
    Object* obj = freeObj;

    // Deal with the remainder of the free object
    size_t freeSize = freeObj->m_size - objSize;
    if (freeSize > sizeof(Object))
    {
        // Stick it back on the free list
        freeObj = (Object*)((uintptr_t)freeObj + objSize);
        m_arena.m_freeList.push_back(freeObj);

        freeObj->m_class = NULL;
        freeObj->m_size = freeSize;
    }
    else if (freeSize > 0)
    {
        // Too small to be of much use, just add it to the end of
        // the new object
        objSize += freeSize;
    }

#ifdef DEBUG_GC
    printf("Runtime::allocateObject: obj=%p, size=%d\n", obj, objSize);
#endif

    memset(obj, 0, objSize);

    obj->m_class = clazz;
    obj->m_size = objSize;
    obj->m_gcMark = 0;

    if (m_arena.m_head == NULL)
    {
        m_arena.m_head = obj;
    }

    m_newObjects++;
    m_newBytes += objSize;
    m_currentObjects++;
    m_currentBytes += objSize;

    m_gcEnabled = enabled;

    return obj;
}

Object* Runtime::newObject(Context* context, Class* clazz, int argCount)
{
    Object* obj = allocateObject(clazz);

    bool res = callConstructor(context, obj, clazz, argCount);
    if (!res)
    {
        printf("Runtime::newObject: Constructor failed!\n");
        return NULL;
    }

    return obj;
}

Object* Runtime::newObject(Context* context, wstring clazzName, int argCount)
{
    Class* clazz = findClass(context, clazzName);
    if (clazz == NULL)
    {
        printf("Runtime::newObject: Unable to find class: %ls\n", clazzName.c_str());
        return NULL;
    }
    return newObject(context, clazz, argCount);
}

Object* Runtime::newObject(Context* context, Class* clazz, int argCount, Value* args)
{
    int i;
    for (i = 0; i < argCount; i++)
    {
        context->push(args[i]);
    }
    return newObject(context, clazz, argCount);
}

Object* Runtime::newObject(Context* context, wstring clazzName, int argCount, Value* args)
{
    Class* clazz = findClass(context, clazzName);
    if (clazz == NULL)
    {
        return NULL;
    }
    return newObject(context, clazz, argCount, args);
}

Object* Runtime::newArray(Context* context, int size)
{
    Value sizeVal;
    sizeVal.type = VALUE_INTEGER;
    sizeVal.i = size;
    context->push(sizeVal);
    return newObject(context, m_arrayClass, 1);
}

Object* Runtime::newString(Context* context, wstring string)
{
    map<wstring, Object*>::iterator it;
    it = m_stringTable.find(string);
    if (it != m_stringTable.end())
    {
        return it->second;
    }

    Object* stringObj = String::createString(context, string);
    m_stringTable.insert(make_pair(string, stringObj));
    stringObj->setExternalGC();
    return stringObj;
}

bool Runtime::callConstructor(Context* context, Object* obj, Class* clazz, int argCount)
{
    bool res;
    if (clazz->getSuperClass() != NULL)
    {
        res = callConstructor(context, obj, clazz->getSuperClass(), 0);
        if (!res)
        {
            return false;
        }
    }

#ifdef DEBUG_RUNTIME_NEW
    printf("Runtime::callConstructor: obj=%p, class=%ls, super=%p\n", obj, clazz->getName().c_str(), clazz->getSuperClass());
#endif

    // The constructor should be named after the
    // last part of the class name
    wstring ctorName = clazz->getName();
    size_t pos = ctorName.rfind('.');
    if (pos != string::npos)
    {
        ctorName = ctorName.substr(pos + 1);
    }

    Function* ctor = clazz->findMethod(ctorName);
#ifdef DEBUG_RUNTIME_NEW
    printf("Runtime::callConstructor: constructor name: %ls = %p\n", ctorName.c_str(), ctor);
#endif

    if (ctor != NULL)
    {
        res = ctor->execute(context, obj, argCount);
        if (!res)
        {
#ifdef DEBUG_RUNTIME_NEW
    printf("Runtime::callConstructor: Execution failed! ctor=%ls\n", ctor->getFullName().c_str());
#endif
            return false;
        }

        // All functions must return something.
        // However, constructor's output is ignored
        context->pop();

    }
    return true;
}

bool Runtime::isObjectValid(Object* obj)
{
    uint64_t end = m_arena.m_start + m_arena.m_size;

    if ((uint64_t)obj >= m_arena.m_start && (uint64_t)obj < end)
    {
        return true;
    }
    return false;
}

void Runtime::gc(bool force)
{
    if (!m_gcEnabled)
    {
        return;
    }

    uint64_t now = getTimestamp();
#ifdef DEBUG_GC
    fprintf(stderr, "Runtime::gc: Marking, now=%" PRIx64 "\n", now);
#endif
    int64_t freed = 0;
    if (force || m_newObjects > m_gcLastAlloc)
    {

        // Mark objects from static fields
        map<wstring, Class*>::iterator it;
        for (it = m_classes.begin(); it != m_classes.end(); it++)
        {
            unsigned int i;
            Class* clazz = it->second;
            for (i = 0; i < clazz->getStaticFieldCount(); i++)
            {
                Value v = clazz->getStaticField(i);
                if (v.type == VALUE_OBJECT && isObjectValid(v.object))
                {
                    gcMarkObject(v.object, now);
                }
            }
        }

        freed += gcArena(&m_arena, now);
        m_gcLastAlloc = m_newObjects;
    }
    uint64_t endTime = getTimestamp();
    m_gcTime += (endTime - now);

#ifdef DEBUG_GC
    if (freed > 0)
    {
        fprintf(stderr, "Runtime::gc: Freed %" PRId64 " bytes\n", freed);
    }
    fprintf(stderr, "Runtime::gc: Time: %" PRId64 " msec\n", (endTime - now) / 1000);
#endif
}

int64_t Runtime::gcArena(Arena* arena, uint64_t mark)
{
    uint64_t pos = arena->m_start;
    uint64_t end = arena->m_start + arena->m_size;

    // Update everything on stacks
    vector<Context*>::iterator ctxit;
    for (ctxit = m_contexts.begin(); ctxit != m_contexts.end(); ctxit++)
    {
        Context* ctx = *ctxit;
        int s;

        for (s = 0; s < ctx->getStackPos(); s++)
        {
            Value* v = &(ctx->getStack()[s]);
            if (v->type == VALUE_OBJECT && isObjectValid(v->object))
            {
#ifdef DEBUG_GC
                fprintf(stderr, "Runtime::gcArena: Stacked Object: %p\n", v->object);
#endif
                gcMarkObject(v->object, mark);
            }
            else if (v->type == VALUE_FRAME && v->pointer != NULL)
            {
#ifdef DEBUG_GC
                fprintf(stderr, "Runtime::gcArena: Stacked Frame: %p\n", v->pointer);
#endif
                Frame* frame = (Frame*)(v->pointer);
                int i;
                for (i = 0; i < frame->localVarsCount; i++)
                {
                    Object* obj = frame->localVars[i].object;
                    if (frame->localVars[i].type == VALUE_OBJECT && isObjectValid(obj))
                    {
                        gcMarkObject(obj, mark);
                    }
                }
            }
        }
    }

    // Look for objects that haven't been marked
    m_currentObjects = 0;
    m_currentBytes = 0;
    int freed = 0;
    while (pos < end)
    {
        Object* obj = (Object*)pos;

        if (obj->m_class != NULL)
        {
            if (obj->m_gcMark == mark || obj->m_gcMark == B0RK_GC_EXTERNAL)
            {
                m_currentObjects++;
                m_currentBytes += obj->m_size;
            }
            else
            {
                // This object hasn't been marked, collect it!

                freed += obj->m_size;
#ifdef DEBUG_GC
                float survived = 0;
                if (obj->m_gcMark != 0)
                {
                    survived = (float)(mark - obj->m_gcMark) / (float)USEC_PER_SEC;
                }

                fprintf(stderr, "Runtime::gc:  Collecting: %p (%ls) %" PRId64 "-%" PRId64 " = survived=%f\n", obj, obj->m_class->getName().c_str(), mark, obj->m_gcMark, survived);
#endif
                m_collectedObjects++;
                obj->m_class = NULL;
                obj->m_gcMark = -1;

                // Return object back to free list
                m_arena.m_freeList.push_back(obj);
            }
        }

        pos += obj->m_size;
    }

    if (freed > 0)
    {
        // Sort it, this takes time but makes coalescing much easier
        m_arena.m_freeList.sort();

        // Coalesce ajacent free objects together
        bool changed = true;
        while (changed)
        {
            changed = false;
            Object* prevFreeObj = NULL;
            uint64_t prevFreeObjEnd = 0;
            list<Object*>::iterator it;
            for (it = m_arena.m_freeList.begin(); it != m_arena.m_freeList.end(); it++)
            {
                Object* freeObj = *it;
                if ((uint64_t)freeObj == prevFreeObjEnd)
                {
#ifdef DEBUG_GC
                    fprintf(
                        stderr,
                        "Runtime::gc: Coalesce: prev=%p-0x%" PRIx64 ", this=%p-0x%" PRIx64 "\n",
                        prevFreeObj,
                        (uint64_t)prevFreeObj + prevFreeObj->m_size,
                        freeObj,
                        (uint64_t)freeObj + freeObj->m_size);
#endif
                    prevFreeObj->m_size += freeObj->m_size;
                    m_arena.m_freeList.erase(it);
                    changed = true;
                    break;
                }
                prevFreeObj = freeObj;
                prevFreeObjEnd = (uint64_t)freeObj + freeObj->m_size;
            }
        }
    }
    return freed;
}

void Runtime::gcMarkObject(Object* obj, uint64_t mark)
{
    if (obj->m_gcMark != B0RK_GC_EXTERNAL)
    {
        obj->m_gcMark = mark;
    }

    if (B0RK_UNLIKELY(obj->m_class == NULL))
    {
        printf("Runtime::gcMarkObject: WTF? class is NULL");
        return;
    }

    // Note, the number of values this object has my be different
    // to the number of it's classes fields, for instance the
    // ArrayData class adds extra values to store it's data
    unsigned int valueCount = (obj->m_size - sizeof(Object)) / sizeof(Value);
#ifdef DEBUG_GC
    fprintf(stderr, "Runtime::gcMarkObject: %p (%ls), values=%u\n", obj, obj->m_class->getName().c_str(), valueCount);
#endif

    unsigned int i;
    for (i = 0; i < valueCount; i++)
    {
        Object* child = obj->m_values[i].object;
        if (obj->m_values[i].type == VALUE_OBJECT && isObjectValid(child))
        {
            gcMarkObject(child, mark);
        }
    }
}

void Runtime::gcStats()
{
    fprintf(stderr, "Runtime: Stats:\n");
    fprintf(stderr, "Runtime:  New Objects: %" PRId64 "\n", m_newObjects);
    fprintf(stderr, "Runtime:  Current Objects: %" PRId64 "\n", m_currentObjects);
    fprintf(stderr, "Runtime:  Collected Objects: %" PRId64 "\n", m_collectedObjects);
    fprintf(stderr, "Runtime:  New Bytes: %" PRId64 "\n", m_newBytes);
    fprintf(stderr, "Runtime:  Current Bytes: %" PRId64 "\n", m_currentBytes);
    fprintf(stderr, "Runtime:  GC Time: %" PRId64 " ms\n", m_gcTime / 1000);

}

void Runtime::gcDump()
{
    uint64_t pos = m_arena.m_start;
    uint64_t end = m_arena.m_start + m_arena.m_size;
    while (pos < end)
    {
        Object* obj = (Object*)pos;

        if (obj->m_class != NULL)
        {
            fprintf(
                stderr,
                "Runtime: Dump: %p: %ls, size=%ld, gcMark=0x%" PRIx64 "\n",
                obj,
                obj->m_class->getName().c_str(),
                obj->m_size, obj->m_gcMark);
        }
        pos += obj->m_size;
    }
}

Context* Runtime::createContext()
{
    Context* ctx = new Context(this);
    m_contexts.push_back(ctx);
    return ctx;
}

Executor* Runtime::getExecutor()
{
    return m_executor;
}

