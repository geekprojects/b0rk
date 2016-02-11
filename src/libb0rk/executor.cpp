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

#include <b0rk/executor.h>
#include <b0rk/compiler.h>
#include "packages/system/lang/StringClass.h"
#include "packages/system/lang/Array.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

#undef DEBUG_EXECUTOR
#undef DEBUG_EXECUTOR_STATS

#define DOUBLE_VALUE(_v) (((_v).type == VALUE_DOUBLE) ? (_v.d) : (double)(_v.i))
#define INTEGER_VALUE(_v) (((_v).type == VALUE_INTEGER) ? (_v.i) : (int)(floor(_v.d)))

#ifdef DEBUG_EXECUTOR
#define LOG(_fmt, _args...) \
    printf("Executor::run: %s:%04llx: 0x%llx " _fmt "\n", frame->code->function->getFullName().c_str(), thisPC, opcode, _args);
#else
#define LOG(_fmt, _args...)
#endif

#define ERROR(_fmt, _args...) \
    printf("Executor::run: %s:%04llx: 0x%llx ERROR: " _fmt "\n", frame->code->function->getFullName().c_str(), thisPC, opcode, _args);

using namespace std;
using namespace b0rk;

#ifdef DEBUG_EXECUTOR_STATS
int g_stats[OPCODE_MAX];
#endif

uint64_t g_loadVarCount = 0;
uint64_t g_loadVarThisCount = 0;

static bool opcodeLoadVar(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int varId = frame->fetch();
    context->push(frame->localVars[varId]);
    LOG("LOAD_VAR: v%d: %s", varId, frame->localVars[varId].toString().c_str());
    g_loadVarCount++;
    if (varId == 0)
    {
        g_loadVarThisCount++;
    }
    return true;
}

static bool opcodeStoreVar(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int varId = frame->fetch();
    frame->localVars[varId] = context->pop();
    LOG("STORE_VAR: v%d = %s", varId, frame->localVars[varId].toString().c_str());

    return true;
}

static bool opcodeLoadField(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value objValue = context->pop();
    int fieldId = frame->fetch();
    LOG("LOAD_FIELD: f%d, this=%p (value count=%ld)", fieldId, objValue.object, objValue.object->getClass()->getFieldCount());
    Value result = objValue.object->getValue(fieldId);
    context->push(result);
    return true;
}

