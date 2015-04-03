#include "stdafx.h"
#include "DexOpcodes.h"
#include "Exception.h"
#include "InterpC.h"

//////////////////////////////////////////////////////////////////////////

inline void dvmAbort(void) {
    exit(1);
}

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

//////////////////////////////////////////////////////////////////////////

//#define LOG_INSTR                   /* verbose debugging */
/* set and adjust ANDROID_LOG_TAGS='*:i jdwp:i dalvikvm:i dalvikvmi:i' */

/*
 * Export another copy of the PC on every instruction; this is largely
 * redundant with EXPORT_PC and the debugger code.  This value can be
 * compared against what we have stored on the stack with EXPORT_PC to
 * help ensure that we aren't missing any export calls.
 */
#if WITH_EXTRA_GC_CHECKS > 1
# define EXPORT_EXTRA_PC() (self->currentPc2 = pc)
#else
# define EXPORT_EXTRA_PC()
#endif

/*
 * Adjust the program counter.  "_offset" is a signed int, in 16-bit units.
 *
 * Assumes the existence of "const u2* pc" and "const u2* curMethod->insns".
 *
 * We don't advance the program counter until we finish an instruction or
 * branch, because we do want to have to unroll the PC if there's an
 * exception.
 */
#ifdef CHECK_BRANCH_OFFSETS
# define ADJUST_PC(_offset) do {                                            \
        int myoff = _offset;        /* deref only once */                   \
        if (pc + myoff < curMethod->insns ||                                \
            pc + myoff >= curMethod->insns + dvmGetMethodInsnsSize(curMethod)) \
        {                                                                   \
            char* desc;                                                     \
            desc = dexProtoCopyMethodDescriptor(&curMethod->prototype);     \
            MY_LOG_ERROR("Invalid branch %d at 0x%04x in %s.%s %s",                 \
                myoff, (int) (pc - curMethod->insns),                       \
                curMethod->clazz->descriptor, curMethod->name, desc);       \
            free(desc);                                                     \
            dvmAbort();                                                     \
        }                                                                   \
        pc += myoff;                                                        \
        EXPORT_EXTRA_PC();                                                  \
    } while (false)
#else
# define ADJUST_PC(_offset) do {                                            \
        pc += _offset;                                                      \
        EXPORT_EXTRA_PC();                                                  \
    } while (false)
#endif

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

/*
 * Get 16 bits from the specified offset of the program counter.  We always
 * want to load 16 bits at a time from the instruction stream -- it's more
 * efficient than 8 and won't have the alignment problems that 32 might.
 *
 * Assumes existence of "const u2* pc".
 */
#define FETCH(_offset)     (pc[(_offset)])

/*
 * Extract instruction byte from 16-bit fetch (_inst is a u2).
 */
#define INST_INST(_inst)    ((_inst) & 0xff)

/*
 * Replace the opcode (used when handling breakpoints).  _opcode is a u1.
 */
#define INST_REPLACE_OP(_inst, _opcode) (((_inst) & 0xff00) | _opcode)

/*
 * Extract the "vA, vB" 4-bit registers from the instruction word (_inst is u2).
 */
#define INST_A(_inst)       (((_inst) >> 8) & 0x0f)
#define INST_B(_inst)       ((_inst) >> 12)

/*
 * Get the 8-bit "vAA" 8-bit register index from the instruction word.
 * (_inst is u2)
 */
#define INST_AA(_inst)      ((_inst) >> 8)

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
// TODO 这里这里不支持。
#define EXPORT_PC()         /*(SAVEAREA_FROM_FP(fp)->xtra.currentPc = pc)*/

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
static inline bool checkForNull(JNIEnv* env, Object* obj)
{
    if (obj == NULL) {
        dvmThrowNullPointerException(env, NULL);
        return false;
    }
#ifdef WITH_EXTRA_OBJECT_VALIDATION
    if (!dvmIsHeapAddress(obj)) {
        MY_LOG_ERROR("Invalid object %p", obj);
        dvmAbort();
    }
#endif
#ifndef NDEBUG
    if (obj->clazz == NULL || ((u4) obj->clazz) <= 65536) {
        /* probable heap corruption */
        MY_LOG_ERROR("Invalid object class %p (in %p)", obj->clazz, obj);
        dvmAbort();
    }
#endif
    return true;
}

/*
 * Check to see if "obj" is NULL.  If so, export the PC into the stack
 * frame and throw an exception.
 *
 * Perform additional checks on debug builds.
 *
 * Use this to check for NULL when the instruction handler doesn't do
 * anything else that can throw an exception.
 */
static inline bool checkForNullExportPC(JNIEnv* env, Object* obj, u4* fp, const u2* pc)
{
    if (obj == NULL) {
        EXPORT_PC();
        dvmThrowNullPointerException(env, NULL);
        return false;
    }
#ifdef WITH_EXTRA_OBJECT_VALIDATION
    if (!dvmIsHeapAddress(obj)) {
        MY_LOG_ERROR("Invalid object %p", obj);
        dvmAbort();
    }
#endif
#ifndef NDEBUG
    if (obj->clazz == NULL || ((u4) obj->clazz) <= 65536) {
        /* probable heap corruption */
        MY_LOG_ERROR("Invalid object class %p (in %p)", obj->clazz, obj);
        dvmAbort();
    }
#endif
    return true;
}

