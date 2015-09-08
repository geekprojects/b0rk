
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "runtime.h"
#include "string.h"
#include "system.h"
#include "executor.h"

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
    m_newObjects = 0;
    m_newBytes = 0;
    m_currentObjects = 0;
    m_currentBytes = 0;
    m_collectedObjects = 0;
    m_gcLastAlloc = 0;
    m_gcTime = 0;

    m_arena.m_size = 4 * 1024 * 1024;
    m_arena.m_start = (uint64_t)malloc(m_arena.m_size);
    memset((void*)m_arena.m_start, 0, m_arena.m_size);
    m_arena.m_head = NULL;

    m_arena.m_free = (Object*)m_arena.m_start;
    m_arena.m_free->m_size = m_arena.m_size;

    addClass(new System());
    addClass(new String());

    m_executor = new Executor();
}

Runtime::~Runtime()
{
}

void Runtime::addClass(Class* clazz)
{
    m_classes.insert(make_pair(clazz->getName(), clazz));
}

Class* Runtime::findClass(string name)
{
    map<string, Class*>::iterator it;
    it = m_classes.find(name);
    if (it != m_classes.end())
    {
        return it->second;
    }
    return NULL;
}

Object* Runtime::allocateObject(Class* clazz)
{
    int objSize = sizeof(Object) + (clazz->getValueCount() * sizeof(Value));
    //Object* obj = (Object*)malloc(objSize);
    size_t freeSize = m_arena.m_free->m_size;

    Object* obj = (Object*)m_arena.m_free;
    //printf("Runtime::allocateObject: obj=%p, size=%d\n", obj, objSize);

    m_arena.m_free = (Object*)((uintptr_t)m_arena.m_free + objSize);
    m_arena.m_free->m_class = NULL;
    m_arena.m_free->m_size = freeSize - objSize;

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
    Object* obj = allocateObject(clazz);

    Function* ctor = clazz->findMethod(clazz->getName());
    if (ctor != NULL)
    {
        bool res = ctor->execute(context, obj, argCount);
        if (!res)
        {
            return NULL;
        }

        // All functions must return something.
        // However, constructor's output is ignored
        context->pop();
    }

    return obj;
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
    uint64_t now = getTimestamp();
    int64_t freed = 0;
    if (m_newObjects > m_gcLastAlloc)
    {
        freed += gcArena(&m_arena, now);
        m_gcLastAlloc = m_newObjects;
    }
    uint64_t endTime = getTimestamp();
    m_gcTime += (endTime - now);

#ifdef DEBUG_GC
    if (freed > 0)
    {
        printf("Runtime::gc: Freed %d bytes\n", freed);
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

    //printf("Runtime::gc: start=0x%llx, end=0x%llx\n", m_arena, end);
    m_currentObjects = 0;
    m_currentBytes = 0;
    int freed = 0;
    while (pos < end)
    {
        Object* obj = (Object*)pos;
        //printf("Runtime::gc: pos=%p, class=%p, size=%ld\n", obj, obj->m_class, obj->m_size);

        if (obj->m_class != NULL)
        {
            //printf("Runtime::gc:  -> mark=%lld\n", obj->m_gcMark);
            if (obj->m_gcMark == mark)
            {
                //printf("Runtime::gc:  -> CURRENT\n");
                m_currentObjects++;
                m_currentBytes += obj->m_size;
            }
            else
            {
                freed += obj->m_size;
                float survived = 0;
                if (obj->m_gcMark != 0)
                {
                    survived = (float)(mark - obj->m_gcMark) / (float)USEC_PER_SEC;
                }
                //printf("Runtime::gc:  -> OLD! GC!! %lld-%lld = survived=%f\n", mark, obj->m_gcMark, survived);
                m_collectedObjects++;
                obj->m_class = NULL;
                obj->m_gcMark = -1;
            }
        }

        pos += obj->m_size;
    }
    return freed;
}

void Runtime::gcMarkObject(Object* obj, uint64_t mark)
{
    obj->m_gcMark = mark;

    int i;
    for (i = 0; i < obj->m_class->getValueCount(); i++)
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
    printf("Runtime: Stats:\n");
    printf("Runtime:  New Objects: %lld\n", m_newObjects);
    printf("Runtime:  Current Objects: %lld\n", m_currentObjects);
    printf("Runtime:  Collected Objects: %lld\n", m_collectedObjects);
    printf("Runtime:  New Bytes: %lld\n", m_newBytes);
    printf("Runtime:  Current Bytes: %lld\n", m_currentBytes);
    printf("Runtime:  GC Time: %lld ms\n", m_gcTime / 1000);
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