static bool opcodeLoadFieldNamed(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Object* nameObj = context->pop().object;
    Value objValue = context->pop();
    string varName = String::getString(context, nameObj);
    LOG("LOAD_FIELD_NAMED: obj=%p, name=%s\n", objValue.object, varName.c_str());

    if (objValue.type == VALUE_OBJECT && objValue.object != NULL)
    {
        Object* obj = objValue.object;
        Class* clazz = obj->getClass();

        int id = clazz->getFieldId(varName);
        if (id != -1)
        {

            LOG("LOAD_FIELD_NAMED: class=%s, obj=%p, name=%s, id=%d\n", clazz->getName().c_str(), objValue.object, varName.c_str(), id);

            context->push(obj->getValue(id));
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
    return true;
}

static bool opcodeStoreFieldNamed(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value nameValue = context->pop();
    Value objValue = context->pop();
    Value value = context->pop();
    string varName = String::getString(context, nameValue);
    LOG("STORE_FIELD_NAMED: obj=%p, name=%s\n", objValue.object, varName.c_str());

    if (objValue.type == VALUE_OBJECT && objValue.object != NULL)
    {
        Object* obj = objValue.object;
        Class* clazz = obj->getClass();

        int id = clazz->getFieldId(varName);
        if (id != -1)
        {
            LOG("STORE_FIELD_NAMED: class=%s, obj=%p, name=%s, id=%d\n", clazz->getName().c_str(), objValue.object, varName.c_str(), id);
            obj->setValue(id, value);
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

static bool opcodeLoadStaticField(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Class* clazz = (Class*)frame->fetch();
    int fieldId = frame->fetch();

    Value v = clazz->getStaticField(fieldId);

    LOG("LOAD_STATIC_FIELD: f%d, class=%s, v=%s", fieldId, clazz->getName().c_str(), v.toString().c_str());

    context->push(v);

    return true;
}

static bool opcodeStoreField(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value objValue = context->pop();
    Value value = context->pop();
    int fieldId = frame->fetch();
    LOG("STORE_FIELD: f%d, this=%p", fieldId, objValue.object);
    if (objValue.object == NULL)
    {
        ERROR("STORE_FIELD: f%d, this=%p", fieldId, objValue.object);
        return false;
    }

    objValue.object->setValue(fieldId, value);
    return true;
}

static bool opcodeLoadArray(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value indexValue = context->pop();
    Value arrayValue = context->pop();

    LOG("STORE_ARRAY: indexValue=%s", indexValue.toString().c_str());
    LOG("STORE_ARRAY: arrayValue=%s", arrayValue.toString().c_str());

    bool res = false;
    if (arrayValue.type == VALUE_OBJECT && arrayValue.object != NULL)
    {
        Object* array = arrayValue.object;
        Value valueValue;
        res = Array::load(array, indexValue, valueValue);
        context->push(valueValue);
    }
    else
    {
        ERROR("STORE_ARRAY: Array is invalid: %s", arrayValue.toString().c_str());
    }

    return res;
}

static bool opcodeStoreArray(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value indexValue = context->pop();
    Value arrayValue = context->pop();
    Value valueValue = context->pop();

    LOG("STORE_ARRAY: indexValue=%s", indexValue.toString().c_str());
    LOG("STORE_ARRAY: arrayValue=%s", arrayValue.toString().c_str());
    LOG("STORE_ARRAY: valueValue=%s", valueValue.toString().c_str());

    bool res = false;
    if (arrayValue.type == VALUE_OBJECT && arrayValue.object != NULL)
    {
        Object* array = arrayValue.object;
        res = Array::store(array, indexValue, valueValue);
    }
    else
    {
        ERROR("STORE_ARRAY: Array is invalid: %s", arrayValue.toString().c_str());
    }

    return res;
}

static bool opcodeIncVar(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int varId = frame->fetch();
    int64_t amount = frame->fetch();
    frame->localVars[varId].i += amount;

    LOG("INC_VAR: v%d, %lld: %lld", varId, amount, frame->localVars[varId].i);

    return true;
}


static bool opcodePushI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_INTEGER;
    v.i = frame->fetch();
    LOG("PUSHI: %lld", v.i);
    context->push(v);
    return true;
}

static bool opcodePushD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_DOUBLE;
    uint64_t i = frame->fetch();
    double* d = (double*)(&i);
    v.d = *d;
    LOG("PUSHD: %0.2f", v.d);
    context->push(v);
    return true;
}

static bool opcodePushObj(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_OBJECT;
    Object* obj = (Object*)frame->fetch();
    v.object = obj;
    LOG("PUSHD: %p", obj);
    context->push(v);
    return true;
}


static bool opcodeDup(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v = context->pop();
    LOG("DUP: %s", v.toString().c_str());
    context->push(v);
    context->push(v);
    return true;
}

static bool opcodeAdd(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    if (v1.type == VALUE_OBJECT)
    {
        // TODO: This should have been figured out by the assembler?
        Class* clazz = v1.object->getClass();
        LOG("ADD OBJECT: class=%s", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod("operator+");
        LOG("ADD OBJECT: add operator=%p", addFunc);
        bool res = addFunc->execute(context, v1.object, 1);
        if (!res)
        {
            return false;
        }
    }
    else
    {
        Value v2 = context->pop();
        Value result;
        if (v1.type == VALUE_DOUBLE && v2.type == VALUE_DOUBLE)
        {
            result.type = VALUE_DOUBLE;
            result.d = v1.d + v2.d;
            LOG("ADD: %0.2f + %0.2f = %0.2f", v1.d, v2.d, result.d);
        }
        else if (v1.type == VALUE_DOUBLE || v2.type == VALUE_DOUBLE)
        {
            result.type = VALUE_DOUBLE;
            result.d = DOUBLE_VALUE(v1) + DOUBLE_VALUE(v2);
            LOG("ADD: %0.2f + %0.2f = %0.2f", v1.d, v2.d, result.d);
        }
        else
        {
            result.type = VALUE_INTEGER;
            result.i = v1.i + v2.i;
            LOG("ADD: %lld + %lld = %lld", v1.i, v2.i, result.i);
        }
        context->push(result);
    }
    return true;
}

static bool opcodeSub(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    if (v1.type == VALUE_OBJECT)
    {
        // TODO: This should have been figured out by the assembler?
        Class* clazz = v1.object->getClass();
        LOG("SUB OBJECT: class=%s", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod("operator-");
        LOG("SUB OBJECT: sub operator=%p", addFunc);
        bool res = addFunc->execute(context, v1.object, 1);
        if (!res)
        {
            return false;
        }
    }
    else
    {
        Value v2 = context->pop();
        Value result;
        if (v1.type == VALUE_DOUBLE || v2.type == VALUE_DOUBLE)
        {
            result.type = VALUE_DOUBLE;
            result.d = DOUBLE_VALUE(v1) - DOUBLE_VALUE(v2);
            LOG("SUB: %0.2f - %0.2f = %0.2f", v1.d, v2.d, result.d);
        }
        else
        {
            result.type = VALUE_INTEGER;
            result.i = v1.i - v2.i;
            LOG("SUB: %lld - %lld = %lld", v1.i, v2.i, result.i);
        }
        context->push(result);
    }
    return true;
}


static bool opcodeMul(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    if (v1.type == VALUE_OBJECT)
    {
        // TODO: This should have been figured out by the assembler?
        Class* clazz = v1.object->getClass();
        LOG("MUL OBJECT: class=%s", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod("operator*");
        LOG("MUL OBJECT: * operator=%p", addFunc);
        bool res = addFunc->execute(context, v1.object, 1);
        if (!res)
        {
            return false;
        }
    }
    else
    {
        Value v2 = context->pop();
        Value result;
        if (v1.type == VALUE_DOUBLE && v2.type == VALUE_DOUBLE)
        {
            result.type = VALUE_DOUBLE;
            result.d = v1.d * v2.d;
            LOG("MUL: %0.2f * %0.2f = %0.2f", v1.d, v2.d, result.d);
        }
        else if (v1.type == VALUE_DOUBLE || v2.type == VALUE_DOUBLE)
        {
            result.type = VALUE_DOUBLE;
            result.d = DOUBLE_VALUE(v1) * DOUBLE_VALUE(v2);
            LOG("MUL: %0.2f * %0.2f = %0.2f", v1.d, v2.d, result.d);
        }
        else
        {
            result.type = VALUE_INTEGER;
            result.i = v1.i * v2.i;
            LOG("MUL: %lld * %lld = %lld", v1.i, v2.i, result.i);
        }
        context->push(result);
    }
    return true;
}

static bool opcodeDiv(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    if (v1.type == VALUE_OBJECT)
    {
        // TODO: This should have been figured out by the assembler?
        Class* clazz = v1.object->getClass();
        LOG("MUL OBJECT: class=%s", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod("operator/");
        LOG("MUL OBJECT: / operator=%p", addFunc);
        bool res = addFunc->execute(context, v1.object, 1);
        if (!res)
        {
            return false;
        }
    }
    else
    {
        Value v2 = context->pop();
        Value result;
        if (v1.type == VALUE_DOUBLE && v2.type == VALUE_DOUBLE)
        {
            result.type = VALUE_DOUBLE;
            result.d = v1.d / v2.d;
            LOG("DIV: %0.2f / %0.2f = %0.2f", v1.d, v2.d, result.d);
        }
        else if (v1.type == VALUE_DOUBLE || v2.type == VALUE_DOUBLE)
        {
            result.type = VALUE_DOUBLE;
            result.d = DOUBLE_VALUE(v1) / DOUBLE_VALUE(v2);
            LOG("DIV: %0.2f / %0.2f = %0.2f", v1.d, v2.d, result.d);
        }
        else
        {
            result.type = VALUE_INTEGER;
            result.i = v1.i / v2.i;
            LOG("DIV: %lld / %lld = %lld", v1.i, v2.i, result.i);
        }
        context->push(result);
    }
    return true;
}

static bool opcodeAnd(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();

    Value result;
    result.type = VALUE_INTEGER;
    result.i = (v1.i && v2.i);

    LOG("AND: %lld + %lld = %lld", v1.i, v2.i, result.i);

    context->push(result);
return true;
}

static bool opcodeAddI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();
    Value result;
    result.type = VALUE_INTEGER;
    result.i = v1.i + v2.i;
    LOG("ADDI: %lld + %lld = %lld", v1.i, v2.i, result.i);
    context->push(result);
return true;
}

static bool opcodeSubI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();
    Value result;
    result.type = VALUE_INTEGER;
    result.i = v1.i - v2.i;
    LOG("SUBI: %lld - %lld = %lld", v1.i, v2.i, result.i);
    context->push(result);
    return true;
}

static bool opcodeAddD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();
    Value result;
    result.type = VALUE_DOUBLE;
    result.d = v1.d + v2.d;
    LOG("ADDD: %0.2f + %0.2f = %0.2f", v1.d, v2.d, result.d);
    context->push(result);
    return true;
}

static bool opcodeSubD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();
    Value result;
    result.type = VALUE_DOUBLE;
    result.d = v1.d - v2.d;
    LOG("SUBD: %0.2f - %0.2f = %0.2f", v1.d, v2.d, result.d);
    context->push(result);
    return true;
}

static bool opcodeMulI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();
    Value result;
    result.type = VALUE_INTEGER;
    result.i = v1.i * v2.i;
    LOG("MULI: %lld * %lld = %lld", v1.i, v2.i, result.i);
    context->push(result);
    return true;
}


static bool opcodeMulD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();
    Value result;
    result.type = VALUE_DOUBLE;
    result.d = v1.d * v2.d;
    LOG("MULD: %0.2f * %0.2f = %0.2f", v1.d, v2.d, result.d);
    context->push(result);
    return true;
}

static bool opcodePushCE(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_INTEGER;
    v.i = frame->flags.zero;
    context->push(v);
    LOG("PUSHCE: %lld", v.i);
    LOG("PUSHCE:  -> zero=%d, sign=%d, overflow=%d", frame->flags.zero, frame->flags.sign, frame->flags.overflow);
    return true;
}

static bool opcodePushCL(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_INTEGER;
    v.i = (frame->flags.sign != frame->flags.overflow);
    context->push(v);
    LOG("PUSHCL: %lld", v.i);
    LOG("PUSHCL:  -> zero=%d, sign=%d, overflow=%d", frame->flags.zero, frame->flags.sign, frame->flags.overflow);
    return true;
}

static bool opcodePushCLE(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_INTEGER;
    v.i = frame->flags.zero || (frame->flags.sign != frame->flags.overflow);
    context->push(v);
    LOG("PUSHCLE: %lld", v.i);
    LOG("PUSHCLE:  -> zero=%d, sign=%d, overflow=%d", frame->flags.zero, frame->flags.sign, frame->flags.overflow);
    return true;
}

static bool opcodePushCG(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_INTEGER;
    v.i = !frame->flags.zero && (frame->flags.sign == frame->flags.overflow);
    context->push(v);
    LOG("PUSHCG: %lld", v.i);
    LOG("PUSHCG:  -> zero=%d, sign=%d, overflow=%d", frame->flags.zero, frame->flags.sign, frame->flags.overflow);
    return true;
}

static bool opcodePushCGE(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_INTEGER;
    v.i = (frame->flags.sign == frame->flags.overflow);
    context->push(v);
    LOG("PUSHCGE: %lld", v.i);
    LOG("PUSHCGE:  -> zero=%d, sign=%d, overflow=%d", frame->flags.zero, frame->flags.sign, frame->flags.overflow);
    return true;
}


static bool opcodePop(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
#ifdef DEBUG_EXECUTOR
    Value v = context->pop();
    LOG("POP: %s", v.toString().c_str());
#else
    context->pop();
#endif
    return true;
}

static bool opcodeCall(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value objValue = context->pop();
    Function* function = (Function*)frame->fetch();
    int count = frame->fetch();
    LOG("CALL: %p.%p", objValue.object, function);
    return function->execute(context, objValue.object, count);
}
 
static bool opcodeCallStatic(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Function* function = (Function*)frame->fetch();
    int count = frame->fetch();
    LOG("CALL_STATIC: %p", function);
    return function->execute(context, NULL, count);
}

static bool opcodeCallNamed(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int count = frame->fetch();
    Object* nameObj = context->pop().object;
    Value objValue = context->pop();
    string funcName = String::getString(context, nameObj);
    LOG("CALL_NAMED: obj=%s, name=%s", objValue.toString().c_str(), funcName.c_str());
    Object* obj = objValue.object;
    LOG("CALL_NAMED:  -> obj class=%s, ", obj->getClass()->getName().c_str());

    if (B0RK_UNLIKELY(obj == NULL))
    {
        ERROR("CALL_NAMED: object is null! %p", obj);
        return false;
    }
    else if (B0RK_UNLIKELY(obj->getClass() == NULL))
    {
        ERROR("CALL_NAMED: object with no class!? obj=%p", obj);
        return false;
    }

    Function* function;
    int funcFieldId = obj->getClass()->getFieldId(funcName);
    LOG("CALL_NAMED:  -> funcFieldId=%d", funcFieldId);
    if (funcFieldId != -1)
    {
        Value funcFieldValue = obj->getValue(funcFieldId);
        function = (Function*)(funcFieldValue.object->getValue(0).pointer);
    }
    else
    {
        function = obj->getClass()->findMethod(funcName);
    }

    LOG("CALL_NAMED:  -> function=%p", function);
    if (B0RK_UNLIKELY(function == NULL))
    {
        ERROR("CALL_NAMED: function not found! class=%s, function=%s", obj->getClass()->getName().c_str(), funcName.c_str());
        return false;
    }

    return function->execute(context, obj, count);
}

static bool opcodeCallObj(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int count = frame->fetch();

    Value funcObjValue = context->pop();
    if (B0RK_LIKELY(funcObjValue.type == VALUE_OBJECT && funcObjValue.object != NULL))
    {
        LOG("CALL_OBJ: function obj=%p, argCount=%d", funcObjValue.object, count);
        Function* function = (Function*)(funcObjValue.object->getValue(0).pointer);
        if (B0RK_LIKELY(function != NULL))
        {
            LOG("CALL_OBJ: function=%p", function);
            return function->execute(context, NULL, count);
        }
    }
    return false;
}

static bool opcodeNew(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Class* clazz = (Class*)frame->fetch();
    int count = frame->fetch();
    LOG("NEW: class=%s", clazz->getName().c_str());
    Object* obj = context->getRuntime()->newObject(context, clazz, count);
    if (B0RK_LIKELY(obj != NULL))
    {
        Value v;
        v.type = VALUE_OBJECT;
        v.object = obj;
        context->push(v);
    }
    else
    {
        ERROR("NEW: Failed to create new object! class=%s\n", clazz->getName().c_str());
        return false;
    }
    return true;
}

static bool opcodeNewFunction(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Function* func = (Function*)frame->fetch();

    LOG("NEW_FUNCTION: %p", func);

    Class* clazz = context->getRuntime()->findClass(context, "system.lang.Function", false);
    LOG("NEW_FUNCTION: Function class=%p", clazz);

    Value funcValue;
    funcValue.type = VALUE_POINTER;
    funcValue.pointer = func;
    context->push(funcValue);
    Object* funcObject = context->getRuntime()->newObject(context, clazz, 1);
    LOG("NEW_FUNCTION: funcObject=%p", funcObject);

    Value v;
    v.type = VALUE_OBJECT;
    v.object = funcObject;
    context->push(v);

    return true;
}

static bool opcodeReturn(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value result = context->pop();
    LOG("RETURN %s", result.toString().c_str());

    Value frameCheck = context->pop();
    if (frameCheck.type != VALUE_FRAME || frameCheck.pointer != frame)
    {
        ERROR("EXPECTING CURRENT FRAME ON STACK! GOT: %s\n", frameCheck.toString().c_str());
        return false;
    }
    else
    {
        context->push(result);
    }

    frame->running = false;
    return true;
}

#define CMP_INTEGER(_left, _right) \
{ \
    int64_t result = (int64_t)_right.i - (int64_t)_left.i; \
    frame->flags.zero = (result == 0); \
    frame->flags.sign = (result < 0); \
    frame->flags.overflow = !frame->flags.zero || frame->flags.sign; \
    LOG("CMP: left=%lld, right=%lld, result=%lld", _left.i, _right.i, result); \
}

#define CMP_DOUBLE(_left, _right) \
{ \
    double result = _right.d - _left.d; \
    LOG("CMP: left=%0.2f, right=%0.2f, result=%0.2f", _left.d, _right.d, result); \
    frame->flags.zero = (result > -DBL_EPSILON && result < DBL_EPSILON); \
    frame->flags.sign = (result < 0.0 && !frame->flags.zero); \
    frame->flags.overflow = !frame->flags.zero || frame->flags.sign; \
}

static bool opcodeCmp(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value left = context->pop();
    Value right = context->pop();
    if (left.type == VALUE_INTEGER && right.type == VALUE_INTEGER)
    {
        CMP_INTEGER(left, right);
    }
    else if (left.type == VALUE_DOUBLE && right.type == VALUE_DOUBLE)
    {
        CMP_DOUBLE(left, right);
    }
    else if (left.type == VALUE_DOUBLE || right.type == VALUE_DOUBLE)
    {
        double leftDouble = DOUBLE_VALUE(left);
        double rightDouble = DOUBLE_VALUE(right);
        double result = rightDouble - leftDouble;
        LOG("CMP: left=%0.2f, right=%0.2f, result=%0.2f", leftDouble, rightDouble, result);
        frame->flags.zero = (result > -DBL_EPSILON && result < DBL_EPSILON);
        frame->flags.sign = (result < 0.0 && !frame->flags.zero);
        frame->flags.overflow = !frame->flags.zero || frame->flags.sign;
    }
    else
    {
        printf("CMP: UNHANDLED DATA TYPES\n");
        return false;
    }

    LOG("CMP:  -> zero=%d, sign=%d, overflow=%d", frame->flags.zero, frame->flags.sign, frame->flags.overflow);
    return true;
}

static bool opcodeCmpI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value left = context->pop();
    Value right = context->pop();
    CMP_INTEGER(left, right);
 
    return true;
}

