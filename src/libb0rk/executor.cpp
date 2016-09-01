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
#include <b0rk/utils.h>
#include "packages/system/lang/StringClass.h"
#include "packages/system/lang/Array.h"
#include "packages/system/lang/Exception.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <cinttypes>

#undef DEBUG_EXECUTOR
#undef DEBUG_EXECUTOR_STATS

#define DOUBLE_VALUE(_v) (((_v).type == VALUE_DOUBLE) ? (_v.d) : (double)(_v.i))
#define INTEGER_VALUE(_v) (((_v).type == VALUE_INTEGER) ? (_v.i) : (int)(floor(_v.d)))

#ifdef DEBUG_EXECUTOR
#define LOG(_fmt, _args...) \
    printf("Executor::run: %ls:%04" PRIx64 ": 0x%" PRIx64 " " _fmt "\n", frame->code->function->getFullName().c_str(), thisPC, opcode, _args);
#else
#define LOG(_fmt, _args...)
#endif

#define ERROR(_fmt, _args...) \
    printf("Executor::run: %ls:%04" PRIx64 ": 0x%" PRIx64 " ERROR: " _fmt "\n", frame->code->function->getFullName().c_str(), thisPC, opcode, _args);

using namespace std;
using namespace b0rk;

#ifdef DEBUG_EXECUTOR_STATS
int g_stats[OPCODE_MAX];

uint64_t g_loadVarCount = 0;
uint64_t g_loadVarThisCount = 0;
#endif

static bool opcodeLoadVar(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int varId = frame->fetch();
    context->push(frame->localVars[varId]);
    LOG("LOAD_VAR: v%d: %ls", varId, frame->localVars[varId].toString().c_str());
#ifdef DEBUG_EXECUTOR_STATS
    g_loadVarCount++;
    if (varId == 0)
    {
        g_loadVarThisCount++;
    }
#endif
    return true;
}

static bool opcodeStoreVar(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int varId = frame->fetch();
    frame->localVars[varId] = context->pop();
    LOG("STORE_VAR: v%d = %ls", varId, frame->localVars[varId].toString().c_str());

    return true;
}

static bool opcodeLoadField(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value objValue = context->pop();
    int fieldId = frame->fetch();
    LOG("LOAD_FIELD: f%d, this=%p (value count=%lu)", fieldId, objValue.object, objValue.object->getClass()->getFieldCount());
    Value result = objValue.object->getValue(fieldId);
    context->push(result);
    return true;
}