/* File: portable/stubdefs.cpp */
/*
 * In the C mterp stubs, "goto" is a function call followed immediately
 * by a return.
 */

#define GOTO_TARGET_DECL(_target, ...)

#define GOTO_TARGET(_target, ...) _target:

#define GOTO_TARGET_END

/* ugh */
#define STUB_HACK(x)
#define JIT_STUB_HACK(x)

/*
 * InterpSave's pc and fp must be valid when breaking out to a
 * "Reportxxx" routine.  Because the portable interpreter uses local
 * variables for these, we must flush prior.  Stubs, however, use
 * the interpSave vars directly, so this is a nop for stubs.
 */
#define PC_FP_TO_SELF()                                                    \
    self->interpSave.pc = pc;                                              \
    self->interpSave.curFrame = fp;
#define PC_TO_SELF() self->interpSave.pc = pc;

/*
 * Instruction framing.  For a switch-oriented implementation this is
 * case/break, for a threaded implementation it's a goto label and an
 * instruction fetch/computed goto.
 *
 * Assumes the existence of "const u2* pc" and (for threaded operation)
 * "u2 inst".
 */
# define H(_op)             &&op_##_op
# define HANDLE_OPCODE(_op) op_##_op:
# define FINISH(_offset) {                                                  \
        ADJUST_PC(_offset);                                                 \
        inst = FETCH(0);                                                    \
        /*if (self->interpBreak.ctl.subMode) {*/                                \
            /*dvmCheckBefore(pc, fp, self);*/                                   \
        /*}*/                                                                   \
        goto *handlerTable[INST_INST(inst)];                                \
    }
# define FINISH_BKPT(_opcode) {                                             \
        goto *handlerTable[_opcode];                                        \
    }

#define OP_END

/*
 * The "goto" targets just turn into goto statements.  The "arguments" are
 * passed through local variables.
 */

#define GOTO_exceptionThrown() goto exceptionThrown;

//#define GOTO_returnFromMethod() goto returnFromMethod;

#define GOTO_invoke(_target, _methodCallRange)                              \
    do {                                                                    \
        methodCallRange = _methodCallRange;                                 \
        goto _target;                                                       \
    } while(false)

/* for this, the "args" are already in the locals */
#define GOTO_invokeMethod(_methodCallRange, _methodToCall, _vsrc1, _vdst) goto invokeMethod;

#define GOTO_bail() goto bail;

/*
 * Periodically check for thread suspension.
 *
 * While we're at it, see if a debugger has attached or the profiler has
 * started.  If so, switch to a different "goto" table.
 */
#define PERIODIC_CHECKS(_pcadj) {                              \
        if (dvmCheckSuspendQuick(self)) {                                   \
            EXPORT_PC();  /* need for precise GC */                         \
            dvmCheckSuspendPending(self);                                   \
        }                                                                   \
    }

/* File: c/opcommon.cpp */
/* forward declarations of goto targets */
// GOTO_TARGET_DECL(filledNewArray, bool methodCallRange);
// GOTO_TARGET_DECL(invokeVirtual, bool methodCallRange);
// GOTO_TARGET_DECL(invokeSuper, bool methodCallRange);
// GOTO_TARGET_DECL(invokeInterface, bool methodCallRange);
// GOTO_TARGET_DECL(invokeDirect, bool methodCallRange);
// GOTO_TARGET_DECL(invokeStatic, bool methodCallRange);
// GOTO_TARGET_DECL(invokeVirtualQuick, bool methodCallRange);
// GOTO_TARGET_DECL(invokeSuperQuick, bool methodCallRange);
// GOTO_TARGET_DECL(invokeMethod, bool methodCallRange, const Method* methodToCall,
//     u2 count, u2 regs);
// GOTO_TARGET_DECL(returnFromMethod);
// GOTO_TARGET_DECL(exceptionThrown);

/*
 * ===========================================================================
 *
 * What follows are opcode definitions shared between multiple opcodes with
 * minor substitutions handled by the C pre-processor.  These should probably
 * use the mterp substitution mechanism instead, with the code here moved
 * into common fragment files (like the asm "binop.S"), although it's hard
 * to give up the C preprocessor in favor of the much simpler text subst.
 *
 * ===========================================================================
 */

#define HANDLE_NUMCONV(_opcode, _opname, _fromtype, _totype)                \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s v%d,v%d", (_opname), vdst, vsrc1);                       \
        SET_REGISTER##_totype(vdst,                                         \
            GET_REGISTER##_fromtype(vsrc1));                                \
        FINISH(1);

