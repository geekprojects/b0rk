
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <b0rk/runtime.h>
#include <b0rk/executor.h>
#include <b0rk/lexer.h>
#include <b0rk/parser.h>

#include "packages/system/lang/Object.h"
#include "packages/system/lang/Array.h"
#include "packages/system/lang/StringClass.h"
#include "packages/system/lang/Function.h"
#include "packages/system/io/File.h"

using namespace std;

#undef DEBUG_GC

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
    m_classpath.push_back(".");
    m_classpath.push_back(DATA_PATH "/packages");

    m_newObjects = 0;
    m_newBytes = 0;
    m_currentObjects = 0;
    m_currentBytes = 0;
    m_collectedObjects = 0;

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

    Context* initContext = new Context(this);
    m_objectClass = new ObjectClass();
    addClass(initContext, m_objectClass, true);
    addClass(initContext, new Array(), true);
    addClass(initContext, new String(), true);
    addClass(initContext, new FunctionClass(), true);
    addClass(initContext, new File(), true);
    delete initContext;

    m_executor = new Executor();
}

Runtime::~Runtime()
{
    gc();
    gcStats();

    // Get rid of classes
    map<std::string, Class*>::iterator it;
    for (it = m_classes.begin(); it != m_classes.end(); it++)
    {
        delete it->second;
    }

    vector<Context*>::iterator ctxIt;
    for (ctxIt = m_contexts.begin(); ctxIt != m_contexts.end(); ctxIt++)
    {
        delete *ctxIt;
    }

    delete m_executor;
    free((void*)m_arena.m_start);
}

