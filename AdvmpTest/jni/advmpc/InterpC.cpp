#include "stdafx.h"
#include "DexOpcodes.h"
#include "InterpC.h"

//////////////////////////////////////////////////////////////////////////

static const char kSpacing[] = "            ";

//////////////////////////////////////////////////////////////////////////

/**
 * 获得参数寄存器个数。
 * @param[in] separatorData Separator数据。
 * @return 返回参数寄存器个数。
 */
static size_t getParamRegCount(const SeparatorData* separatorData) {
    int count = 0;

    for (int i = 0; i < separatorData->paramShortDesc.size; i++) {
        switch (separatorData->paramShortDesc.str[i]) {
        case 'Z':
        case 'B':
        case 'S':
        case 'C':
        case 'I':
        case 'F':
        case 'L':
        case '[':
            count++;
            break;
        case 'J':
        case 'D':
            count += 2;
            break;
        default:
            MY_LOG_ERROR("无效的短类型！");
            break;
        }
    }
    return count;
}

/**
 * 是否是静态方法。
 * @param[in] separatorData Separator数据。
 * @return true：是静态方法。false：不是静态方法。
 */
static inline bool isStaticMethod(const SeparatorData* separatorData) {
    return separatorData->accessFlag & ACC_STATIC == 0 ? false : true;
}

/**
 * 解析可变参数，获得参数数组。
 * @param[in] 
 * @param[in] 
 * @return 返回参数数组。这个数组使用完后需要释放内存。
 */
static jvalue* getParams(const SeparatorData* separatorData, va_list args) {
    jvalue* params = new jvalue[separatorData->paramSize];
    for (int i = 0; i < separatorData->paramSize; i++) {
        switch (separatorData->paramShortDesc.str[i]) {
        case 'Z':
            params[i].z = va_arg(args, jboolean);
            break;

        case 'B':
            params[i].b = va_arg(args, jbyte);
            break;

        case 'S':
            params[i].s = va_arg(args, jshort);
            break;

        case 'C':
            params[i].c = va_arg(args, jchar);
            break;

        case 'I':
            params[i].i = va_arg(args, jint);
            break;

        case 'J':
            params[i].j = va_arg(args, jlong);
            break;

        case 'F':
            params[i].f = va_arg(args, jfloat);
            break;

        case 'D':
            params[i].d = va_arg(args, jdouble);
            break;

        case 'L':
            params[i].l = va_arg(args, jobject);
            break;

        case '[':
            params[i].l = va_arg(args, jarray);
            break;
        default:
            MY_LOG_WARNING("无效的短类型。");
            break;
        }
    }
    return params;
}

//////////////////////////////////////////////////////////////////////////

/* get a long from an array of u4 */
static inline s8 getLongFromArray(const u4* ptr, int idx)
{
#if defined(NO_UNALIGN_64__UNION)
    union { s8 ll; u4 parts[2]; } conv;

    ptr += idx;
    conv.parts[0] = ptr[0];
    conv.parts[1] = ptr[1];
    return conv.ll;
#else
    s8 val;
    memcpy(&val, &ptr[idx], 8);
    return val;
#endif
}

/* store a long into an array of u4 */
static inline void putLongToArray(u4* ptr, int idx, s8 val)
{
#if defined(NO_UNALIGN_64__UNION)
    union { s8 ll; u4 parts[2]; } conv;

    ptr += idx;
    conv.ll = val;
    ptr[0] = conv.parts[0];
    ptr[1] = conv.parts[1];
#else
    memcpy(&ptr[idx], &val, 8);
#endif
}

/* get a double from an array of u4 */
static inline double getDoubleFromArray(const u4* ptr, int idx)
{
#if defined(NO_UNALIGN_64__UNION)
    union { double d; u4 parts[2]; } conv;

    ptr += idx;
    conv.parts[0] = ptr[0];
    conv.parts[1] = ptr[1];
    return conv.d;
#else
    double dval;
    memcpy(&dval, &ptr[idx], 8);
    return dval;
#endif
}

/* store a double into an array of u4 */
static inline void putDoubleToArray(u4* ptr, int idx, double dval)
{
#if defined(NO_UNALIGN_64__UNION)
    union { double d; u4 parts[2]; } conv;

    ptr += idx;
    conv.d = dval;
    ptr[0] = conv.parts[0];
    ptr[1] = conv.parts[1];
#else
    memcpy(&ptr[idx], &dval, 8);
#endif
}

/*
 * Check to see if "obj" is NULL.  If so, throw an exception.  Assumes the
 * pc has already been exported to the stack.
 *
 * Perform additional checks on debug builds.
 *
 * Use this to check for NULL when the instruction handler calls into
 * something that could throw an exception (so we have already called
 * EXPORT_PC at the top).
 */
static inline bool checkForNull(Object* obj)
{
    if (obj == NULL) {
        dvmThrowNullPointerException(NULL);
        return false;
    }
#ifdef WITH_EXTRA_OBJECT_VALIDATION
    if (!dvmIsHeapAddress(obj)) {
        ALOGE("Invalid object %p", obj);
        dvmAbort();
    }
#endif
#ifndef NDEBUG
    if (obj->clazz == NULL || ((u4) obj->clazz) <= 65536) {
        /* probable heap corruption */
        ALOGE("Invalid object class %p (in %p)", obj->clazz, obj);
        dvmAbort();
    }
#endif
    return true;
}

//////////////////////////////////////////////////////////////////////////

// TODO 这个宏现在还不支持。
/* move between the stack save area and the frame pointer */
#define SAVEAREA_FROM_FP(_fp)   ((StackSaveArea*)(_fp) -1)