#define HANDLE_FLOAT_TO_INT(_opcode, _opname, _fromvtype, _fromrtype,       \
        _tovtype, _tortype)                                                 \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
    {                                                                       \
        /* spec defines specific handling for +/- inf and NaN values */     \
        _fromvtype val;                                                     \
        _tovtype intMin, intMax, result;                                    \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s v%d,v%d", (_opname), vdst, vsrc1);                       \
        val = GET_REGISTER##_fromrtype(vsrc1);                              \
        intMin = (_tovtype) 1 << (sizeof(_tovtype) * 8 -1);                 \
        intMax = ~intMin;                                                   \
        result = (_tovtype) val;                                            \
        if (val >= intMax)          /* +inf */                              \
            result = intMax;                                                \
        else if (val <= intMin)     /* -inf */                              \
            result = intMin;                                                \
        else if (val != val)        /* NaN */                               \
            result = 0;                                                     \
        else                                                                \
            result = (_tovtype) val;                                        \
        SET_REGISTER##_tortype(vdst, result);                               \
    }                                                                       \
    FINISH(1);

#define HANDLE_INT_TO_SMALL(_opcode, _opname, _type)                        \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|int-to-%s v%d,v%d", (_opname), vdst, vsrc1);                \
        SET_REGISTER(vdst, (_type) GET_REGISTER(vsrc1));                    \
        FINISH(1);

/* NOTE: the comparison result is always a signed 4-byte integer */
#define HANDLE_OP_CMPX(_opcode, _opname, _varType, _type, _nanVal)          \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        int result;                                                         \
        u2 regs;                                                            \
        _varType val1, val2;                                                \
        vdst = INST_AA(inst);                                               \
        regs = FETCH(1);                                                    \
        vsrc1 = regs & 0xff;                                                \
        vsrc2 = regs >> 8;                                                  \
        MY_LOG_VERBOSE("|cmp%s v%d,v%d,v%d", (_opname), vdst, vsrc1, vsrc2);         \
        val1 = GET_REGISTER##_type(vsrc1);                                  \
        val2 = GET_REGISTER##_type(vsrc2);                                  \
        if (val1 == val2)                                                   \
            result = 0;                                                     \
        else if (val1 < val2)                                               \
            result = -1;                                                    \
        else if (val1 > val2)                                               \
            result = 1;                                                     \
        else                                                                \
            result = (_nanVal);                                             \
        MY_LOG_VERBOSE("+ result=%d", result);                                       \
        SET_REGISTER(vdst, result);                                         \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_IF_XX(_opcode, _opname, _cmp)                             \
    HANDLE_OPCODE(_opcode /*vA, vB, +CCCC*/)                                \
        vsrc1 = INST_A(inst);                                               \
        vsrc2 = INST_B(inst);                                               \
        if ((s4) GET_REGISTER(vsrc1) _cmp (s4) GET_REGISTER(vsrc2)) {       \
            int branchOffset = (s2)FETCH(1);    /* sign-extended */         \
            MY_LOG_VERBOSE("|if-%s v%d,v%d,+0x%04x", (_opname), vsrc1, vsrc2,        \
                branchOffset);                                              \
            MY_LOG_VERBOSE("> branch taken");                                        \
            if (branchOffset < 0)                                           \
                PERIODIC_CHECKS(branchOffset);                              \
            FINISH(branchOffset);                                           \
        } else {                                                            \
            MY_LOG_VERBOSE("|if-%s v%d,v%d,-", (_opname), vsrc1, vsrc2);             \
            FINISH(2);                                                      \
        }

#define HANDLE_OP_IF_XXZ(_opcode, _opname, _cmp)                            \
    HANDLE_OPCODE(_opcode /*vAA, +BBBB*/)                                   \
        vsrc1 = INST_AA(inst);                                              \
        if ((s4) GET_REGISTER(vsrc1) _cmp 0) {                              \
            int branchOffset = (s2)FETCH(1);    /* sign-extended */         \
            MY_LOG_VERBOSE("|if-%s v%d,+0x%04x", (_opname), vsrc1, branchOffset);    \
            MY_LOG_VERBOSE("> branch taken");                                        \
            if (branchOffset < 0)                                           \
                PERIODIC_CHECKS(branchOffset);                              \
            FINISH(branchOffset);                                           \
        } else {                                                            \
            MY_LOG_VERBOSE("|if-%s v%d,-", (_opname), vsrc1);                        \
            FINISH(2);                                                      \
        }

#define HANDLE_UNOP(_opcode, _opname, _pfx, _sfx, _type)                    \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s v%d,v%d", (_opname), vdst, vsrc1);                       \
        SET_REGISTER##_type(vdst, _pfx GET_REGISTER##_type(vsrc1) _sfx);    \
        FINISH(1);

#define HANDLE_OP_X_INT(_opcode, _opname, _op, _chkdiv)                     \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        u2 srcRegs;                                                         \
        vdst = INST_AA(inst);                                               \
        srcRegs = FETCH(1);                                                 \
        vsrc1 = srcRegs & 0xff;                                             \
        vsrc2 = srcRegs >> 8;                                               \
        MY_LOG_VERBOSE("|%s-int v%d,v%d", (_opname), vdst, vsrc1);                   \
        if (_chkdiv != 0) {                                                 \
            s4 firstVal, secondVal, result;                                 \
            firstVal = GET_REGISTER(vsrc1);                                 \
            secondVal = GET_REGISTER(vsrc2);                                \
            if (secondVal == 0) {                                           \
                EXPORT_PC();                                                \
                dvmThrowArithmeticException(env, "divide by zero");              \
                GOTO_exceptionThrown();                                     \
            }                                                               \
            if ((u4)firstVal == 0x80000000 && secondVal == -1) {            \
                if (_chkdiv == 1)                                           \
                    result = firstVal;  /* division */                      \
                else                                                        \
                    result = 0;         /* remainder */                     \
            } else {                                                        \
                result = firstVal _op secondVal;                            \
            }                                                               \
            SET_REGISTER(vdst, result);                                     \
        } else {                                                            \
            /* non-div/rem case */                                          \
            SET_REGISTER(vdst,                                              \
                (s4) GET_REGISTER(vsrc1) _op (s4) GET_REGISTER(vsrc2));     \
        }                                                                   \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_SHX_INT(_opcode, _opname, _cast, _op)                     \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        u2 srcRegs;                                                         \
        vdst = INST_AA(inst);                                               \
        srcRegs = FETCH(1);                                                 \
        vsrc1 = srcRegs & 0xff;                                             \
        vsrc2 = srcRegs >> 8;                                               \
        MY_LOG_VERBOSE("|%s-int v%d,v%d", (_opname), vdst, vsrc1);                   \
        SET_REGISTER(vdst,                                                  \
            _cast GET_REGISTER(vsrc1) _op (GET_REGISTER(vsrc2) & 0x1f));    \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_X_INT_LIT16(_opcode, _opname, _op, _chkdiv)               \
    HANDLE_OPCODE(_opcode /*vA, vB, #+CCCC*/)                               \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        vsrc2 = FETCH(1);                                                   \
        MY_LOG_VERBOSE("|%s-int/lit16 v%d,v%d,#+0x%04x",                             \
            (_opname), vdst, vsrc1, vsrc2);                                 \
        if (_chkdiv != 0) {                                                 \
            s4 firstVal, result;                                            \
            firstVal = GET_REGISTER(vsrc1);                                 \
            if ((s2) vsrc2 == 0) {                                          \
                EXPORT_PC();                                                \
                dvmThrowArithmeticException(env, "divide by zero");              \
                GOTO_exceptionThrown();                                     \
            }                                                               \
            if ((u4)firstVal == 0x80000000 && ((s2) vsrc2) == -1) {         \
                /* won't generate /lit16 instr for this; check anyway */    \
                if (_chkdiv == 1)                                           \
                    result = firstVal;  /* division */                      \
                else                                                        \
                    result = 0;         /* remainder */                     \
            } else {                                                        \
                result = firstVal _op (s2) vsrc2;                           \
            }                                                               \
            SET_REGISTER(vdst, result);                                     \
        } else {                                                            \
            /* non-div/rem case */                                          \
            SET_REGISTER(vdst, GET_REGISTER(vsrc1) _op (s2) vsrc2);         \
        }                                                                   \
        FINISH(2);

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
                dvmThrowArithmeticException(env, "divide by zero");              \
                GOTO_exceptionThrown();                                     \
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
            SET_REGISTER(vdst,                                              \
                (s4) GET_REGISTER(vsrc1) _op (s1) vsrc2);                   \
        }                                                                   \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_SHX_INT_LIT8(_opcode, _opname, _cast, _op)                \
    HANDLE_OPCODE(_opcode /*vAA, vBB, #+CC*/)                               \
    {                                                                       \
        u2 litInfo;                                                         \
        vdst = INST_AA(inst);                                               \
        litInfo = FETCH(1);                                                 \
        vsrc1 = litInfo & 0xff;                                             \
        vsrc2 = litInfo >> 8;       /* constant */                          \
        MY_LOG_VERBOSE("|%s-int/lit8 v%d,v%d,#+0x%02x",                              \
            (_opname), vdst, vsrc1, vsrc2);                                 \
        SET_REGISTER(vdst,                                                  \
            _cast GET_REGISTER(vsrc1) _op (vsrc2 & 0x1f));                  \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_X_INT_2ADDR(_opcode, _opname, _op, _chkdiv)               \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s-int-2addr v%d,v%d", (_opname), vdst, vsrc1);             \
        if (_chkdiv != 0) {                                                 \
            s4 firstVal, secondVal, result;                                 \
            firstVal = GET_REGISTER(vdst);                                  \
            secondVal = GET_REGISTER(vsrc1);                                \
            if (secondVal == 0) {                                           \
                EXPORT_PC();                                                \
                dvmThrowArithmeticException(env, "divide by zero");              \
                GOTO_exceptionThrown();                                     \
            }                                                               \
            if ((u4)firstVal == 0x80000000 && secondVal == -1) {            \
                if (_chkdiv == 1)                                           \
                    result = firstVal;  /* division */                      \
                else                                                        \
                    result = 0;         /* remainder */                     \
            } else {                                                        \
                result = firstVal _op secondVal;                            \
            }                                                               \
            SET_REGISTER(vdst, result);                                     \
        } else {                                                            \
            SET_REGISTER(vdst,                                              \
                (s4) GET_REGISTER(vdst) _op (s4) GET_REGISTER(vsrc1));      \
        }                                                                   \
        FINISH(1);

#define HANDLE_OP_SHX_INT_2ADDR(_opcode, _opname, _cast, _op)               \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s-int-2addr v%d,v%d", (_opname), vdst, vsrc1);             \
        SET_REGISTER(vdst,                                                  \
            _cast GET_REGISTER(vdst) _op (GET_REGISTER(vsrc1) & 0x1f));     \
        FINISH(1);

#define HANDLE_OP_X_LONG(_opcode, _opname, _op, _chkdiv)                    \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        u2 srcRegs;                                                         \
        vdst = INST_AA(inst);                                               \
        srcRegs = FETCH(1);                                                 \
        vsrc1 = srcRegs & 0xff;                                             \
        vsrc2 = srcRegs >> 8;                                               \
        MY_LOG_VERBOSE("|%s-long v%d,v%d,v%d", (_opname), vdst, vsrc1, vsrc2);       \
        if (_chkdiv != 0) {                                                 \
            s8 firstVal, secondVal, result;                                 \
            firstVal = GET_REGISTER_WIDE(vsrc1);                            \
            secondVal = GET_REGISTER_WIDE(vsrc2);                           \
            if (secondVal == 0LL) {                                         \
                EXPORT_PC();                                                \
                dvmThrowArithmeticException(env, "divide by zero");              \
                GOTO_exceptionThrown();                                     \
            }                                                               \
            if ((u8)firstVal == 0x8000000000000000ULL &&                    \
                secondVal == -1LL)                                          \
            {                                                               \
                if (_chkdiv == 1)                                           \
                    result = firstVal;  /* division */                      \
                else                                                        \
                    result = 0;         /* remainder */                     \
            } else {                                                        \
                result = firstVal _op secondVal;                            \
            }                                                               \
            SET_REGISTER_WIDE(vdst, result);                                \
        } else {                                                            \
            SET_REGISTER_WIDE(vdst,                                         \
                (s8) GET_REGISTER_WIDE(vsrc1) _op (s8) GET_REGISTER_WIDE(vsrc2)); \
        }                                                                   \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_SHX_LONG(_opcode, _opname, _cast, _op)                    \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        u2 srcRegs;                                                         \
        vdst = INST_AA(inst);                                               \
        srcRegs = FETCH(1);                                                 \
        vsrc1 = srcRegs & 0xff;                                             \
        vsrc2 = srcRegs >> 8;                                               \
        MY_LOG_VERBOSE("|%s-long v%d,v%d,v%d", (_opname), vdst, vsrc1, vsrc2);       \
        SET_REGISTER_WIDE(vdst,                                             \
            _cast GET_REGISTER_WIDE(vsrc1) _op (GET_REGISTER(vsrc2) & 0x3f)); \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_X_LONG_2ADDR(_opcode, _opname, _op, _chkdiv)              \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s-long-2addr v%d,v%d", (_opname), vdst, vsrc1);            \
        if (_chkdiv != 0) {                                                 \
            s8 firstVal, secondVal, result;                                 \
            firstVal = GET_REGISTER_WIDE(vdst);                             \
            secondVal = GET_REGISTER_WIDE(vsrc1);                           \
            if (secondVal == 0LL) {                                         \
                EXPORT_PC();                                                \
                dvmThrowArithmeticException(env, "divide by zero");              \
                GOTO_exceptionThrown();                                     \
            }                                                               \
            if ((u8)firstVal == 0x8000000000000000ULL &&                    \
                secondVal == -1LL)                                          \
            {                                                               \
                if (_chkdiv == 1)                                           \
                    result = firstVal;  /* division */                      \
                else                                                        \
                    result = 0;         /* remainder */                     \
            } else {                                                        \
                result = firstVal _op secondVal;                            \
            }                                                               \
            SET_REGISTER_WIDE(vdst, result);                                \
        } else {                                                            \
            SET_REGISTER_WIDE(vdst,                                         \
                (s8) GET_REGISTER_WIDE(vdst) _op (s8)GET_REGISTER_WIDE(vsrc1));\
        }                                                                   \
        FINISH(1);

#define HANDLE_OP_SHX_LONG_2ADDR(_opcode, _opname, _cast, _op)              \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s-long-2addr v%d,v%d", (_opname), vdst, vsrc1);            \
        SET_REGISTER_WIDE(vdst,                                             \
            _cast GET_REGISTER_WIDE(vdst) _op (GET_REGISTER(vsrc1) & 0x3f)); \
        FINISH(1);

#define HANDLE_OP_X_FLOAT(_opcode, _opname, _op)                            \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        u2 srcRegs;                                                         \
        vdst = INST_AA(inst);                                               \
        srcRegs = FETCH(1);                                                 \
        vsrc1 = srcRegs & 0xff;                                             \
        vsrc2 = srcRegs >> 8;                                               \
        MY_LOG_VERBOSE("|%s-float v%d,v%d,v%d", (_opname), vdst, vsrc1, vsrc2);      \
        SET_REGISTER_FLOAT(vdst,                                            \
            GET_REGISTER_FLOAT(vsrc1) _op GET_REGISTER_FLOAT(vsrc2));       \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_X_DOUBLE(_opcode, _opname, _op)                           \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        u2 srcRegs;                                                         \
        vdst = INST_AA(inst);                                               \
        srcRegs = FETCH(1);                                                 \
        vsrc1 = srcRegs & 0xff;                                             \
        vsrc2 = srcRegs >> 8;                                               \
        MY_LOG_VERBOSE("|%s-double v%d,v%d,v%d", (_opname), vdst, vsrc1, vsrc2);     \
        SET_REGISTER_DOUBLE(vdst,                                           \
            GET_REGISTER_DOUBLE(vsrc1) _op GET_REGISTER_DOUBLE(vsrc2));     \
    }                                                                       \
    FINISH(2);