static bool opcodeLoadFieldNamed(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Object* nameObj = context->pop().object;
    Value objValue = context->pop();
    wstring varName = String::getString(context, nameObj);
    LOG("LOAD_FIELD_NAMED: obj=%p, name=%ls\n", objValue.object, varName.c_str());

    if (objValue.type == VALUE_OBJECT && objValue.object != NULL)
    {
        Object* obj = objValue.object;
        Class* clazz = obj->getClass();

        int id = clazz->getFieldId(varName);
        if (id != -1)
        {

            LOG("LOAD_FIELD_NAMED: class=%ls, obj=%p, name=%ls, id=%d\n", clazz->getName().c_str(), objValue.object, varName.c_str(), id);

            Value v = obj->getValue(id);
            context->push(v);
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
    wstring varName = String::getString(context, nameValue);
    LOG("STORE_FIELD_NAMED: obj=%p, name=%ls\n", objValue.object, varName.c_str());

    if (objValue.type == VALUE_OBJECT && objValue.object != NULL)
    {
        Object* obj = objValue.object;
        Class* clazz = obj->getClass();

        int id = clazz->getFieldId(varName);
        if (id != -1)
        {
            LOG("STORE_FIELD_NAMED: class=%ls, obj=%p, name=%ls, id=%d\n", clazz->getName().c_str(), objValue.object, varName.c_str(), id);
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

    LOG("LOAD_STATIC_FIELD: f%d, class=%ls, v=%ls", fieldId, clazz->getName().c_str(), v.toString().c_str());

    context->push(v);

    return true;
}

static bool opcodeStoreStaticField(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Class* clazz = (Class*)frame->fetch();
    int fieldId = frame->fetch();

    Value v = context->pop();

    clazz->setStaticField(fieldId, v);

    LOG("STORE_STATIC_FIELD: f%d, class=%ls, v=%ls", fieldId, clazz->getName().c_str(), v.toString().c_str());

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

    LOG("LOAD_ARRAY: indexValue=%ls", indexValue.toString().c_str());
    LOG("LOAD_ARRAY: arrayValue=%ls", arrayValue.toString().c_str());

    bool res = false;
    if (arrayValue.type == VALUE_OBJECT && arrayValue.object != NULL)
    {
        Object* array = arrayValue.object;
        Value valueValue;
        res = ((Array*)array->m_class)->load(context, array, indexValue, valueValue);
        context->push(valueValue);
    }
    else
    {
        ERROR("STORE_ARRAY: Array is invalid: %ls", arrayValue.toString().c_str());
    }

    return res;
}

static bool opcodeStoreArray(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value indexValue = context->pop();
    Value arrayValue = context->pop();
    Value valueValue = context->pop();

    LOG("STORE_ARRAY: indexValue=%ls", indexValue.toString().c_str());
    LOG("STORE_ARRAY: arrayValue=%ls", arrayValue.toString().c_str());
    LOG("STORE_ARRAY: valueValue=%ls", valueValue.toString().c_str());

    bool res = false;
    if (arrayValue.type == VALUE_OBJECT && arrayValue.object != NULL)
    {
        Object* array = arrayValue.object;
        res = ((Array*)array->m_class)->store(context, array, indexValue, valueValue);
    }
    else
    {
        ERROR("STORE_ARRAY: Array is invalid: %ls", arrayValue.toString().c_str());
    }

    return res;
}

static bool opcodeIncVar(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int varId = frame->fetch();
    int64_t amount = frame->fetch();
    frame->localVars[varId].i += amount;

    LOG("INC_VAR: v%d, %" PRId64 ": %" PRId64, varId, amount, frame->localVars[varId].i);

    return true;
}

static bool opcodeIncVarD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int varId = frame->fetch();

    uint64_t amounti = frame->fetch();
    double* amount = (double*)(&amounti);
    frame->localVars[varId].d += *amount;

    LOG("INC_VARD: v%d, %" PRId64 ": %" PRIf64, varId, amount, frame->localVars[varId].d);

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
    LOG("PUSHOBJ: %p", obj);
    context->push(v);
    return true;
}


static bool opcodeDup(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v = context->peek();
    LOG("DUP: %ls", v.toString().c_str());
    context->push(v);
    return true;
}

static bool opcodeSwap(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    Value v2 = context->pop();
    LOG("SWAP: %ls, %ls", v1.toString().c_str(), v2.toString().c_str());
    context->push(v1);
    context->push(v2);
    return true;
}

static bool opcodeAdd(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();
    if (v1.type == VALUE_OBJECT)
    {
        // TODO: This should have been figured out by the assembler?
        Class* clazz = v1.object->getClass();
        LOG("ADD OBJECT: class=%ls", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod(L"operator+");
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
        LOG("SUB OBJECT: class=%ls", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod(L"operator-");
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
        LOG("MUL OBJECT: class=%ls", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod(L"operator*");
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
        LOG("MUL OBJECT: class=%ls", clazz->getName().c_str());
        Function* addFunc = clazz->findMethod(L"operator/");
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

static bool opcodeNot(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v1 = context->pop();

    Value result;
    result.type = VALUE_INTEGER;
    result.i = !v1.i;

    LOG("NOT: %lld = %lld", v1.i, result.i);

    context->push(result);
    return true;
}


static bool opcodeAddI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int64_t v1 = context->popInt();
    int64_t v2 = context->popInt();
    int64_t result = v1 + v2;
    LOG("ADDI: %lld + %lld = %lld", v1, v2, result);
    context->pushInt(result);
    return true;
}

static bool opcodeSubI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int64_t v1 = context->popInt();
    int64_t v2 = context->popInt();
    int64_t result = v1 - v2;
    LOG("SUBI: %lld - %lld = %lld", v1, v2, result);
    context->pushInt(result);
    return true;
}

static bool opcodeAddD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    double v1 = context->popDouble();
    double v2 = context->popDouble();
    double result = v1 + v2;
    LOG("ADDD: %0.2f + %0.2f = %0.2f", v1 v2, result);
    context->pushDouble(result);
    return true;
}

static bool opcodeSubD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    double v1 = context->popDouble();
    double v2 = context->popDouble();
    double result = v1 - v2;
    LOG("SUBD: %0.2f - %0.2f = %0.2f", v1, v2, result);
    context->pushDouble(result);
    return true;
}

static bool opcodeMulI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int64_t v1 = context->popInt();
    int64_t v2 = context->popInt();
    int64_t result = v1 * v2;
    LOG("MULI: %lld * %lld = %lld", v1, v2, result);
    context->pushInt(result);
    return true;
}

static bool opcodeDivI(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    int64_t v1 = context->popInt();
    int64_t v2 = context->popInt();
    int64_t result = v1 / v2;
    LOG("DIVI: %lld * %lld = %lld", v1, v2, result);
    context->pushInt(result);
    return true;
}

static bool opcodeMulD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    double v1 = context->popDouble();
    double v2 = context->popDouble();
    double result = v1 * v2;
    LOG("MULD: %0.2f * %0.2f = %0.2f", v1, v2, result);
    context->pushDouble(result);
    return true;
}

static bool opcodeDivD(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    double v1 = context->popDouble();
    double v2 = context->popDouble();
    double result = v1 / v2;
    LOG("DIVD: %0.2f * %0.2f = %0.2f", v1, v2, result);
    context->pushDouble(result);
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

static bool opcodePushCNE(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value v;
    v.type = VALUE_INTEGER;
    v.i = !frame->flags.zero;
    context->push(v);
    LOG("PUSHCNE: %lld", v.i);
    LOG("PUSHCNE:  -> zero=%d, sign=%d, overflow=%d", frame->flags.zero, frame->flags.sign, frame->flags.overflow);
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
    LOG("POP: %ls", v.toString().c_str());
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
    wstring funcName = String::getString(context, nameObj);
    LOG("CALL_NAMED: obj=%ls, name=%ls", objValue.toString().c_str(), funcName.c_str());
    Object* obj = objValue.object;
    LOG("CALL_NAMED:  -> obj class=%ls, ", obj->getClass()->getName().c_str());

    if (B0RK_UNLIKELY(obj == NULL))
    {
        ERROR("CALL_NAMED: object is null! %p", obj);
        return false;
    }

    Class* clazz = obj->getClass();
    if (B0RK_UNLIKELY(clazz == NULL))
    {
        ERROR("CALL_NAMED: object with no class!? obj=%p", obj);
        return false;
    }

    int funcFieldId = clazz->getFieldId(funcName);
    LOG("CALL_NAMED:  -> funcFieldId=%d", funcFieldId);
    Function* function;
    if (funcFieldId != -1)
    {
        Value funcFieldValue = obj->getValue(funcFieldId);
        function = (Function*)(funcFieldValue.object->getValue(0).pointer);
    }
    else
    {
        function = clazz->findMethod(funcName);
    }

    LOG("CALL_NAMED:  -> function=%p", function);
    if (B0RK_UNLIKELY(function == NULL))
    {
        ERROR("CALL_NAMED: function not found! class=%ls, function=%ls", obj->getClass()->getName().c_str(), funcName.c_str());
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
    LOG("NEW: class=%ls", clazz->getName().c_str());
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
        ERROR("NEW: Failed to create new object! class=%ls", clazz->getName().c_str());
        return false;
    }
    return true;
}

static bool opcodeNewFunction(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Function* func = (Function*)frame->fetch();

    LOG("NEW_FUNCTION: %p", func);

    Class* clazz = context->getRuntime()->findClass(context, L"system.lang.Function", false);
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
    LOG("RETURN %ls", result.toString().c_str());

    Value frameCheck = context->pop();
    if (frameCheck.type != VALUE_FRAME || frameCheck.pointer != frame)
    {
        ERROR("EXPECTING CURRENT FRAME ON STACK! GOT: %ls\n", frameCheck.toString().c_str());
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
    if (left.type == VALUE_INTEGER || left.type == VALUE_DOUBLE)
    {
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
            // Incompatible types: They always get a result of -1
            int result = -1;
            frame->flags.zero = (result == 0);
            frame->flags.sign = result < 0 && result != 0;
            frame->flags.overflow = !frame->flags.zero || frame->flags.sign;
        }
    }
    else if (left.type == VALUE_OBJECT)
    {
        // TODO: This should have been figured out by the assembler?
        Class* clazz = left.object->getClass();
        LOG("CMP OBJECT: class=%ls", clazz->getName().c_str());
        Function* eqFunc = clazz->findMethod(L"operator==");
        LOG("CMP OBJECT: cmp operator=%p", eqFunc);
        bool res = eqFunc->execute(context, left.object, 1);
        if (!res)
        {
            return false;
        }

        Value result = context->pop();
        frame->flags.zero = (result.i == 0);
        frame->flags.sign = result.i < 0 && result.i != 0;
        frame->flags.overflow = !frame->flags.zero || frame->flags.sign;
    }
    else
    {
        ERROR("CMP: UNHANDLED DATA TYPES: left=%d\n", left.type);
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
    if (!frame->flags.zero &&(frame->flags.sign == frame->flags.overflow))
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

static bool opcodeThrow(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    Value value = context->pop();
    LOG("THROW: value=%ls", value.toString().c_str());
    Object* exception = Exception::createException(context, value);
    context->throwException(exception);
 
    return true;
}

static bool opcodePushHandler(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    ExceptionHandler handler;
    handler.excepVar = frame->fetch();
    handler.handlerPC = frame->fetch();
    frame->handlerStack.push_back(handler);
    return true;
}

static bool opcodePopHandler(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    frame->handlerStack.pop_back();
    return true;
}

static bool opcodeInvalid(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    ERROR("Invalid Opcode: 0x%" PRIx64, opcode);
    return false;
}

static opcodeFunc_t g_opcodeTable[OPCODE_MAX];

Executor::Executor()
{
    int i;
    for (i = 0; i < OPCODE_MAX; i++)
    {
        g_opcodeTable[i] = opcodeInvalid;
    }

    g_opcodeTable[OPCODE_LOAD_VAR] = opcodeLoadVar;
    g_opcodeTable[OPCODE_STORE_VAR] = opcodeStoreVar;
    g_opcodeTable[OPCODE_LOAD_FIELD] = opcodeLoadField;
    g_opcodeTable[OPCODE_STORE_FIELD] = opcodeStoreField;
    g_opcodeTable[OPCODE_LOAD_FIELD_NAMED] = opcodeLoadFieldNamed;
    g_opcodeTable[OPCODE_STORE_FIELD_NAMED] = opcodeStoreFieldNamed;
    g_opcodeTable[OPCODE_LOAD_STATIC_FIELD] = opcodeLoadStaticField;
    g_opcodeTable[OPCODE_STORE_STATIC_FIELD] = opcodeStoreStaticField;
    g_opcodeTable[OPCODE_LOAD_ARRAY] = opcodeLoadArray;
    g_opcodeTable[OPCODE_STORE_ARRAY] = opcodeStoreArray;
    g_opcodeTable[OPCODE_INC_VAR] = opcodeIncVar;
    g_opcodeTable[OPCODE_INC_VARD] = opcodeIncVarD;
    g_opcodeTable[OPCODE_PUSHI] = opcodePushI;
    g_opcodeTable[OPCODE_PUSHD] = opcodePushD;
    g_opcodeTable[OPCODE_PUSHOBJ] = opcodePushObj;
    g_opcodeTable[OPCODE_DUP] = opcodeDup;
    g_opcodeTable[OPCODE_SWAP] = opcodeSwap;
    g_opcodeTable[OPCODE_ADD] = opcodeAdd;
    g_opcodeTable[OPCODE_SUB] = opcodeSub;
    g_opcodeTable[OPCODE_MUL] = opcodeMul;
    g_opcodeTable[OPCODE_DIV] = opcodeDiv;
    g_opcodeTable[OPCODE_AND] = opcodeAnd;
    g_opcodeTable[OPCODE_NOT] = opcodeNot;

    g_opcodeTable[OPCODE_ADDI] = opcodeAddI;
    g_opcodeTable[OPCODE_SUBI] = opcodeSubI;
    g_opcodeTable[OPCODE_MULI] = opcodeMulI;
    g_opcodeTable[OPCODE_DIVI] = opcodeDivI;
    g_opcodeTable[OPCODE_ANDI] = opcodeAnd;
    g_opcodeTable[OPCODE_NOTI] = opcodeNot;

    g_opcodeTable[OPCODE_ADDD] = opcodeAddD;
    g_opcodeTable[OPCODE_SUBD] = opcodeSubD;
    g_opcodeTable[OPCODE_MULD] = opcodeMulD;
    g_opcodeTable[OPCODE_DIVD] = opcodeDivD;

    g_opcodeTable[OPCODE_PUSHCE] = opcodePushCE;
    g_opcodeTable[OPCODE_PUSHCNE] = opcodePushCNE;
    g_opcodeTable[OPCODE_PUSHCL] = opcodePushCL;
    g_opcodeTable[OPCODE_PUSHCLE] = opcodePushCLE;
    g_opcodeTable[OPCODE_PUSHCG] = opcodePushCG;
    g_opcodeTable[OPCODE_PUSHCGE] = opcodePushCGE;
    g_opcodeTable[OPCODE_POP] = opcodePop;
    g_opcodeTable[OPCODE_CALL] = opcodeCall;
    g_opcodeTable[OPCODE_CALL_STATIC] = opcodeCallStatic;
    g_opcodeTable[OPCODE_CALL_NAMED] = opcodeCallNamed;
    g_opcodeTable[OPCODE_CALL_OBJ] = opcodeCallObj;
    g_opcodeTable[OPCODE_NEW] = opcodeNew;
    g_opcodeTable[OPCODE_NEW_FUNCTION] = opcodeNewFunction;
    g_opcodeTable[OPCODE_RETURN] = opcodeReturn;
    g_opcodeTable[OPCODE_CMP] = opcodeCmp;
    g_opcodeTable[OPCODE_CMPI] = opcodeCmpI;
    g_opcodeTable[OPCODE_CMPD] = opcodeCmpD;
    g_opcodeTable[OPCODE_JMP] = opcodeJmp;
    g_opcodeTable[OPCODE_BEQ] = opcodeBEq;
    g_opcodeTable[OPCODE_BNE] = opcodeBNE;
    g_opcodeTable[OPCODE_BL] = opcodeBL;
    g_opcodeTable[OPCODE_BLE] = opcodeBLE;
    g_opcodeTable[OPCODE_BG] = opcodeBG;
    g_opcodeTable[OPCODE_BGE] = opcodeBGE;
    g_opcodeTable[OPCODE_THROW] = opcodeThrow;
    g_opcodeTable[OPCODE_PUSHTRY] = opcodePushHandler;
    g_opcodeTable[OPCODE_POPTRY] = opcodePopHandler;

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
#ifdef DEBUG_EXECUTOR
    wstring functionName = code->function->getFullName();
    printf("Executor::run: Entering %ls\n", functionName.c_str());
#endif
    Frame frame;
    frame.pc = 0;
    frame.code = code;
    frame.flags.zero = false;
    frame.flags.sign = false;
    frame.flags.overflow = false;

    // Allocate local vars on our real stack
    // This is UNSAFE. It could allow b0rk code
    // to hijack the interpreter
    int frameSize = sizeof(Value) * code->localVars;
    frame.localVars = (Value*)alloca(frameSize);
    memset(frame.localVars, 0, frameSize);

    // v0 is always the "this" pointer
    Value thisValue;
    thisValue.type = VALUE_OBJECT;
    thisValue.object = thisObj;
    frame.localVarsCount = code->localVars;
    frame.localVars[0] = thisValue;

    // Populate the first n variables with any arguments
    int arg;
    for (arg = 0; arg < argCount; arg++)
    {
        frame.localVars[(argCount - arg) + 0] = context->pop();
    }

    /*
     * Push the frame pointer to the stack
     * This is used for a variety of things:
     * - By the Garbage Collector looking for local vars
     * - Generating stack traces
     * - Ensuring the stack is consistent
    */
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

        if (B0RK_UNLIKELY(!context->checkStack()))
        {
            fprintf(
                stderr,
                "Executor::run: %ls:%04" PRIx64 ": Stack position is out of bounds",
                frame.code->function->getFullName().c_str(),
                thisPC);
            return false;
        }

#ifdef DEBUG_EXECUTOR_STATS
        g_stats[opcode]++;
#endif

        opcodeFunc_t func = opcodeInvalid;

        if (B0RK_LIKELY(opcode < OPCODE_MAX))
        {
            func = g_opcodeTable[opcode];
        }

        success = func(thisPC, opcode, context, &frame);
        if (B0RK_UNLIKELY(!success))
        {
            frame.running = false;
        }

        // Check for any exceptions
        if (B0RK_UNLIKELY(context->hasException()))
        {
            success = handleException(thisPC, opcode, context, &frame);
        }
    }

#ifdef DEBUG_EXECUTOR
    printf("Executor::run: Leaving %ls\n", code->function->getFullName().c_str());
#endif

    return success;
}

bool Executor::handleException(uint64_t thisPC, uint64_t opcode, Context* context, Frame* frame)
{
    bool success = true;
#ifdef DEBUG_EXECUTOR
    fprintf(
        stderr,
        "Executor::handleException: %ls:%04" PRIx64 ": 0x%" PRIx64 " ERROR: Exception thrown!\n",
        frame->code->function->getFullName().c_str(),
        thisPC,
        opcode);
#endif

    if (frame->handlerStack.empty())
    {
        // There are no current exception handlers
#ifdef DEBUG_EXECUTOR
        fprintf(
            stderr,
            "Executor::handleException: %ls:%04" PRIx64 ": 0x%" PRIx64 " -> No handler, leaving function!\n",
            frame->code->function->getFullName().c_str(),
            thisPC,
            opcode);
#endif
        success = true;
        frame->running = false;

        // Clear the stack until we find our current frame
        while (true)
        {
            Value v = context->pop();
#ifdef DEBUG_EXECUTOR
            fprintf(stderr, "Executor::handleException: Popping frame entry: %ls\n", v.toString().c_str());
#endif
            if (v.type == VALUE_FRAME)
            {
                if (v.pointer != frame)
                {
                    fprintf(stderr, "Executor::handleException: ERROR: Frame is not our frame!?\n");
                }
                break;
            }
        }
    }
    else
    {
        // There is an exception handler!
#ifdef DEBUG_EXECUTOR
        fprintf(
            stderr,
            "Executor::handleException: %ls:%04" PRIx64 ": 0x%" PRIx64 " -> Found handler!\n",
            frame->code->function->getFullName().c_str(),
            thisPC,
            opcode);
#endif
        ExceptionHandler handler = frame->handlerStack.back();
        frame->handlerStack.pop_back();
        context->clearException();

        frame->pc = handler.handlerPC;
        frame->localVars[handler.excepVar] = context->getExceptionValue();

        context->getExceptionValue();
    }
    return success;
}