static bool opcodeCmpD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value left = context->pop();
    Value right = context->pop();
    CMP_DOUBLE(left, right);
    return true;
}

static bool opcodeJmp(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    uint64_t dest = frame->fetch();
    LOG("JMP: dest=%lld", dest);
    frame->pc = dest;
    return true;
}

static bool opcodeBEq(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    uint64_t dest = frame->fetch();
    LOG("BEQ: frame->flags.zero=%d, dest=0x%llx", frame->flags.zero, dest);
    if (frame->flags.zero)
    {
        frame->pc = dest;
    }
    return true;
}

static bool opcodeBNE(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    uint64_t dest = frame->fetch();
    LOG("BNE: frame->flags.zero=%d, dest=0x%llx", frame->flags.zero, dest);
    if (!frame->flags.zero)
    {
        frame->pc = dest;
    }
    return true;
}

static bool opcodeBL(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    uint64_t dest = frame->fetch();
    LOG("BL: frame->flags.zero=%d, dest=0x%llx", frame->flags.zero, dest);
    if (frame->flags.sign != frame->flags.overflow)
    {
        frame->pc = dest;
    }
    return true;
}

static bool opcodeBLE(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    uint64_t dest = frame->fetch();
    LOG("BLE: flags.sign=%d, flags.overflow=%d, dest=0x%llx", frame->flags.zero, frame->flags.overflow, dest);
    if (!frame->flags.zero || (frame->flags.sign == frame->flags.overflow))
    {
        frame->pc = dest;
    }
    return true;
}