bool Runtime::addClass(Context* context, Class* clazz, bool findScript)
{
    if (findScript && m_objectClass != NULL && clazz != m_objectClass)
    {
        clazz->setSuperClass(m_objectClass);
    }

    m_classes.insert(make_pair(clazz->getName(), clazz));

    clazz->initStaticFields();

    Function* initFunction = clazz->findMethod("<staticinit>");

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

Class* Runtime::findClass(Context* context, string name, bool load)
{
    map<string, Class*>::iterator it;
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

Class* Runtime::loadClass(Context* context, string name, bool addToExisting)
{
    string path = "";
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
    path += ".bs";

#if 0
    fprintf(stderr, "Runtime::loadClass: Attempting to load class %s from file: %s\n", name.c_str(), path.c_str());
#endif

    FILE* fp = NULL;
    vector<string>::iterator cpit;

    for (cpit = m_classpath.begin(); cpit != m_classpath.end(); cpit++)
    {
        string cppath = *cpit + "/" + path;
        fp = fopen(cppath.c_str(), "r");
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
        if (!addToExisting)
        {
            fprintf(stderr, "Runtime::loadClass: Unable to load class %s\n", name.c_str());
        }
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = new char[length + 128];
    memset(buffer + length, 0, 128);
    fread(buffer, 1, length, fp);
    fclose(fp);

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

    map<string, Class*>::iterator it;
    it = m_classes.find(name);
    if (it != m_classes.end())
    {
        return it->second;
    }

    printf("Runtime::loadClass: ERROR: class %s not found in file: %s\n", name.c_str(), path.c_str());

    return NULL;
}

Object* Runtime::allocateObject(Class* clazz)
{
    int objSize = sizeof(Object) + (clazz->getFieldCount() * sizeof(Value));

    Object* freeObj = NULL;
    list<Object*>::iterator it;
    for (it = m_arena.m_freeList.begin(); it != m_arena.m_freeList.end(); it++)
    {
        Object* o = *it;
#ifdef DEBUG_GC
        printf("Runtime::allocateObject: Free: %p (size=%ld, requested=%d)\n", o, o->m_size, objSize);
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

    return obj;
}

Object* Runtime::newObject(Context* context, Class* clazz, int argCount)
{
    // Disable GC otherwise it could collect the new object before we're finished!
    bool enabled = m_gcEnabled;
    m_gcEnabled = false;
    Object* obj = allocateObject(clazz);

    bool res = callConstructor(context, obj, clazz, argCount);
    if (!res)
    {
        return NULL;
    }
    m_gcEnabled = enabled;

    return obj;
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
    printf("Runtime::callConstructor: obj=%p, class=%s, super=%p\n", obj, clazz->getName().c_str(), clazz->getSuperClass());
#endif

    // The constructor should be named after the
    // last part of the class name
    string ctorName = clazz->getName();
    size_t pos = ctorName.rfind('.');
    if (pos != string::npos)
    {
        ctorName = ctorName.substr(pos + 1);
    }

    Function* ctor = clazz->findMethod(ctorName);
    if (ctor != NULL)
    {
        res = ctor->execute(context, obj, argCount);
        if (!res)
        {
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

void Runtime::gc()
{
    if (!m_gcEnabled)
    {
        return;
    }

    uint64_t now = getTimestamp();
    int64_t freed = 0;
    if (m_newObjects > m_gcLastAlloc)
    {

        // Mark objects from static fields
        map<string, Class*>::iterator it;
        for (it = m_classes.begin(); it != m_classes.end(); it++)
        {
            int i;
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
        printf("Runtime::gc: Freed %lld bytes\n", freed);
        gcStats();
    }
    printf("Runtime::gc: Time: %lld msec\n", (endTime - now) / 1000);
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
        vector<Value>::iterator stackit;
        for (stackit = ctx->getStack().begin(); stackit != ctx->getStack().end(); stackit++)
        {
            if ((*stackit).type == VALUE_OBJECT && isObjectValid((*stackit).object))
            {
                //printf("Runtime::gc: Stacked Object: %p\n", (*stackit).object);
                gcMarkObject((*stackit).object, mark);
            }
            else if ((*stackit).type == VALUE_FRAME && (*stackit).pointer != NULL)
            {
                //printf("Runtime::gc: Stacked Frame: %p\n", (*stackit).pointer);
                Frame* frame = (Frame*)((*stackit).pointer);
                int i;
                for (i = 0; i < frame->localVarsCount; i++)
                {
                    Object* obj = frame->localVars[i].object;
                    if (frame->localVars[i].type == VALUE_OBJECT && isObjectValid(obj))
                    {
                        //printf("Runtime::gc: Object on Variable: %p\n", obj);
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
            if (obj->m_gcMark == mark)
            {
                m_currentObjects++;
                m_currentBytes += obj->m_size;
            }
            else
            {
                // This object hasn't been marked, collect it!

                // Free up any private pointers (Mostly Strings)
                int i;
                for (i = 0; i < obj->m_class->getFieldCount(); i++)
                {
                    if (obj->m_values[i].type == VALUE_POINTER && obj->m_values[i].pointer != NULL)
                    {
                        free(obj->m_values[i].pointer);
                    }
                }

                freed += obj->m_size;
#ifdef DEBUG_GC
                float survived = 0;
                if (obj->m_gcMark != 0)
                {
                    survived = (float)(mark - obj->m_gcMark) / (float)USEC_PER_SEC;
                }

                printf("Runtime::gc:  -> OLD! GC!! %lld-%lld = survived=%f\n", mark, obj->m_gcMark, survived);
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
                    printf("Runtime::gc: Coalesce: prev=%p-0x%llx, this=%p-0x%llx\n",
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
    obj->m_gcMark = mark;

if (obj->m_class == NULL)
{
printf("Runtime::gcMarkObject: WTF? class is NULL");
return;
}

    int i;
    for (i = 0; i < obj->m_class->getFieldCount(); i++)
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
    fprintf(stderr, "Runtime:  New Objects: %lld\n", m_newObjects);
    fprintf(stderr, "Runtime:  Current Objects: %lld\n", m_currentObjects);
    fprintf(stderr, "Runtime:  Collected Objects: %lld\n", m_collectedObjects);
    fprintf(stderr, "Runtime:  New Bytes: %lld\n", m_newBytes);
    fprintf(stderr, "Runtime:  Current Bytes: %lld\n", m_currentBytes);
    fprintf(stderr, "Runtime:  GC Time: %lld ms\n", m_gcTime / 1000);
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

