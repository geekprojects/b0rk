
#include <b0rk/executor.h>
#include "packages/system/lang/StringClass.h"
#include "packages/system/lang/Array.h"

#include <stdlib.h>
#include <math.h>
#include <float.h>

#undef DEBUG_EXECUTOR

#define DOUBLE_VALUE(_v) (((_v).type == VALUE_DOUBLE) ? (_v.d) : (double)(_v.i))
#define INTEGER_VALUE(_v) (((_v).type == VALUE_INTEGER) ? (_v.i) : (int)(floor(_v.d)))

using namespace std;
using namespace b0rk;

Executor::Executor()
{
}

#ifdef DEBUG_EXECUTOR
#define LOG(_fmt, _args...) \
    printf("Executor::run: %04llx: 0x%llx " _fmt "\n", thisPC, opcode, _args);
#else
#define LOG(_fmt, _args...)
#endif

#define ERROR(_fmt, _args...) \
    printf("Executor::run: %04llx: 0x%llx ERROR: " _fmt "\n", thisPC, opcode, _args);

bool Executor::run(Context* context, Object* thisObj, AssembledCode& code, int argCount)
{
#ifdef DEBUG_EXECUTOR
    printf("Executor::run: Entering %s\n", code.function->getFullName().c_str());
#endif
    size_t pc = 0;
    bool res;

    Frame frame;
    int frameSize = sizeof(Frame) * code.localVars;
    frame.localVars = (Value*)alloca(frameSize);
    memset(frame.localVars, 0, frameSize);

    bool flagZero = false;
    bool flagSign = false;
    bool flagOverflow = false;

    bool running = true;
    bool success = true;

    int arg;

    Value thisValue;
    thisValue.type = VALUE_OBJECT;
    thisValue.object = thisObj;
    frame.localVarsCount = code.localVars;
    frame.localVars[0] = thisValue;

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

    while (running && pc < code.size)
    {
        uint64_t thisPC = pc;
        uint64_t opcode = code.code[pc++];
        success = false;
        switch (opcode)
        {
            case OPCODE_LOAD_VAR:
            {
                int varId = code.code[pc++];
                context->push(frame.localVars[varId]);
                LOG("LOAD_VAR: v%d: %s", varId, frame.localVars[varId].toString().c_str());
            } break;

            case OPCODE_STORE_VAR:
            {
                int varId = code.code[pc++];
                frame.localVars[varId] = context->pop();
                LOG("STORE_VAR: v%d = %s", varId, frame.localVars[varId].toString().c_str());
            } break;

            case OPCODE_LOAD_FIELD:
            {
                Value objValue = context->pop();
                int fieldId = code.code[pc++];
                LOG("LOAD_FIELD: f%d, this=%p (value count=%ld)", fieldId, objValue.object, objValue.object->getClass()->getFieldCount());
                Value result = objValue.object->getValue(fieldId);
                context->push(result);
            } break;

            case OPCODE_LOAD_STATIC_FIELD:
            {
                Class* clazz = (Class*)code.code[pc++];
                int fieldId = code.code[pc++];

                Value v = clazz->getStaticField(fieldId);

                LOG("LOAD_STATIC_FIELD: f%d, class=%s, v=%s", fieldId, clazz->getName().c_str(), v.toString().c_str());

                context->push(v);
            } break;

            case OPCODE_STORE_FIELD:
            {
                Value objValue = context->pop();
                Value value = context->pop();
                int fieldId = code.code[pc++];
                LOG("STORE_FIELD: f%d, this=%p", fieldId, objValue.object);
                if (objValue.object == NULL)
                {
                    ERROR("STORE_FIELD: f%d, this=%p", fieldId, objValue.object);
                    running = false;
                    success = false;
                    continue;
                }

                objValue.object->setValue(fieldId, value);
            } break;

            case OPCODE_LOAD_ARRAY:
            {
                Value indexValue = context->pop();
                Value arrayValue = context->pop();

                LOG("STORE_ARRAY: indexValue=%s", indexValue.toString().c_str());
                LOG("STORE_ARRAY: arrayValue=%s", arrayValue.toString().c_str());

                res = false;
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

                if (!res)
                {
                    running = false;
                    success = false;
                }
            } break;

            case OPCODE_STORE_ARRAY:
            {
                Value indexValue = context->pop();
                Value arrayValue = context->pop();
                Value valueValue = context->pop();

                LOG("STORE_ARRAY: indexValue=%s", indexValue.toString().c_str());
                LOG("STORE_ARRAY: arrayValue=%s", arrayValue.toString().c_str());
                LOG("STORE_ARRAY: valueValue=%s", valueValue.toString().c_str());

                res = false;
                if (arrayValue.type == VALUE_OBJECT && arrayValue.object != NULL)
                {
                    Object* array = arrayValue.object;
                    res = Array::store(array, indexValue, valueValue);
                }
                else
                {
                    ERROR("STORE_ARRAY: Array is invalid: %s", arrayValue.toString().c_str());
                }

                if (!res)
                {
                    running = false;
                    success = false;
                }
            } break;

            case OPCODE_PUSHI:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = code.code[pc++];
                LOG("PUSHI: %lld", v.i);
                context->push(v);
            } break;

            case OPCODE_PUSHD:
            {
                Value v;
                v.type = VALUE_DOUBLE;
                double* d = (double*)(&(code.code[pc++]));
                v.d = *d;
                LOG("PUSHD: %0.2f", v.d);
                context->push(v);
            } break;

            case OPCODE_DUP:
            {
                Value v;
                v = context->pop();
                LOG("DUP: %s", v.toString().c_str());
                context->push(v);
                context->push(v);
            } break;;

            case OPCODE_ADD:
            {
                Value v1 = context->pop();
                if (v1.type == VALUE_OBJECT)
                {
                    // TODO: This should have been figured out by the assembler?
                    Class* clazz = v1.object->getClass();
                    LOG("ADD OBJECT: class=%s", clazz->getName().c_str());
                    Function* addFunc = clazz->findMethod("operator+");
                    LOG("ADD OBJECT: add operator=%p", addFunc);
                    res = addFunc->execute(context, v1.object, 1);
                    if (!res)
                    {
                        running = false;
                        success = false;
                    }
                }
                else
                {
                    Value v2 = context->pop();
                    Value result;
                    if (v1.type == VALUE_DOUBLE || v2.type == VALUE_DOUBLE)
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
            } break;

            case OPCODE_SUB:
            {
                Value v1 = context->pop();
                if (v1.type == VALUE_OBJECT)
                {
                    // TODO: This should have been figured out by the assembler?
                    Class* clazz = v1.object->getClass();
                    LOG("SUB OBJECT: class=%s", clazz->getName().c_str());
                    Function* addFunc = clazz->findMethod("operator-");
                    LOG("SUB OBJECT: sub operator=%p", addFunc);
                    res = addFunc->execute(context, v1.object, 1);
                    if (!res)
                    {
                        running = false;
                        success = false;
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
            } break;


            case OPCODE_MUL:
            {
                Value v1 = context->pop();
                if (v1.type == VALUE_OBJECT)
                {
                    // TODO: This should have been figured out by the assembler?
                    Class* clazz = v1.object->getClass();
                    LOG("MUL OBJECT: class=%s", clazz->getName().c_str());
                    Function* addFunc = clazz->findMethod("operator*");
                    LOG("MUL OBJECT: * operator=%p", addFunc);
                    res = addFunc->execute(context, v1.object, 1);
                    if (!res)
                    {
                        running = false;
                        success = false;
                    }
                }
                else
                {
                    Value v2 = context->pop();
                    Value result;
                    if (v1.type == VALUE_DOUBLE || v2.type == VALUE_DOUBLE)
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
            } break;


            case OPCODE_AND:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();

                Value result;
                result.type = VALUE_INTEGER;
                result.i = (v1.i && v2.i);

                LOG("AND: %lld + %lld = %lld", v1.i, v2.i, result.i);

                context->push(result);
            } break;

            case OPCODE_ADDI:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_INTEGER;
                result.i = v1.i + v2.i;
                LOG("ADDI: %lld + %lld = %lld", v1.i, v2.i, result.i);
                context->push(result);
            } break;

            case OPCODE_SUBI:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_INTEGER;
                result.i = v1.i - v2.i;
                LOG("SUBI: %lld - %lld = %lld", v1.i, v2.i, result.i);
                context->push(result);
            } break;

            case OPCODE_ADDD:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_DOUBLE;
                result.d = v1.d + v2.d;
                LOG("ADDD: %0.2f + %0.2f = %0.2f", v1.d, v2.d, result.d);
                context->push(result);
            } break;

            case OPCODE_SUBD:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_DOUBLE;
                result.d = v1.d - v2.d;
                LOG("SUBD: %0.2f - %0.2f = %0.2f", v1.d, v2.d, result.d);
                context->push(result);
            } break;

            case OPCODE_MULI:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_INTEGER;
                result.i = v1.i * v2.i;
                LOG("MULI: %lld * %lld = %lld", v1.i, v2.i, result.i);
                context->push(result);
            } break;


            case OPCODE_MULD:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_DOUBLE;
                result.d = v1.d * v2.d;
                LOG("MULD: %0.2f * %0.2f = %0.2f", v1.d, v2.d, result.d);
                context->push(result);
            } break;

            case OPCODE_PUSHCE:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = flagZero;
                context->push(v);
                LOG("PUSHCE: %lld", v.i);
                LOG("PUSHCE:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_PUSHCL:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = (flagSign != flagOverflow);
                context->push(v);
                LOG("PUSHCL: %lld", v.i);
                LOG("PUSHCL:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_PUSHCLE:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = flagZero || (flagSign != flagOverflow);
                context->push(v);
                LOG("PUSHCLE: %lld", v.i);
                LOG("PUSHCLE:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_PUSHCG:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = !flagZero && (flagSign == flagOverflow);
                context->push(v);
                LOG("PUSHCG: %lld", v.i);
                LOG("PUSHCG:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_PUSHCGE:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = (flagSign == flagOverflow);
                context->push(v);
                LOG("PUSHCGE: %lld", v.i);
                LOG("PUSHCGE:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
            } break;


            case OPCODE_POP:
            {
#ifdef DEBUG_EXECUTOR
                Value v = context->pop();
                LOG("POP: %s", v.toString().c_str());
#else
                context->pop();
#endif
            } break;

            case OPCODE_CALL:
            {
                Value objValue = context->pop();
                Function* function = (Function*)code.code[pc++];
                int count = code.code[pc++];
                LOG("CALL: %p.%p", objValue.object, function);
                res = function->execute(context, objValue.object, count);
                if (!res)
                {
                    running = false;
                    success = false;
                }
            } break;

 
            case OPCODE_CALL_STATIC:
            {
                Function* function = (Function*)code.code[pc++];
                int count = code.code[pc++];
                LOG("CALL: %p", function);
                res = function->execute(context, NULL, count);
                if (!res)
                {
                    running = false;
                    success = false;
                }
            } break;

            case OPCODE_CALL_NAMED:
            {
                int count = code.code[pc++];
                Object* nameObj = context->pop().object;
                Value objValue = context->pop();
                string funcName = String::getString(context, nameObj);
                LOG("CALL_NAMED: obj=%s, name=%s", objValue.toString().c_str(), funcName.c_str());
                Object* obj = objValue.object;
                LOG("CALL_NAMED:  -> obj class=%s, ", obj->getClass()->getName().c_str());

                if (obj == NULL)
                {
                    ERROR("CALL_NAMED: object is null! %p", obj);
                    running = false;
                    success = false;
                    continue;
                }
                else if (obj->getClass() == NULL)
                {
                    ERROR("CALL_NAMED: object with no class!? obj=%p", obj);
                    running = false;
                    success = false;
                    continue;
                }

                res = false;
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
                if (function == NULL)
                {
                    ERROR("CALL_NAMED: function not found! class=%s, function=%s", obj->getClass()->getName().c_str(), funcName.c_str());
                    running = false;
                    success = false;
                    continue;
                }
                res = function->execute(context, obj, count);
                if (!res)
                {
                    running = false;
                    success = false;
                }
            } break;

            case OPCODE_CALL_OBJ:
            {
                int count = code.code[pc++];
                res = false;

                Value funcObjValue = context->pop();
                if (funcObjValue.type == VALUE_OBJECT && funcObjValue.object != NULL)
                {
                    LOG("CALL_OBJ: function obj=%p, argCount=%d", funcObjValue.object, count);
                    Function* function = (Function*)(funcObjValue.object->getValue(0).pointer);
                    if (function != NULL)
                    {
                        LOG("CALL_OBJ: function=%p", function);
                        res = function->execute(context, NULL, count);
                    }
                }
                if (!res)
                {
                    running = false;
                    success = false;
                }
            } break;

            case OPCODE_NEW:
            {
                Class* clazz = (Class*)code.code[pc++];
                int count = code.code[pc++];
                LOG("NEW: class=%s", clazz->getName().c_str());
                Object* obj = context->getRuntime()->newObject(context, clazz, count);
                if (obj != NULL)
                {
                    Value v;
                    v.type = VALUE_OBJECT;
                    v.object = obj;
                    context->push(v);
                }
                else
                {
                    ERROR("NEW: Failed to create new object! class=%s\n", clazz->getName().c_str());
                    running = false;
                    success = false;
                }
            } break;

            case OPCODE_NEW_STRING:
            {
                const char* str = (const char*)code.code[pc++];
                LOG("NEW_STRING: %s", str);
                Object* strObj = String::createString(context, str);
                if (strObj == NULL)
                {
                    running = false;
                    success = false;
                }
                Value v;
                v.type = VALUE_OBJECT;
                v.object = strObj;
                context->push(v);
            } break;

            case OPCODE_NEW_FUNCTION:
            {
                Function* func = (Function*)code.code[pc++];

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
            } break;

            case OPCODE_RETURN:
            {
                Value result = context->pop();
                LOG("RETURN %s", result.toString().c_str());

                Value frameCheck = context->pop();
                if (frameCheck.type != VALUE_FRAME || frameCheck.pointer != &frame)
                {
                    ERROR("EXPECTING CURRENT FRAME ON STACK! GOT: %s\n", frameCheck.toString().c_str());
                    success = false;
                }
                else
                {
                    success = true;
                    context->push(result);
                }
                running = false;
            } break;

            case OPCODE_CMP:
            {
                Value left = context->pop();
                Value right = context->pop();
                if (left.type == VALUE_INTEGER && right.type == VALUE_INTEGER)
                {
                    int64_t result = (int64_t)right.i - (int64_t)left.i;
                    //int64_t uresult = (uint64_t)right.i - (uint64_t)left.i;
                    flagZero = (result == 0);
                    flagSign = (result < 0);
                    flagOverflow = !flagZero || flagSign;
                    //context->push(result);
                    LOG("CMP: left=%s, right=%s, result=%lld", left.toString().c_str(), right.toString().c_str(), result);
                }
                else if (left.type == VALUE_DOUBLE || right.type == VALUE_DOUBLE)
                {
                    double leftDouble = DOUBLE_VALUE(left);
                    double rightDouble = DOUBLE_VALUE(right);
                    double result = rightDouble - leftDouble;
                    double resultAbs = fabs(result);
                    LOG("CMP: left=%0.2f, right=%0.2f, result=%0.2f (%0.2f)", leftDouble, rightDouble, result, resultAbs);
                    flagZero = (resultAbs < FLT_EPSILON);
                    flagSign = (result < 0.0 && !flagZero);
                    flagOverflow = !flagZero || flagSign;
                }
                else
                {
                    printf("CMP: UNHANDLED DATA TYPES\n");
                    running = false;
                    success = false;
                }

                LOG("CMP:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_CMPI:
            {
                Value left = context->pop();
                Value right = context->pop();
                int64_t result = (int64_t)right.i - (int64_t)left.i;
                flagZero = (result == 0);
                flagSign = (result < 0);
                flagOverflow = !flagZero || flagSign;
                //context->push(result);
                LOG("CMPI: left=%s, right=%s, result=%lld", left.toString().c_str(), right.toString().c_str(), result);
                LOG("CMPI:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
 
            } break;

            case OPCODE_CMPD:
            {
                Value left = context->pop();
                Value right = context->pop();
                double leftDouble = left.d;
                double rightDouble = right.d;
                double result = rightDouble - leftDouble;
                double resultAbs = fabs(result);
                LOG("CMPD: left=%0.2f, right=%0.2f, result=%0.2f (%0.2f)", leftDouble, rightDouble, result, resultAbs);
                flagZero = (resultAbs < FLT_EPSILON);
                flagSign = (result < 0.0 && !flagZero);
                    flagOverflow = !flagZero || flagSign;
 
                LOG("CMPD:  -> zero=%d, sign=%d, overflow=%d", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_JMP:
            {
                uint64_t dest = code.code[pc++];
                LOG("JMP: dest=%lld", dest);
                pc = dest;
            } break;

            case OPCODE_BEQ:
            {
                uint64_t dest = code.code[pc++];
                LOG("BEQ: flagZero=%d, dest=0x%llx", flagZero, dest);
                if (flagZero)
                {
                    pc = dest;
                }
            } break;

            case OPCODE_BNE:
            {
                uint64_t dest = code.code[pc++];
                LOG("BNE: flagZero=%d, dest=0x%llx", flagZero, dest);
                if (!flagZero)
                {
                    pc = dest;
                }
            } break;

            default:
                ERROR("Unhandled opcode: 0x%llx", opcode);
                running = false;
                success = false;
        }
    }

    //context->getRuntime()->gc();

#ifdef DEBUG_EXECUTOR
    printf("Executor::run: Leaving %s\n", code.function->getFullName().c_str());
#endif

    return success;
}