static bool opcodeBG(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    uint64_t dest = frame->fetch();
    LOG("BG: flags.sign=%d, flags.overflow=%d, dest=0x%llx", frame->flags.zero, frame->flags.overflow, dest);
    if (frame->flags.zero || (frame->flags.sign != frame->flags.overflow))
    {
        frame->pc = dest;
    }
    return true;
}

static bool opcodeBGE(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    uint64_t dest = frame->fetch();
    LOG("BGE: flags.sign=%d, flags.overflow=%d, dest=0x%llx", frame->flags.zero, frame->flags.overflow, dest);
    if (frame->flags.sign == frame->flags.overflow)
    {
        frame->pc = dest;
    }
    return true;
}



Executor::Executor()
{
#ifdef DEBUG_EXECUTOR_STATS
    int i;
    for (i = 0; i < OPCODE_MAX; i++)
    {
        g_stats[i] = 0;
    }
#endif
}

Executor::~Executor()
{
#ifdef DEBUG_EXECUTOR_STATS
    int i;
    for (i = 0; i < OPCODE_MAX; i++)
    {
        if (g_stats[i] > 0)
        {
            printf("Executor::~Executor: %04x: %d\n", i, g_stats[i]);
        }
    }
    printf("Executor::~Executor: LoadVar: %lld, this: %lld\n", g_loadVarCount, g_loadVarThisCount);
#endif
}