/*
 * The current PC must be available to Throwable constructors, e.g.
 * those created by the various exception throw routines, so that the
 * exception stack trace can be generated correctly.  If we don't do this,
 * the offset within the current method won't be shown correctly.  See the
 * notes in Exception.c.
 *
 * This is also used to determine the address for precise GC.
 *
 * Assumes existence of "u4* fp" and "const u2* pc".
 */
#define EXPORT_PC()         (SAVEAREA_FROM_FP(fp)->xtra.currentPc = pc)

/*
 * TODO 这个宏现在还不支持。
 * Periodically check for thread suspension.
 *
 * While we're at it, see if a debugger has attached or the profiler has
 * started.  If so, switch to a different "goto" table.
 */
#define PERIODIC_CHECKS(_pcadj) {                              \
        /*if (dvmCheckSuspendQuick(self)) {*/                                   \
            EXPORT_PC();  /* need for precise GC */                         \
            /*dvmCheckSuspendPending(self);*/                                   \
        }                                                                   \
    }

//////////////////////////////////////////////////////////////////////////

/*
 * If enabled, validate the register number on every access.  Otherwise,
 * just do an array access.
 *
 * Assumes the existence of "u4* fp".
 *
 * "_idx" may be referenced more than once.
 */
#ifdef CHECK_REGISTER_INDICES
# define GET_REGISTER(_idx) \
    ( (_idx) < curMethod->registersSize ? \
        (fp[(_idx)]) : (assert(!"bad reg"),1969) )
# define SET_REGISTER(_idx, _val) \
    ( (_idx) < curMethod->registersSize ? \
        (fp[(_idx)] = (u4)(_val)) : (assert(!"bad reg"),1969) )
# define GET_REGISTER_AS_OBJECT(_idx)       ((Object *)GET_REGISTER(_idx))
# define SET_REGISTER_AS_OBJECT(_idx, _val) SET_REGISTER(_idx, (s4)_val)
# define GET_REGISTER_INT(_idx) ((s4) GET_REGISTER(_idx))
# define SET_REGISTER_INT(_idx, _val) SET_REGISTER(_idx, (s4)_val)
# define GET_REGISTER_WIDE(_idx) \
    ( (_idx) < curMethod->registersSize-1 ? \
        getLongFromArray(fp, (_idx)) : (assert(!"bad reg"),1969) )
# define SET_REGISTER_WIDE(_idx, _val) \
    ( (_idx) < curMethod->registersSize-1 ? \
        (void)putLongToArray(fp, (_idx), (_val)) : assert(!"bad reg") )
# define GET_REGISTER_FLOAT(_idx) \
    ( (_idx) < curMethod->registersSize ? \
        (*((float*) &fp[(_idx)])) : (assert(!"bad reg"),1969.0f) )
# define SET_REGISTER_FLOAT(_idx, _val) \
    ( (_idx) < curMethod->registersSize ? \
        (*((float*) &fp[(_idx)]) = (_val)) : (assert(!"bad reg"),1969.0f) )
# define GET_REGISTER_DOUBLE(_idx) \
    ( (_idx) < curMethod->registersSize-1 ? \
        getDoubleFromArray(fp, (_idx)) : (assert(!"bad reg"),1969.0) )
# define SET_REGISTER_DOUBLE(_idx, _val) \
    ( (_idx) < curMethod->registersSize-1 ? \
        (void)putDoubleToArray(fp, (_idx), (_val)) : assert(!"bad reg") )
#else
# define GET_REGISTER(_idx)                 (fp[(_idx)])
# define SET_REGISTER(_idx, _val)           (fp[(_idx)] = (_val))
# define GET_REGISTER_AS_OBJECT(_idx)       ((Object*) fp[(_idx)])
# define SET_REGISTER_AS_OBJECT(_idx, _val) (fp[(_idx)] = (u4)(_val))
# define GET_REGISTER_INT(_idx)             ((s4)GET_REGISTER(_idx))
# define SET_REGISTER_INT(_idx, _val)       SET_REGISTER(_idx, (s4)_val)
# define GET_REGISTER_WIDE(_idx)            getLongFromArray(fp, (_idx))
# define SET_REGISTER_WIDE(_idx, _val)      putLongToArray(fp, (_idx), (_val))
# define GET_REGISTER_FLOAT(_idx)           (*((float*) &fp[(_idx)]))
# define SET_REGISTER_FLOAT(_idx, _val)     (*((float*) &fp[(_idx)]) = (_val))
# define GET_REGISTER_DOUBLE(_idx)          getDoubleFromArray(fp, (_idx))
# define SET_REGISTER_DOUBLE(_idx, _val)    putDoubleToArray(fp, (_idx), (_val))
#endif

//////////////////////////////////////////////////////////////////////////