#define HANDLE_OP_X_FLOAT_2ADDR(_opcode, _opname, _op)                      \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s-float-2addr v%d,v%d", (_opname), vdst, vsrc1);           \
        SET_REGISTER_FLOAT(vdst,                                            \
            GET_REGISTER_FLOAT(vdst) _op GET_REGISTER_FLOAT(vsrc1));        \
        FINISH(1);

#define HANDLE_OP_X_DOUBLE_2ADDR(_opcode, _opname, _op)                     \
    HANDLE_OPCODE(_opcode /*vA, vB*/)                                       \
        vdst = INST_A(inst);                                                \
        vsrc1 = INST_B(inst);                                               \
        MY_LOG_VERBOSE("|%s-double-2addr v%d,v%d", (_opname), vdst, vsrc1);          \
        SET_REGISTER_DOUBLE(vdst,                                           \
            GET_REGISTER_DOUBLE(vdst) _op GET_REGISTER_DOUBLE(vsrc1));      \
        FINISH(1);

#define HANDLE_OP_AGET(_opcode, _opname, _type, _regsize)                   \
    HANDLE_OPCODE(_opcode /*vAA, vBB, vCC*/)                                \
    {                                                                       \
        ArrayObject* arrayObj;                                              \
        u2 arrayInfo;                                                       \
        EXPORT_PC();                                                        \
        vdst = INST_AA(inst);                                               \
        arrayInfo = FETCH(1);                                               \
        vsrc1 = arrayInfo & 0xff;    /* array ptr */                        \
        vsrc2 = arrayInfo >> 8;      /* index */                            \
        MY_LOG_VERBOSE("|aget%s v%d,v%d,v%d", (_opname), vdst, vsrc1, vsrc2);        \
        arrayObj = (ArrayObject*) GET_REGISTER(vsrc1);                      \
        if (!checkForNull(env, (Object*) arrayObj))                              \
            GOTO_exceptionThrown();                                         \
        if (GET_REGISTER(vsrc2) >= arrayObj->length) {                      \
            dvmThrowArrayIndexOutOfBoundsException(                         \
                arrayObj->length, GET_REGISTER(vsrc2));                     \
            GOTO_exceptionThrown();                                         \
        }                                                                   \
        SET_REGISTER##_regsize(vdst,                                        \
            ((_type*)(void*)arrayObj->contents)[GET_REGISTER(vsrc2)]);      \
        MY_LOG_VERBOSE("+ AGET[%d]=%#x", GET_REGISTER(vsrc2), GET_REGISTER(vdst));   \
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
        if (!checkForNull(env, (Object*) arrayObj))                              \
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