bool Executor::run(Context* context, Object* thisObj, AssembledCode* code, int argCount)
{
    string functionName = code->function->getFullName();
#ifdef DEBUG_EXECUTOR
    printf("Executor::run: Entering %s\n", code->function->getFullName().c_str());
#endif
    Frame frame;
    frame.pc = 0;
    frame.code = code;
    frame.flags.zero = false;
    frame.flags.sign = false;
    frame.flags.overflow = false;

    int frameSize = sizeof(Frame) * code->localVars;
    frame.localVars = (Value*)alloca(frameSize);
    memset(frame.localVars, 0, frameSize);

    Value thisValue;
    thisValue.type = VALUE_OBJECT;
    thisValue.object = thisObj;
    frame.localVarsCount = code->localVars;
    frame.localVars[0] = thisValue;

    int arg;
    for (arg = 0; arg < argCount; arg++)
    {
        frame.localVars[(argCount - arg) + 0] = context->pop();
    }
    for (; arg < frame.localVarsCount; arg++)
    {
        frame.localVars[arg + 1].type = VALUE_VOID;
    }

    Value frameValue;
    frameValue.type = VALUE_FRAME;
    frameValue.pointer = &frame;
    context->push(frameValue);

    frame.running = true;
    bool success = true;

    while (frame.running && frame.pc < code->size)
    {

        uint64_t thisPC = frame.pc;
        uint64_t opcode = frame.fetch();

#ifdef DEBUG_EXECUTOR_STATS
        g_stats[opcode]++;
#endif

        switch (opcode)
        {
            case OPCODE_LOAD_VAR: success = opcodeLoadVar(thisPC, opcode, context, &frame); break;
            case OPCODE_STORE_VAR: success = opcodeStoreVar(thisPC, opcode, context, &frame); break;
            case OPCODE_LOAD_FIELD: success = opcodeLoadField(thisPC, opcode, context, &frame); break;
            case OPCODE_LOAD_FIELD_NAMED: success = opcodeLoadFieldNamed(thisPC, opcode, context, &frame); break;
            case OPCODE_STORE_FIELD_NAMED: success = opcodeStoreFieldNamed(thisPC, opcode, context, &frame); break;
            case OPCODE_LOAD_STATIC_FIELD: success = opcodeLoadStaticField(thisPC, opcode, context, &frame); break;
            case OPCODE_STORE_FIELD: success = opcodeStoreField(thisPC, opcode, context, &frame); break;
            case OPCODE_LOAD_ARRAY: success = opcodeLoadArray(thisPC, opcode, context, &frame); break;
            case OPCODE_STORE_ARRAY: success = opcodeStoreArray(thisPC, opcode, context, &frame); break;
            case OPCODE_INC_VAR: success = opcodeIncVar(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHI: success = opcodePushI(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHD: success = opcodePushD(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHOBJ: success = opcodePushObj(thisPC, opcode, context, &frame); break;
            case OPCODE_DUP: success = opcodeDup(thisPC, opcode, context, &frame); break;
            case OPCODE_ADD: success = opcodeAdd(thisPC, opcode, context, &frame); break;
            case OPCODE_SUB: success = opcodeSub(thisPC, opcode, context, &frame); break;
            case OPCODE_MUL: success = opcodeMul(thisPC, opcode, context, &frame); break;
            case OPCODE_DIV: success = opcodeDiv(thisPC, opcode, context, &frame); break;
            case OPCODE_AND: success = opcodeAnd(thisPC, opcode, context, &frame); break;
            case OPCODE_ADDI: success = opcodeAddI(thisPC, opcode, context, &frame); break;
            case OPCODE_SUBI: success = opcodeSubI(thisPC, opcode, context, &frame); break;
            case OPCODE_ADDD: success = opcodeAddD(thisPC, opcode, context, &frame); break;
            case OPCODE_SUBD: success = opcodeSubD(thisPC, opcode, context, &frame); break;
            case OPCODE_MULI: success = opcodeMulI(thisPC, opcode, context, &frame); break;
            case OPCODE_MULD: success = opcodeMulD(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHCE: success = opcodePushCE(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHCL: success = opcodePushCL(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHCLE: success = opcodePushCLE(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHCG: success = opcodePushCG(thisPC, opcode, context, &frame); break;
            case OPCODE_PUSHCGE: success = opcodePushCGE(thisPC, opcode, context, &frame); break;
            case OPCODE_POP: success = opcodePop(thisPC, opcode, context, &frame); break;
            case OPCODE_CALL: success = opcodeCall(thisPC, opcode, context, &frame); break;
            case OPCODE_CALL_STATIC: success = opcodeCallStatic(thisPC, opcode, context, &frame); break;
            case OPCODE_CALL_NAMED: success = opcodeCallNamed(thisPC, opcode, context, &frame); break;
            case OPCODE_CALL_OBJ: success = opcodeCallObj(thisPC, opcode, context, &frame); break;
            case OPCODE_NEW: success = opcodeNew(thisPC, opcode, context, &frame); break;
            case OPCODE_NEW_FUNCTION: success = opcodeNewFunction(thisPC, opcode, context, &frame); break;
            case OPCODE_RETURN: success = opcodeReturn(thisPC, opcode, context, &frame); break;
            case OPCODE_CMP: success = opcodeCmp(thisPC, opcode, context, &frame); break;
            case OPCODE_CMPI: success = opcodeCmpI(thisPC, opcode, context, &frame); break;
            case OPCODE_CMPD: success = opcodeCmpD(thisPC, opcode, context, &frame); break;
            case OPCODE_JMP: success = opcodeJmp(thisPC, opcode, context, &frame); break;
            case OPCODE_BEQ: success = opcodeBEq(thisPC, opcode, context, &frame); break;
            case OPCODE_BNE: success = opcodeBNE(thisPC, opcode, context, &frame); break;
            case OPCODE_BL: success = opcodeBL(thisPC, opcode, context, &frame); break;
            case OPCODE_BLE: success = opcodeBLE(thisPC, opcode, context, &frame); break;
            case OPCODE_BG: success = opcodeBG(thisPC, opcode, context, &frame); break;
            case OPCODE_BGE: success = opcodeBGE(thisPC, opcode, context, &frame); break;
            default:
                fprintf(stderr, "Executor::run: %s:%04llx: 0x%llx ERROR: Unknown opcode\n", frame.code->function->getFullName().c_str(), thisPC, opcode);
                success = false;
        }

        if (B0RK_UNLIKELY(!success))
        {
            frame.running = false;
        }
    }

    //context->getRuntime()->gc();

#ifdef DEBUG_EXECUTOR
    printf("Executor::run: Leaving %s\n", code->function->getFullName().c_str());
#endif

    return success;
}


