package buwai.android.dexlib2.helper;

import org.jf.dexlib2.dexbacked.DexBackedDexFile;
import org.jf.dexlib2.dexbacked.DexBackedMethod;
import org.jf.dexlib2.dexbacked.DexBackedMethodImplementation;
import org.jf.dexlib2.dexbacked.raw.CodeItem;
import org.jf.dexlib2.iface.Method;
import org.jf.dexlib2.iface.MethodImplementation;
import org.jf.dexlib2.iface.instruction.Instruction;
import org.jf.dexlib2.iface.reference.MethodReference;
import org.jf.dexlib2.immutable.ImmutableMethod;
import org.jf.dexlib2.immutable.ImmutableMethodImplementation;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;

/**
 * 增强对Method类的操作。
 * Created by buwai on 2015/4/1.
 */
public class MethodHelper {

    public static final String[] paramNames = { "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",
            "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z" };

    /**
     * 在起始位置插入指令。
     * @param method 方法对象。
     * @param insts 要插入的指令。
     * @param regIncrement 寄存器增量。因为插入的指令可能需要增加寄存器。
     * @return 返回新的方法对象。
     */
    public static Method insertInstructionInStart(Method method, List<Instruction> insts, int regIncrement) {
        MethodImplementation oldmi = method.getImplementation();
        List<Instruction> newInsts = new ArrayList<>();

        // 在起始位置插入指令。
        for (Instruction inst : insts) {
            newInsts.add(inst);
        }

        // 再将原有指令插入。
        for (Instruction inst : oldmi.getInstructions()) {
            newInsts.add(inst);
        }

        int regCount = oldmi.getRegisterCount() + regIncrement;
        ImmutableMethodImplementation newmi = new ImmutableMethodImplementation(regCount, newInsts, oldmi.getTryBlocks(), oldmi.getDebugItems());
        return new ImmutableMethod(method.getDefiningClass(), method.getName(), method.getParameters(), method.getReturnType(), method.getAccessFlags(), method.getAnnotations(), newmi);
    }

    /**
     * 获得方法中的指令。
     * @param dexBackedMethod
     * @return 返回方法中的指令。
     */
    public static short[] getInstructions(DexBackedMethod dexBackedMethod) {
        int codeOffset = getCodeOffset(dexBackedMethod);
        DexBackedDexFile dexFile = dexBackedMethod.getImplementation().dexFile;

        // 这个size指是u2数组的元素个数。
        int instructionsSize = dexFile.readSmallUint(codeOffset + CodeItem.INSTRUCTION_COUNT_OFFSET);

        int instructionsStartOffset = codeOffset + CodeItem.INSTRUCTION_START_OFFSET;
        short[] insts = new short[instructionsSize];
        for (int i = 0; i < instructionsSize; i++) {
            insts[i] = (short) dexFile.readUshort(instructionsStartOffset);
            instructionsStartOffset += 2;
        }

        return insts;
    }

    /**
     * 获得方法的code_item结构的偏移。
     * @param dexBackedMethod
     * @return 返回方法code_item结构的偏移。
     */
    public static int getCodeOffset(DexBackedMethod dexBackedMethod) {
        int offset = -1;
        DexBackedMethodImplementation dbmi = dexBackedMethod.getImplementation();
        try {
            Field field = dbmi.getClass().getDeclaredField("codeOffset");
            field.setAccessible(true);
            offset = (Integer) field.get(dbmi);
        } catch (NoSuchFieldException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        }
        return offset;
    }

    /**
     * 生成方法签名。
     *
     * @param mr
     * @return
     */
    public static String genMethodSig(MethodReference mr) {
        List<? extends CharSequence> params = mr.getParameterTypes();
        StringBuilder sb = new StringBuilder();
        sb.append("(");
        int length = params.size();
        for (int i = 0; i < length; i++) {
            sb.append(params.get(i));
        }
        sb.append(")");
        sb.append(mr.getReturnType());
        return sb.toString();
    }

    /**
     * 生成方法参数的短类型。
     * @param mr
     * @return 返回方法参数的短类型。
     */
    public static String genParamsShortDesc(MethodReference mr) {
        List<? extends CharSequence> params = mr.getParameterTypes();
        StringBuilder sb = new StringBuilder();
        int length = params.size();
        for (int i = 0; i < length; i++) {
            char chType = params.get(i).charAt(0);
            if ('[' == chType) {
                sb.append('L');
            } else {
                sb.append(chType);
            }
        }
        return sb.toString();
    }

    /**
     * 生成在native中的类型。
     * @param mr
     * @return
     */
    public static String genTypeInNative (MethodReference mr) {
        String type = mr.getReturnType();
        return genTypeInNative(type);
    }

    /**
     * 生成在native中的类型。
     * @param type
     * @return
     */
    public static String genTypeInNative (CharSequence type) {
        char cType = type.charAt(0);
        switch (cType) {
            case 'V':
                return "void";
            case 'Z':
                return "jboolean";
            case 'B':
                return "jbyte";
            case 'S':
                return "jshort";
            case 'C':
                return "jchar";
            case 'I':
                return "jint";
            case 'J':
                return "jlong";
            case 'F':
                return "jfloat";
            case 'D':
                return "jdouble";
            case 'L':
                return "jobject";
            case '[':
                return "jobject";
            default:
                return null;
        }
    }

    /**
     * 生成JNI方法参数列表。
     *
     * @param mr
     * @return 返回方法参数列表。
     */
    public static String genParamTypeListInNative(MethodReference mr) {
        List<? extends CharSequence> params = mr.getParameterTypes();
        StringBuilder paramsList = new StringBuilder();
        paramsList.append("JNIEnv *env, jobject thiz");
        int length = params.size();

        for (int i = 0; i < length; i++) {
            paramsList.append(", ");
            paramsList.append(genTypeInNative(mr));
            paramsList.append(" ");
            paramsList.append(paramNames[i]);
        }
        return paramsList.toString();
    }

    /**
     * 生成JNINativeMethod结构的数据。
     * @param method
     * @return
     */
    public static String genJNINativeMethod(Method method) {
        return String.format("{ \"%s\", \"%s\", (void*)%s }, ", method.getName(), genMethodSig(method), method.getName());
    }

}