#define HANDLE_OP_X_INT_LIT8(_opcode, _opname, _op, _chkdiv)                \
    HANDLE_OPCODE(_opcode /*vAA, vBB, #+CC*/)                               \
    {                                                                       \
        u2 litInfo;                                                         \
        vdst = INST_AA(inst);                                               \
        litInfo = FETCH(1);                                                 \
        vsrc1 = litInfo & 0xff;                                             \
        vsrc2 = litInfo >> 8;       /* constant */                          \
        MY_LOG_VERBOSE("|%s-int/lit8 v%d,v%d,#+0x%02x",                              \
            (_opname), vdst, vsrc1, vsrc2);                                 \
        if (_chkdiv != 0) {                                                 \
            s4 firstVal, result;                                            \
            firstVal = GET_REGISTER(vsrc1);                                 \
            if ((s1) vsrc2 == 0) {                                          \
                EXPORT_PC();                                                \
                /*dvmThrowArithmeticException("divide by zero"); */             \
                /*GOTO_exceptionThrown();*/                                     \
                MY_LOG_ERROR("divide by zero");                             \
            }                                                               \
            if ((u4)firstVal == 0x80000000 && ((s1) vsrc2) == -1) {         \
                if (_chkdiv == 1)                                           \
                    result = firstVal;  /* division */                      \
                else                                                        \
                    result = 0;         /* remainder */                     \
            } else {                                                        \
                result = firstVal _op ((s1) vsrc2);                         \
            }                                                               \
            SET_REGISTER(vdst, result);                                     \
        } else {                                                            \
            /*MY_LOG_INFO("op1:%d,op2:%d", GET_REGISTER(vsrc1), (s1) vsrc2);*/ \
            SET_REGISTER(vdst,                                              \
                (s4) GET_REGISTER(vsrc1) _op (s1) vsrc2);                   \
        }                                                                   \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_APUT(_opcode, _opname, _type, _regsize)                   \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        ArrayObject* arrayObj;                                              \
        u2 arrayInfo;                                                       \
        EXPORT_PC();                                                        \
        vdst = INST_AA(inst);       /* AA: source value */                  \
        arrayInfo = FETCH(1);                                               \
        vsrc1 = arrayInfo & 0xff;   /* BB: array ptr */                     \
        vsrc2 = arrayInfo >> 8;     /* CC: index */                         \
        MY_LOG_VERBOSE("|aput%s v%d,v%d,v%d", (_opname), vdst, vsrc1, vsrc2);        \
        arrayObj = (ArrayObject*) GET_REGISTER(vsrc1);                      \
        if (!checkForNull((Object*) arrayObj))                              \
            GOTO_exceptionThrown();                                         \
        if (GET_REGISTER(vsrc2) >= arrayObj->length) {                      \
            dvmThrowArrayIndexOutOfBoundsException(                         \
                arrayObj->length, GET_REGISTER(vsrc2));                     \
            GOTO_exceptionThrown();                                         \
        }                                                                   \
        MY_LOG_VERBOSE("+ APUT[%d]=0x%08x", GET_REGISTER(vsrc2), GET_REGISTER(vdst));\
        ((_type*)(void*)arrayObj->contents)[GET_REGISTER(vsrc2)] =          \
            GET_REGISTER##_regsize(vdst);                                   \
    }                                                                       \
    FINISH(2);

//////////////////////////////////////////////////////////////////////////

#define GOTO_bail() goto bail;

//////////////////////////////////////////////////////////////////////////