/*--- start of opcodes ---*/

/* File: c/OP_NOP.cpp */
HANDLE_OPCODE(OP_NOP)
    FINISH(1);
OP_END

/* File: c/OP_MOVE.cpp */
HANDLE_OPCODE(OP_MOVE /*vA, vB*/)
    vdst = INST_A(inst);
    vsrc1 = INST_B(inst);
    MY_LOG_VERBOSE("|move%s v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(1);
OP_END

/* File: c/OP_MOVE_FROM16.cpp */
HANDLE_OPCODE(OP_MOVE_FROM16 /*vAA, vBBBB*/)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|move%s/from16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_FROM16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(2);
OP_END

/* File: c/OP_MOVE_16.cpp */
HANDLE_OPCODE(OP_MOVE_16 /*vAAAA, vBBBB*/)
    vdst = FETCH(1);
    vsrc1 = FETCH(2);
    MY_LOG_VERBOSE("|move%s/16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(3);
OP_END

/* File: c/OP_MOVE_WIDE.cpp */
HANDLE_OPCODE(OP_MOVE_WIDE /*vA, vB*/)
    /* IMPORTANT: must correctly handle overlapping registers, e.g. both
     * "move-wide v6, v7" and "move-wide v7, v6" */
    vdst = INST_A(inst);
    vsrc1 = INST_B(inst);
    MY_LOG_VERBOSE("|move-wide v%d,v%d %s(v%d=0x%08llx)", vdst, vsrc1,
        kSpacing+5, vdst, GET_REGISTER_WIDE(vsrc1));
    SET_REGISTER_WIDE(vdst, GET_REGISTER_WIDE(vsrc1));
    FINISH(1);
OP_END

/* File: c/OP_MOVE_WIDE_FROM16.cpp */
HANDLE_OPCODE(OP_MOVE_WIDE_FROM16 /*vAA, vBBBB*/)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|move-wide/from16 v%d,v%d  (v%d=0x%08llx)", vdst, vsrc1,
        vdst, GET_REGISTER_WIDE(vsrc1));
    SET_REGISTER_WIDE(vdst, GET_REGISTER_WIDE(vsrc1));
    FINISH(2);
OP_END

/* File: c/OP_MOVE_WIDE_16.cpp */
HANDLE_OPCODE(OP_MOVE_WIDE_16 /*vAAAA, vBBBB*/)
    vdst = FETCH(1);
    vsrc1 = FETCH(2);
    MY_LOG_VERBOSE("|move-wide/16 v%d,v%d %s(v%d=0x%08llx)", vdst, vsrc1,
        kSpacing+8, vdst, GET_REGISTER_WIDE(vsrc1));
    SET_REGISTER_WIDE(vdst, GET_REGISTER_WIDE(vsrc1));
    FINISH(3);
OP_END

/* File: c/OP_MOVE_OBJECT.cpp */
/* File: c/OP_MOVE.cpp */
HANDLE_OPCODE(OP_MOVE_OBJECT /*vA, vB*/)
    vdst = INST_A(inst);
    vsrc1 = INST_B(inst);
    MY_LOG_VERBOSE("|move%s v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(1);
OP_END


/* File: c/OP_MOVE_OBJECT_FROM16.cpp */
/* File: c/OP_MOVE_FROM16.cpp */
HANDLE_OPCODE(OP_MOVE_OBJECT_FROM16 /*vAA, vBBBB*/)
    vdst = INST_AA(inst);
    vsrc1 = FETCH(1);
    MY_LOG_VERBOSE("|move%s/from16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_FROM16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(2);
OP_END


/* File: c/OP_MOVE_OBJECT_16.cpp */
/* File: c/OP_MOVE_16.cpp */
HANDLE_OPCODE(OP_MOVE_OBJECT_16 /*vAAAA, vBBBB*/)
    vdst = FETCH(1);
    vsrc1 = FETCH(2);
    MY_LOG_VERBOSE("|move%s/16 v%d,v%d %s(v%d=0x%08x)",
        (INST_INST(inst) == OP_MOVE_16) ? "" : "-object", vdst, vsrc1,
        kSpacing, vdst, GET_REGISTER(vsrc1));
    SET_REGISTER(vdst, GET_REGISTER(vsrc1));
    FINISH(3);
OP_END


/* File: c/OP_MOVE_RESULT.cpp */
HANDLE_OPCODE(OP_MOVE_RESULT /*vAA*/)
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


// TODO 异常还不支持。
/* File: c/OP_MOVE_EXCEPTION.cpp */
HANDLE_OPCODE(OP_MOVE_EXCEPTION /*vAA*/)
    vdst = INST_AA(inst);
    MY_LOG_VERBOSE("|move-exception v%d", vdst);
    /*assert(self->exception != NULL);*/
    /*SET_REGISTER(vdst, (u4)self->exception);*/
    /*dvmClearException(self);*/
    FINISH(1);
OP_END

/* File: c/OP_RETURN_VOID.cpp */
HANDLE_OPCODE(OP_RETURN_VOID /**/)
    MY_LOG_VERBOSE("|return-void");
#ifndef NDEBUG
    retval.j = 0xababababULL;    // placate valgrind
#endif
    /*GOTO_returnFromMethod();*/
    GOTO_bail();
OP_END

/* File: c/OP_RETURN.cpp */
HANDLE_OPCODE(OP_RETURN /*vAA*/)
    vsrc1 = INST_AA(inst);
    MY_LOG_VERBOSE("|return%s v%d",
        (INST_INST(inst) == OP_RETURN) ? "" : "-object", vsrc1);
    retval.i = GET_REGISTER(vsrc1);
    /*GOTO_returnFromMethod();*/
    GOTO_bail();
OP_END

/* File: c/OP_RETURN_WIDE.cpp */
HANDLE_OPCODE(OP_RETURN_WIDE /*vAA*/)
    vsrc1 = INST_AA(inst);
    MY_LOG_VERBOSE("|return-wide v%d", vsrc1);
    retval.j = GET_REGISTER_WIDE(vsrc1);
    /*GOTO_returnFromMethod();*/
    GOTO_bail();
OP_END

/* File: c/OP_RETURN_OBJECT.cpp */
/* File: c/OP_RETURN.cpp */
HANDLE_OPCODE(OP_RETURN_OBJECT /*vAA*/)
    vsrc1 = INST_AA(inst);
    MY_LOG_VERBOSE("|return%s v%d",
        (INST_INST(inst) == OP_RETURN) ? "" : "-object", vsrc1);
    retval.i = GET_REGISTER(vsrc1);
    /*GOTO_returnFromMethod();*/
    GOTO_bail();
OP_END


/* File: c/OP_CONST_4.cpp */
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
HANDLE_OPCODE(OP_GOTO)
HANDLE_OPCODE(OP_GOTO_16)
HANDLE_OPCODE(OP_GOTO_32)
HANDLE_OPCODE(OP_PACKED_SWITCH)
HANDLE_OPCODE(OP_SPARSE_SWITCH)
HANDLE_OPCODE(OP_CMPL_FLOAT)
HANDLE_OPCODE(OP_CMPG_FLOAT)
HANDLE_OPCODE(OP_CMPL_DOUBLE)
HANDLE_OPCODE(OP_CMPG_DOUBLE)
HANDLE_OPCODE(OP_CMP_LONG)
HANDLE_OPCODE(OP_IF_EQ)
HANDLE_OPCODE(OP_IF_NE)
HANDLE_OPCODE(OP_IF_LT)
HANDLE_OPCODE(OP_IF_GE)
HANDLE_OPCODE(OP_IF_GT)
HANDLE_OPCODE(OP_IF_LE)
HANDLE_OPCODE(OP_IF_EQZ)
HANDLE_OPCODE(OP_IF_NEZ)
HANDLE_OPCODE(OP_IF_LTZ)
HANDLE_OPCODE(OP_IF_GEZ)
HANDLE_OPCODE(OP_IF_GTZ)
HANDLE_OPCODE(OP_IF_LEZ)
HANDLE_OPCODE(OP_UNUSED_3E)
HANDLE_OPCODE(OP_UNUSED_3F)
HANDLE_OPCODE(OP_UNUSED_40)
HANDLE_OPCODE(OP_UNUSED_41)
HANDLE_OPCODE(OP_UNUSED_42)
HANDLE_OPCODE(OP_UNUSED_43)
HANDLE_OPCODE(OP_AGET)
HANDLE_OPCODE(OP_AGET_WIDE)
HANDLE_OPCODE(OP_AGET_OBJECT)
HANDLE_OPCODE(OP_AGET_BOOLEAN)
HANDLE_OPCODE(OP_AGET_BYTE)
HANDLE_OPCODE(OP_AGET_CHAR)
HANDLE_OPCODE(OP_AGET_SHORT)
HANDLE_OPCODE(OP_APUT)
HANDLE_OPCODE(OP_APUT_WIDE)
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

/* File: c/OP_MUL_INT_LIT16.cpp */
HANDLE_OP_X_INT_LIT16(OP_MUL_INT_LIT16, "mul", *, 0)
OP_END

/* File: c/OP_DIV_INT_LIT16.cpp */
HANDLE_OP_X_INT_LIT16(OP_DIV_INT_LIT16, "div", /, 1)
OP_END

/* File: c/OP_REM_INT_LIT16.cpp */
HANDLE_OP_X_INT_LIT16(OP_REM_INT_LIT16, "rem", %, 2)
OP_END

/* File: c/OP_AND_INT_LIT16.cpp */
HANDLE_OP_X_INT_LIT16(OP_AND_INT_LIT16, "and", &, 0)
OP_END

/* File: c/OP_OR_INT_LIT16.cpp */
HANDLE_OP_X_INT_LIT16(OP_OR_INT_LIT16,  "or",  |, 0)
OP_END

/* File: c/OP_XOR_INT_LIT16.cpp */
HANDLE_OP_X_INT_LIT16(OP_XOR_INT_LIT16, "xor", ^, 0)
OP_END

/* File: c/OP_ADD_INT_LIT8.cpp */
HANDLE_OP_X_INT_LIT8(OP_ADD_INT_LIT8,   "add", +, 0)
OP_END

/* File: c/OP_RSUB_INT_LIT8.cpp */
HANDLE_OPCODE(OP_RSUB_INT_LIT8 /*vAA, vBB, #+CC*/)
{
    u2 litInfo;
    vdst = INST_AA(inst);
    litInfo = FETCH(1);
    vsrc1 = litInfo & 0xff;
    vsrc2 = litInfo >> 8;
    MY_LOG_VERBOSE("|%s-int/lit8 v%d,v%d,#+0x%02x", "rsub", vdst, vsrc1, vsrc2);
    SET_REGISTER(vdst, (s1) vsrc2 - (s4) GET_REGISTER(vsrc1));
}
FINISH(2);
OP_END

/* File: c/OP_MUL_INT_LIT8.cpp */
HANDLE_OP_X_INT_LIT8(OP_MUL_INT_LIT8,   "mul", *, 0)
OP_END

/* File: c/OP_DIV_INT_LIT8.cpp */
HANDLE_OP_X_INT_LIT8(OP_DIV_INT_LIT8,   "div", /, 1)
OP_END

/* File: c/OP_REM_INT_LIT8.cpp */
HANDLE_OP_X_INT_LIT8(OP_REM_INT_LIT8,   "rem", %, 2)
OP_END

/* File: c/OP_AND_INT_LIT8.cpp */
HANDLE_OP_X_INT_LIT8(OP_AND_INT_LIT8,   "and", &, 0)
OP_END

/* File: c/OP_OR_INT_LIT8.cpp */
HANDLE_OP_X_INT_LIT8(OP_OR_INT_LIT8,    "or",  |, 0)
OP_END

/* File: c/OP_XOR_INT_LIT8.cpp */
HANDLE_OP_X_INT_LIT8(OP_XOR_INT_LIT8,   "xor", ^, 0)
OP_END

/* File: c/OP_SHL_INT_LIT8.cpp */
HANDLE_OP_SHX_INT_LIT8(OP_SHL_INT_LIT8,   "shl", (s4), <<)
OP_END

/* File: c/OP_SHR_INT_LIT8.cpp */
HANDLE_OP_SHX_INT_LIT8(OP_SHR_INT_LIT8,   "shr", (s4), >>)
OP_END

/* File: c/OP_USHR_INT_LIT8.cpp */
HANDLE_OP_SHX_INT_LIT8(OP_USHR_INT_LIT8,  "ushr", (u4), >>)

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

// TODO 异常现在不支持。
    /*
     * Jump here when the code throws an exception.
     *
     * By the time we get here, the Throwable has been created and the stack
     * trace has been saved off.
     */
GOTO_TARGET(exceptionThrown)
GOTO_TARGET_END

bail:
    if (NULL != params) {
        delete[] params;
    }
    MY_LOG_INFO("|-- Leaving interpreter loop");
    return retval;
}
