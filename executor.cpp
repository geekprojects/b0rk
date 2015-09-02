
#include "executor.h"
#include "string.h"

#define DOUBLE_VALUE(_v) (((_v).type == VALUE_DOUBLE) ? (_v.d) : (double)(_v.i))
#define INTEGER_VALUE(_v) (((_v).type == VALUE_INTEGER) ? (_v.i) : (int)(floor(_v.d)))

using namespace std;

Executor::Executor()
{
}

bool Executor::run(Context* context, AssembledCode& code)
{
    size_t pc = 0;
    bool res;

    Value localVars[code.localVars];

    bool flagZero = false;
    bool flagSign = false;
    bool flagOverflow = false;

    while (pc < code.size)
    {
        uint64_t thisPC = pc;
        uint64_t opcode = code.code[pc++];
        printf("Executor::run: %04llx: 0x%llx\n", thisPC, opcode);
        switch (opcode)
        {
            case OPCODE_LOAD:
            {
                int varId = code.code[pc++]; // TODO: Where we're going to load the value from
                context->push(localVars[varId]);
                printf("Executor::run: LOAD: v%d\n", varId);
            } break;

            case OPCODE_STORE:
            {
                int varId = code.code[pc++]; // TODO: Where we're going to store the value
                localVars[varId] = context->pop();
                printf("Executor::run: STORE: v%d = %s\n", varId, localVars[varId].toString().c_str());
            } break;

            case OPCODE_PUSHI:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = code.code[pc++];
                printf("Executor::run: PUSHI: %lld\n", v.i);
                context->push(v);
            } break;

            case OPCODE_PUSHD:
            {
                Value v;
                v.type = VALUE_DOUBLE;
                double* d = (double*)(&(code.code[pc++]));
                v.d = *d;
                printf("Executor::run: PUSHD: %0.2f\n", v.d);
                context->push(v);
            } break;

            case OPCODE_ADD:
            {
                Value v1 = context->pop();
                if (v1.type == VALUE_OBJECT)
                {
                    // TODO: This should have been figured out by the assembler?
                    Class* clazz = v1.object->getClass();
                    printf("Executor::run: ADD OBJECT: class=%s\n", clazz->getName().c_str());
                    Function* addFunc = clazz->findMethod("operator+");
                    printf("Executor::run: ADD OBJECT: add operator=%p\n", addFunc);
                    res = addFunc->execute(context, v1.object);
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
                        result.d = DOUBLE_VALUE(v1) + DOUBLE_VALUE(v2);
                        printf("Executor::run: ADD: %0.2f + %0.2f = %0.2f\n", v1.d, v2.d, result.d);
                    }
                    else
                    {
                        result.type = VALUE_INTEGER;
                        result.i = v1.i + v2.i;
                        printf("Executor::run: ADD: %lld + %lld = %lld\n", v1.i, v2.i, result.i);
                    }
                    context->push(result);
                }
            } break;

            case OPCODE_ADDI:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_INTEGER;
                result.i = v1.i + v2.i;
                printf("Executor::run: ADDI: %lld + %lld = %lld\n", v1.i, v2.i, result.i);
                context->push(result);
            } break;

            case OPCODE_ADDD:
            {
                Value v1 = context->pop();
                Value v2 = context->pop();
                Value result;
                result.type = VALUE_DOUBLE;
                result.d = DOUBLE_VALUE(v1) + DOUBLE_VALUE(v2);
                printf("Executor::run: ADDD: %0.2f + %0.2f = %0.2f\n", v1.d, v2.d, result.d);
                context->push(result);
            } break;

            case OPCODE_PUSHCL:
            {
                Value v;
                v.type = VALUE_INTEGER;
                v.i = (flagSign != flagOverflow);
                context->push(v);
                printf("Executor::run: PUSHCL: %lld\n", v.i);
                printf("Executor::run: PUSHCL:  -> zero=%d, sign=%d, overflow=%d\n", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_CALL_STATIC:
            {
                Function* function = (Function*)code.code[pc++];
                printf("Executor::run: CALL: %p\n", function);
                res = function->execute(context, NULL);
                if (!res)
                {
                    return false;
                }
            } break;

            case OPCODE_CALL_NAMED:
            {
                Object* obj = context->pop().object;
                Object* nameObj = context->pop().object;
                string funcName = String::getString(context, nameObj);
                printf("Executor::run: CALL_NAMED: obj=%p, name=%s\n", obj, funcName.c_str());

                Function* function = obj->getClass()->findMethod(funcName);
                printf("Executor::run: CALL_NAMED:  -> function=%p\n", function);
                res = function->execute(context, obj);
                if (!res)
                {
                    return false;
                }
            } break;

            case OPCODE_NEW:
            {
                Class* clazz = (Class*)code.code[pc++];
                printf("Executor::run: NEW: class=%s\n", clazz->getName().c_str());
                Object* obj = context->getRuntime()->newObject(context, clazz);
                printf("Executor::run: NEW:  -> object=%p\n", obj);
                Value v;
                v.type = VALUE_OBJECT;
                v.object = obj;
                context->push(v);
            } break;

            case OPCODE_NEW_STRING:
            {
                const char* str = (const char*)code.code[pc++];
                printf("Executor::run: NEW_STRING: %s\n", str);
                Object* strObj = String::createString(context, str);
                Value v;
                v.type = VALUE_OBJECT;
                v.object = strObj;
                context->push(v);
            } break;

            case OPCODE_CMP:
            {
                Value left = context->pop();
                Value right = context->pop();
                int64_t result = (int64_t)right.i - (int64_t)left.i;
                flagZero = (result == 0);
                flagSign = (result < 0);
                flagOverflow = (right.i > left.i);
                //context->push(result);
                printf("Executor::run: CMP: left=%s, right=%s, result=%lld\n", left.toString().c_str(), right.toString().c_str(), result);
                printf("Executor::run: CMP:  -> zero=%d, sign=%d, overflow=%d\n", flagZero, flagSign, flagOverflow);
            } break;

            case OPCODE_JMP:
            {
                uint64_t dest = code.code[pc++];
                printf("Executor::run: JMP: dest=%lld\n", dest);
                pc = dest;
                //return false;
            } break;

            case OPCODE_BEQ:
            {
                uint64_t dest = code.code[pc++];
                printf("Executor::run: BEQ: flagZero=%d, dest=0x%llx\n", flagZero, dest);
                if (flagZero)
                {
                    pc = dest;
                }
            } break;

            case OPCODE_BNE:
            {
                uint64_t dest = code.code[pc++];
                printf("Executor::run: BNE: flagZero=%d, dest=0x%llx\n", flagZero, dest);
                if (!flagZero)
                {
                    pc = dest;
                }
            } break;

            default:
                printf("Executor::run: Unhandled opcode: 0x%llx\n", opcode);
                return false;
        }
    }

    return true;
}