jvalue BWdvmInterpretPortable(const SeparatorData* separatorData, JNIEnv* env, jobject thiz, ...) {
    jvalue* params = NULL; // 参数数组。
    jvalue retval;  // 返回值。

    const u2* pc;   // 程序计数器。
    u4 fp[65535];   // 寄存器数组。
    u2 inst;        // 当前指令。
    u2 vsrc1, vsrc2, vdst;      // usually used for register indexes

    unsigned int startIndex;

    // 处理参数。
    va_list args;
    va_start(args, thiz); 
    params = getParams(separatorData, args);
    va_end(args);

    // 获得参数寄存器个数。    
    size_t paramRegCount = getParamRegCount(separatorData);

    // 设置参数寄存器的值。
    if (isStaticMethod(separatorData)) {
        startIndex = separatorData->registerSize - separatorData->paramSize;
    } else {
        startIndex = separatorData->registerSize - separatorData->paramSize;
        fp[startIndex++] = (u4)thiz;
    }
    for (int i = startIndex, j = 0; j < separatorData->paramSize; j++ ) {
        if ('D' == separatorData->paramShortDesc.str[i] || 'J' == separatorData->paramShortDesc.str[i]) {
            fp[i++] = params[j].j & 0xFFFFFFFF;
            fp[i++] = (params[j].j >> 32) & 0xFFFFFFFF;
        } else {
            fp[i++] = params[j].i;
        }
    }

    pc = separatorData->insts;

    /* static computed goto table */
    DEFINE_GOTO_TABLE(handlerTable);

    // 抓取第一条指令。
    FINISH(0);

HANDLE_OPCODE(OP_NOP)
    FINISH(1);
OP_END

HANDLE_OPCODE(OP_MOVE)
    vdst = INST_A(inst);
    vsrc1 = INST_B(inst);
    MY_LOG_VERBOSE("|move%s v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(1);
OP_END

HANDLE_OPCODE(OP_MOVE_FROM16)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|move%s/from16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_FROM16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(2);
OP_END

HANDLE_OPCODE(OP_MOVE_16)
    vdst = FETCH(1);
    vsrc1 = FETCH(2);
    MY_LOG_VERBOSE("|move%s/16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(3);
OP_END

HANDLE_OPCODE(OP_MOVE_WIDE)
    /* IMPORTANT: must correctly handle overlapping registers, e.g. both
     * "move-wide v6, v7" and "move-wide v7, v6" */
    vdst = INST_A(inst);
    vsrc1 = INST_B(inst);
    MY_LOG_VERBOSE("|move-wide v%d,v%d %s(v%d=0x%08llx)", vdst, vsrc1,
        kSpacing+5, vdst, GET_REGISTER_WIDE(vsrc1));
    SET_REGISTER_WIDE(vdst, GET_REGISTER_WIDE(vsrc1));
    FINISH(1);
OP_END

HANDLE_OPCODE(OP_MOVE_WIDE_FROM16)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|move-wide/from16 v%d,v%d  (v%d=0x%08llx)", vdst, vsrc1,
        vdst, GET_REGISTER_WIDE(vsrc1));
    SET_REGISTER_WIDE(vdst, GET_REGISTER_WIDE(vsrc1));
    FINISH(2);
OP_END

HANDLE_OPCODE(OP_MOVE_WIDE_16)
    vdst = FETCH(1);
    vsrc1 = FETCH(2);
    MY_LOG_VERBOSE("|move-wide/16 v%d,v%d %s(v%d=0x%08llx)", vdst, vsrc1,
        kSpacing+8, vdst, GET_REGISTER_WIDE(vsrc1));
    SET_REGISTER_WIDE(vdst, GET_REGISTER_WIDE(vsrc1));
    FINISH(3);
OP_END

HANDLE_OPCODE(OP_MOVE_OBJECT)
    vdst = INST_A(inst);
    vsrc1 = INST_B(inst);
    MY_LOG_VERBOSE("|move%s v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(1);
OP_END

HANDLE_OPCODE(OP_MOVE_OBJECT_FROM16)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|move%s/from16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_FROM16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(2);
OP_END
HANDLE_OPCODE(OP_MOVE_OBJECT_16)
    vdst = FETCH(1);
    vsrc1 = FETCH(2);
    MY_LOG_VERBOSE("|move%s/16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(3);
OP_END

HANDLE_OPCODE(OP_MOVE_RESULT)
    vdst = INST_AA(inst);
    MY_LOG_VERBOSE("|move-result%s v%d %s(v%d=0x%08x)",
         (INST_INST(inst) == OP_MOVE_RESULT) ? "" : "-object",
         vdst, kSpacing+4, vdst,retval.i);
    SET_REGISTER(vdst, retval.i);
    FINISH(1);
OP_END

/* File: c/OP_MOVE_RESULT_WIDE.cpp */
HANDLE_OPCODE(OP_MOVE_RESULT_WIDE /*vAA*/)
    vdst = INST_AA(inst);
    MY_LOG_VERBOSE("|move-result-wide v%d %s(0x%08llx)", vdst, kSpacing, retval.j);
    SET_REGISTER_WIDE(vdst, retval.j);
    FINISH(1);
OP_END

/* File: c/OP_MOVE_RESULT_OBJECT.cpp */
/* File: c/OP_MOVE_RESULT.cpp */
HANDLE_OPCODE(OP_MOVE_RESULT_OBJECT /*vAA*/)
    vdst = INST_AA(inst);
    MY_LOG_VERBOSE("|move-result%s v%d %s(v%d=0x%08x)",
         (INST_INST(inst) == OP_MOVE_RESULT) ? "" : "-object",
         vdst, kSpacing+4, vdst,retval.i);
    SET_REGISTER(vdst, retval.i);
    FINISH(1);
OP_END


/* File: c/OP_MOVE_EXCEPTION.cpp */
HANDLE_OPCODE(OP_MOVE_EXCEPTION /*vAA*/)
    vdst = INST_AA(inst);
    MY_LOG_VERBOSE("|move-exception v%d", vdst);
    /* TODO 这里还不支持。 */
    /*assert(self->exception != NULL);*/
    /*SET_REGISTER(vdst, (u4)self->exception);*/
    /* TODO 这里异常还是不支持的。 */
    /*dvmClearException(self);*/
    FINISH(1);
OP_END

HANDLE_OPCODE(OP_RETURN_VOID)
    GOTO_bail();
OP_END

HANDLE_OPCODE(OP_RETURN)
    vsrc1 = INST_AA(inst);
//     MY_LOG_VERBOSE("|return%s v%d",
//           (INST_INST(inst) == OP_RETURN) ? "" : "-object", vsrc1);
    retval.i = GET_REGISTER(vsrc1);
    GOTO_bail();
OP_END

HANDLE_OPCODE(OP_RETURN_WIDE)
    vsrc1 = INST_AA(inst);
//    MY_LOG_VERBOSE("|return-wide v%d", vsrc1);
    retval.j = GET_REGISTER_WIDE(vsrc1);
    GOTO_bail();
OP_END

HANDLE_OPCODE(OP_RETURN_OBJECT)
    GOTO_bail();
OP_END

HANDLE_OPCODE(OP_CONST_4 /*vA, #+B*/)
    {
        s4 tmp;

        vdst = INST_A(inst);
        tmp = (s4) (INST_B(inst) << 28) >> 28;  // sign extend 4-bit value
        MY_LOG_VERBOSE("|const/4 v%d,#0x%02x", vdst, (s4)tmp);
        SET_REGISTER(vdst, tmp);
    }
    FINISH(1);
OP_END

/* File: c/OP_CONST_16.cpp */
HANDLE_OPCODE(OP_CONST_16 /*vAA, #+BBBB*/)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|const/16 v%d,#0x%04x", vdst, (s2)vsrc1);
    SET_REGISTER(vdst, (s2) vsrc1);
    FINISH(2);
OP_END

/* File: c/OP_CONST.cpp */
HANDLE_OPCODE(OP_CONST /*vAA, #+BBBBBBBB*/)
    {
        u4 tmp;

        vdst = INST_AA(inst);
        tmp = FETCH(1);
        tmp |= (u4)FETCH(2) << 16;
        MY_LOG_VERBOSE("|const v%d,#0x%08x", vdst, tmp);
        SET_REGISTER(vdst, tmp);
    }
    FINISH(3);
OP_END

/* File: c/OP_CONST_HIGH16.cpp */
HANDLE_OPCODE(OP_CONST_HIGH16 /*vAA, #+BBBB0000*/)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|const/high16 v%d,#0x%04x0000", vdst, vsrc1);
    SET_REGISTER(vdst, vsrc1 << 16);
    FINISH(2);
OP_END

/* File: c/OP_CONST_WIDE_16.cpp */
HANDLE_OPCODE(OP_CONST_WIDE_16 /*vAA, #+BBBB*/)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|const-wide/16 v%d,#0x%04x", vdst, (s2)vsrc1);
    SET_REGISTER_WIDE(vdst, (s2)vsrc1);
    FINISH(2);
OP_END

/* File: c/OP_CONST_WIDE_32.cpp */
HANDLE_OPCODE(OP_CONST_WIDE_32 /*vAA, #+BBBBBBBB*/)
    {
        u4 tmp;

        vdst = INST_AA(inst);
        tmp = FETCH(1);
        tmp |= (u4)FETCH(2) << 16;
        MY_LOG_VERBOSE("|const-wide/32 v%d,#0x%08x", vdst, tmp);
        SET_REGISTER_WIDE(vdst, (s4) tmp);
    }
    FINISH(3);
OP_END

/* File: c/OP_CONST_WIDE.cpp */
HANDLE_OPCODE(OP_CONST_WIDE /*vAA, #+BBBBBBBBBBBBBBBB*/)
    {
        u8 tmp;

        vdst = INST_AA(inst);
        tmp = FETCH(1);
        tmp |= (u8)FETCH(2) << 16;
        tmp |= (u8)FETCH(3) << 32;
        tmp |= (u8)FETCH(4) << 48;
        MY_LOG_VERBOSE("|const-wide v%d,#0x%08llx", vdst, tmp);
        SET_REGISTER_WIDE(vdst, tmp);
    }
    FINISH(5);
OP_END

/* File: c/OP_CONST_WIDE_HIGH16.cpp */
HANDLE_OPCODE(OP_CONST_WIDE_HIGH16 /*vAA, #+BBBB000000000000*/)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|const-wide/high16 v%d,#0x%04x000000000000", vdst, vsrc1);
    SET_REGISTER_WIDE(vdst, ((u8) vsrc1) << 48);
    FINISH(2);
OP_END

HANDLE_OPCODE(OP_CONST_STRING)
HANDLE_OPCODE(OP_CONST_STRING_JUMBO)
HANDLE_OPCODE(OP_CONST_CLASS)
HANDLE_OPCODE(OP_MONITOR_ENTER)
HANDLE_OPCODE(OP_MONITOR_EXIT)
HANDLE_OPCODE(OP_CHECK_CAST)
HANDLE_OPCODE(OP_INSTANCE_OF)
HANDLE_OPCODE(OP_ARRAY_LENGTH)
HANDLE_OPCODE(OP_NEW_INSTANCE)
HANDLE_OPCODE(OP_NEW_ARRAY)
HANDLE_OPCODE(OP_FILLED_NEW_ARRAY)
HANDLE_OPCODE(OP_FILLED_NEW_ARRAY_RANGE)
HANDLE_OPCODE(OP_FILL_ARRAY_DATA)
HANDLE_OPCODE(OP_THROW)

/* File: c/OP_GOTO.cpp */
HANDLE_OPCODE(OP_GOTO /*+AA*/)
    vdst = INST_AA(inst);
    if ((s1)vdst < 0)
        MY_LOG_VERBOSE("|goto -0x%02x", -((s1)vdst));
    else
        MY_LOG_VERBOSE("|goto +0x%02x", ((s1)vdst));
    MY_LOG_VERBOSE("> branch taken");
    if ((s1)vdst < 0)
        PERIODIC_CHECKS((s1)vdst);
    FINISH((s1)vdst);
OP_END

/* File: c/OP_GOTO_16.cpp */
HANDLE_OPCODE(OP_GOTO_16 /*+AAAA*/)
    {
        s4 offset = (s2) FETCH(1);          /* sign-extend next code unit */

        if (offset < 0)
            MY_LOG_VERBOSE("|goto/16 -0x%04x", -offset);
        else
            MY_LOG_VERBOSE("|goto/16 +0x%04x", offset);
        MY_LOG_VERBOSE("> branch taken");
        if (offset < 0)
            PERIODIC_CHECKS(offset);
        FINISH(offset);
    }
OP_END

/* File: c/OP_GOTO_32.cpp */
HANDLE_OPCODE(OP_GOTO_32 /*+AAAAAAAA*/)
    {
        s4 offset = FETCH(1);               /* low-order 16 bits */
        offset |= ((s4) FETCH(2)) << 16;    /* high-order 16 bits */

        if (offset < 0)
            MY_LOG_VERBOSE("|goto/32 -0x%08x", -offset);
        else
            MY_LOG_VERBOSE("|goto/32 +0x%08x", offset);
        MY_LOG_VERBOSE("> branch taken");
        if (offset <= 0)    /* allowed to branch to self */
            PERIODIC_CHECKS(offset);
        FINISH(offset);
    }
OP_END

HANDLE_OPCODE(OP_PACKED_SWITCH)
HANDLE_OPCODE(OP_SPARSE_SWITCH)

/* File: c/OP_CMPL_FLOAT.cpp */
HANDLE_OP_CMPX(OP_CMPL_FLOAT, "l-float", float, _FLOAT, -1)
OP_END

/* File: c/OP_CMPG_FLOAT.cpp */
HANDLE_OP_CMPX(OP_CMPG_FLOAT, "g-float", float, _FLOAT, 1)
OP_END

/* File: c/OP_CMPL_DOUBLE.cpp */
HANDLE_OP_CMPX(OP_CMPL_DOUBLE, "l-double", double, _DOUBLE, -1)
OP_END

/* File: c/OP_CMPG_DOUBLE.cpp */
HANDLE_OP_CMPX(OP_CMPG_DOUBLE, "g-double", double, _DOUBLE, 1)
OP_END

/* File: c/OP_CMP_LONG.cpp */
HANDLE_OP_CMPX(OP_CMP_LONG, "-long", s8, _WIDE, 0)
OP_END

/* File: c/OP_IF_EQ.cpp */
HANDLE_OP_IF_XX(OP_IF_EQ, "eq", ==)
OP_END

/* File: c/OP_IF_NE.cpp */
HANDLE_OP_IF_XX(OP_IF_NE, "ne", !=)
OP_END

/* File: c/OP_IF_LT.cpp */
HANDLE_OP_IF_XX(OP_IF_LT, "lt", <)
OP_END

/* File: c/OP_IF_GE.cpp */
HANDLE_OP_IF_XX(OP_IF_GE, "ge", >=)
OP_END

/* File: c/OP_IF_GT.cpp */
HANDLE_OP_IF_XX(OP_IF_GT, "gt", >)
OP_END

/* File: c/OP_IF_LE.cpp */
HANDLE_OP_IF_XX(OP_IF_LE, "le", <=)
OP_END

/* File: c/OP_IF_EQZ.cpp */
HANDLE_OP_IF_XXZ(OP_IF_EQZ, "eqz", ==)
OP_END

/* File: c/OP_IF_NEZ.cpp */
HANDLE_OP_IF_XXZ(OP_IF_NEZ, "nez", !=)
OP_END

/* File: c/OP_IF_LTZ.cpp */
HANDLE_OP_IF_XXZ(OP_IF_LTZ, "ltz", <)
OP_END

/* File: c/OP_IF_GEZ.cpp */
HANDLE_OP_IF_XXZ(OP_IF_GEZ, "gez", >=)
OP_END

/* File: c/OP_IF_GTZ.cpp */
HANDLE_OP_IF_XXZ(OP_IF_GTZ, "gtz", >)
OP_END

/* File: c/OP_IF_LEZ.cpp */
HANDLE_OP_IF_XXZ(OP_IF_LEZ, "lez", <=)
OP_END

/* File: c/OP_UNUSED_3E.cpp */
HANDLE_OPCODE(OP_UNUSED_3E)
OP_END

/* File: c/OP_UNUSED_3F.cpp */
HANDLE_OPCODE(OP_UNUSED_3F)
OP_END

/* File: c/OP_UNUSED_40.cpp */
HANDLE_OPCODE(OP_UNUSED_40)
OP_END

/* File: c/OP_UNUSED_41.cpp */
HANDLE_OPCODE(OP_UNUSED_41)
OP_END

/* File: c/OP_UNUSED_42.cpp */
HANDLE_OPCODE(OP_UNUSED_42)
OP_END

/* File: c/OP_UNUSED_43.cpp */
HANDLE_OPCODE(OP_UNUSED_43)
OP_END

/* File: c/OP_AGET.cpp */
HANDLE_OP_AGET(OP_AGET, "", u4, )
OP_END

/* File: c/OP_AGET_WIDE.cpp */
HANDLE_OP_AGET(OP_AGET_WIDE, "-wide", s8, _WIDE)
OP_END

/* File: c/OP_AGET_OBJECT.cpp */
HANDLE_OP_AGET(OP_AGET_OBJECT, "-object", u4, )
OP_END

/* File: c/OP_AGET_BOOLEAN.cpp */
HANDLE_OP_AGET(OP_AGET_BOOLEAN, "-boolean", u1, )
OP_END

/* File: c/OP_AGET_BYTE.cpp */
HANDLE_OP_AGET(OP_AGET_BYTE, "-byte", s1, )
OP_END

/* File: c/OP_AGET_CHAR.cpp */
HANDLE_OP_AGET(OP_AGET_CHAR, "-char", u2, )
OP_END

/* File: c/OP_AGET_SHORT.cpp */
HANDLE_OP_AGET(OP_AGET_SHORT, "-short", s2, )
OP_END

/* File: c/OP_APUT.cpp */
HANDLE_OP_APUT(OP_APUT, "", u4, )
OP_END

/* File: c/OP_APUT_WIDE.cpp */
HANDLE_OP_APUT(OP_APUT_WIDE, "-wide", s8, _WIDE)
OP_END

HANDLE_OPCODE(OP_APUT_OBJECT)
HANDLE_OPCODE(OP_APUT_BOOLEAN)
HANDLE_OPCODE(OP_APUT_BYTE)
HANDLE_OPCODE(OP_APUT_CHAR)
HANDLE_OPCODE(OP_APUT_SHORT)
HANDLE_OPCODE(OP_IGET)
HANDLE_OPCODE(OP_IGET_WIDE)
HANDLE_OPCODE(OP_IGET_OBJECT)
HANDLE_OPCODE(OP_IGET_BOOLEAN)
HANDLE_OPCODE(OP_IGET_BYTE)
HANDLE_OPCODE(OP_IGET_CHAR)
HANDLE_OPCODE(OP_IGET_SHORT)
HANDLE_OPCODE(OP_IPUT)
HANDLE_OPCODE(OP_IPUT_WIDE)
HANDLE_OPCODE(OP_IPUT_OBJECT)
HANDLE_OPCODE(OP_IPUT_BOOLEAN)
HANDLE_OPCODE(OP_IPUT_BYTE)
HANDLE_OPCODE(OP_IPUT_CHAR)
HANDLE_OPCODE(OP_IPUT_SHORT)
HANDLE_OPCODE(OP_SGET)
HANDLE_OPCODE(OP_SGET_WIDE)
HANDLE_OPCODE(OP_SGET_OBJECT)
HANDLE_OPCODE(OP_SGET_BOOLEAN)
HANDLE_OPCODE(OP_SGET_BYTE)
HANDLE_OPCODE(OP_SGET_CHAR)
HANDLE_OPCODE(OP_SGET_SHORT)
HANDLE_OPCODE(OP_SPUT)
HANDLE_OPCODE(OP_SPUT_WIDE)
HANDLE_OPCODE(OP_SPUT_OBJECT)
HANDLE_OPCODE(OP_SPUT_BOOLEAN)
HANDLE_OPCODE(OP_SPUT_BYTE)
HANDLE_OPCODE(OP_SPUT_CHAR)
HANDLE_OPCODE(OP_SPUT_SHORT)
HANDLE_OPCODE(OP_INVOKE_VIRTUAL)
HANDLE_OPCODE(OP_INVOKE_SUPER)
HANDLE_OPCODE(OP_INVOKE_DIRECT)
HANDLE_OPCODE(OP_INVOKE_STATIC)
HANDLE_OPCODE(OP_INVOKE_INTERFACE)
HANDLE_OPCODE(OP_UNUSED_73)
HANDLE_OPCODE(OP_INVOKE_VIRTUAL_RANGE)
HANDLE_OPCODE(OP_INVOKE_SUPER_RANGE)
HANDLE_OPCODE(OP_INVOKE_DIRECT_RANGE)
HANDLE_OPCODE(OP_INVOKE_STATIC_RANGE)
HANDLE_OPCODE(OP_INVOKE_INTERFACE_RANGE)
HANDLE_OPCODE(OP_UNUSED_79)
HANDLE_OPCODE(OP_UNUSED_7A)
HANDLE_OPCODE(OP_NEG_INT)
HANDLE_OPCODE(OP_NOT_INT)
HANDLE_OPCODE(OP_NEG_LONG)
HANDLE_OPCODE(OP_NOT_LONG)
HANDLE_OPCODE(OP_NEG_FLOAT)
HANDLE_OPCODE(OP_NEG_DOUBLE)
HANDLE_OPCODE(OP_INT_TO_LONG)
HANDLE_OPCODE(OP_INT_TO_FLOAT)
HANDLE_OPCODE(OP_INT_TO_DOUBLE)
HANDLE_OPCODE(OP_LONG_TO_INT)
HANDLE_OPCODE(OP_LONG_TO_FLOAT)
HANDLE_OPCODE(OP_LONG_TO_DOUBLE)
HANDLE_OPCODE(OP_FLOAT_TO_INT)
HANDLE_OPCODE(OP_FLOAT_TO_LONG)
HANDLE_OPCODE(OP_FLOAT_TO_DOUBLE)
HANDLE_OPCODE(OP_DOUBLE_TO_INT)
HANDLE_OPCODE(OP_DOUBLE_TO_LONG)
HANDLE_OPCODE(OP_DOUBLE_TO_FLOAT)
HANDLE_OPCODE(OP_INT_TO_BYTE)
HANDLE_OPCODE(OP_INT_TO_CHAR)
HANDLE_OPCODE(OP_INT_TO_SHORT)
HANDLE_OPCODE(OP_ADD_INT)
HANDLE_OPCODE(OP_SUB_INT)
HANDLE_OPCODE(OP_MUL_INT)
HANDLE_OPCODE(OP_DIV_INT)
HANDLE_OPCODE(OP_REM_INT)
HANDLE_OPCODE(OP_AND_INT)
HANDLE_OPCODE(OP_OR_INT)
HANDLE_OPCODE(OP_XOR_INT)
HANDLE_OPCODE(OP_SHL_INT)
HANDLE_OPCODE(OP_SHR_INT)
HANDLE_OPCODE(OP_USHR_INT)
HANDLE_OPCODE(OP_ADD_LONG)
HANDLE_OPCODE(OP_SUB_LONG)
HANDLE_OPCODE(OP_MUL_LONG)
HANDLE_OPCODE(OP_DIV_LONG)
HANDLE_OPCODE(OP_REM_LONG)
HANDLE_OPCODE(OP_AND_LONG)
HANDLE_OPCODE(OP_OR_LONG)
HANDLE_OPCODE(OP_XOR_LONG)
HANDLE_OPCODE(OP_SHL_LONG)
HANDLE_OPCODE(OP_SHR_LONG)
HANDLE_OPCODE(OP_USHR_LONG)
HANDLE_OPCODE(OP_ADD_FLOAT)
HANDLE_OPCODE(OP_SUB_FLOAT)
HANDLE_OPCODE(OP_MUL_FLOAT)
HANDLE_OPCODE(OP_DIV_FLOAT)
HANDLE_OPCODE(OP_REM_FLOAT)
HANDLE_OPCODE(OP_ADD_DOUBLE)
HANDLE_OPCODE(OP_SUB_DOUBLE)
HANDLE_OPCODE(OP_MUL_DOUBLE)
HANDLE_OPCODE(OP_DIV_DOUBLE)
HANDLE_OPCODE(OP_REM_DOUBLE)
HANDLE_OPCODE(OP_ADD_INT_2ADDR)
HANDLE_OPCODE(OP_SUB_INT_2ADDR)
HANDLE_OPCODE(OP_MUL_INT_2ADDR)
HANDLE_OPCODE(OP_DIV_INT_2ADDR)
HANDLE_OPCODE(OP_REM_INT_2ADDR)
HANDLE_OPCODE(OP_AND_INT_2ADDR)
HANDLE_OPCODE(OP_OR_INT_2ADDR)
HANDLE_OPCODE(OP_XOR_INT_2ADDR)
HANDLE_OPCODE(OP_SHL_INT_2ADDR)
HANDLE_OPCODE(OP_SHR_INT_2ADDR)
HANDLE_OPCODE(OP_USHR_INT_2ADDR)
HANDLE_OPCODE(OP_ADD_LONG_2ADDR)
HANDLE_OPCODE(OP_SUB_LONG_2ADDR)
HANDLE_OPCODE(OP_MUL_LONG_2ADDR)
HANDLE_OPCODE(OP_DIV_LONG_2ADDR)
HANDLE_OPCODE(OP_REM_LONG_2ADDR)
HANDLE_OPCODE(OP_AND_LONG_2ADDR)
HANDLE_OPCODE(OP_OR_LONG_2ADDR)
HANDLE_OPCODE(OP_XOR_LONG_2ADDR)
HANDLE_OPCODE(OP_SHL_LONG_2ADDR)
HANDLE_OPCODE(OP_SHR_LONG_2ADDR)
HANDLE_OPCODE(OP_USHR_LONG_2ADDR)
HANDLE_OPCODE(OP_ADD_FLOAT_2ADDR)
HANDLE_OPCODE(OP_SUB_FLOAT_2ADDR)
HANDLE_OPCODE(OP_MUL_FLOAT_2ADDR)
HANDLE_OPCODE(OP_DIV_FLOAT_2ADDR)
HANDLE_OPCODE(OP_REM_FLOAT_2ADDR)
HANDLE_OPCODE(OP_ADD_DOUBLE_2ADDR)
HANDLE_OPCODE(OP_SUB_DOUBLE_2ADDR)
HANDLE_OPCODE(OP_MUL_DOUBLE_2ADDR)
HANDLE_OPCODE(OP_DIV_DOUBLE_2ADDR)
HANDLE_OPCODE(OP_REM_DOUBLE_2ADDR)
HANDLE_OPCODE(OP_ADD_INT_LIT16)
HANDLE_OPCODE(OP_RSUB_INT)
HANDLE_OPCODE(OP_MUL_INT_LIT16)
HANDLE_OPCODE(OP_DIV_INT_LIT16)
HANDLE_OPCODE(OP_REM_INT_LIT16)
HANDLE_OPCODE(OP_AND_INT_LIT16)
HANDLE_OPCODE(OP_OR_INT_LIT16)
HANDLE_OPCODE(OP_XOR_INT_LIT16)

HANDLE_OP_X_INT_LIT8(OP_ADD_INT_LIT8,   "add", +, 0)
OP_END

HANDLE_OPCODE(OP_RSUB_INT_LIT8)
HANDLE_OPCODE(OP_MUL_INT_LIT8)
HANDLE_OPCODE(OP_DIV_INT_LIT8)
HANDLE_OPCODE(OP_REM_INT_LIT8)
HANDLE_OPCODE(OP_AND_INT_LIT8)
HANDLE_OPCODE(OP_OR_INT_LIT8)
HANDLE_OPCODE(OP_XOR_INT_LIT8)
HANDLE_OPCODE(OP_SHL_INT_LIT8)
HANDLE_OPCODE(OP_SHR_INT_LIT8)
HANDLE_OPCODE(OP_USHR_INT_LIT8)
HANDLE_OPCODE(OP_IGET_VOLATILE)
HANDLE_OPCODE(OP_IPUT_VOLATILE)
HANDLE_OPCODE(OP_SGET_VOLATILE)
HANDLE_OPCODE(OP_SPUT_VOLATILE)
HANDLE_OPCODE(OP_IGET_OBJECT_VOLATILE)
HANDLE_OPCODE(OP_IGET_WIDE_VOLATILE)
HANDLE_OPCODE(OP_IPUT_WIDE_VOLATILE)
HANDLE_OPCODE(OP_SGET_WIDE_VOLATILE)
HANDLE_OPCODE(OP_SPUT_WIDE_VOLATILE)
HANDLE_OPCODE(OP_BREAKPOINT)
HANDLE_OPCODE(OP_THROW_VERIFICATION_ERROR)
HANDLE_OPCODE(OP_EXECUTE_INLINE)
HANDLE_OPCODE(OP_EXECUTE_INLINE_RANGE)
HANDLE_OPCODE(OP_INVOKE_OBJECT_INIT_RANGE)
HANDLE_OPCODE(OP_RETURN_VOID_BARRIER)
HANDLE_OPCODE(OP_IGET_QUICK)
HANDLE_OPCODE(OP_IGET_WIDE_QUICK)
HANDLE_OPCODE(OP_IGET_OBJECT_QUICK)
HANDLE_OPCODE(OP_IPUT_QUICK)
HANDLE_OPCODE(OP_IPUT_WIDE_QUICK)
HANDLE_OPCODE(OP_IPUT_OBJECT_QUICK)
HANDLE_OPCODE(OP_INVOKE_VIRTUAL_QUICK)
HANDLE_OPCODE(OP_INVOKE_VIRTUAL_QUICK_RANGE)
HANDLE_OPCODE(OP_INVOKE_SUPER_QUICK)
HANDLE_OPCODE(OP_INVOKE_SUPER_QUICK_RANGE)
HANDLE_OPCODE(OP_IPUT_OBJECT_VOLATILE)
HANDLE_OPCODE(OP_SGET_OBJECT_VOLATILE)
HANDLE_OPCODE(OP_SPUT_OBJECT_VOLATILE)
HANDLE_OPCODE(OP_UNUSED_FF)
OP_END

bail:
    if (NULL != params) {
        delete[] params;
    }
    MY_LOG_INFO("|-- Leaving interpreter loop");
    return retval;
}
