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
    public static byte[] getInstructions(DexBackedMethod dexBackedMethod) {
        int codeOffset = getCodeOffset(dexBackedMethod);
        DexBackedDexFile dexFile = dexBackedMethod.getImplementation().dexFile;

        // 这个size指是u2数组的元素个数。
        int instructionsSize = dexFile.readSmallUint(codeOffset + CodeItem.INSTRUCTION_COUNT_OFFSET);
        // 以4字节对齐。
        if (1 == instructionsSize % 2) {
            instructionsSize += 1;
        }

        int instructionsStartOffset = codeOffset + CodeItem.INSTRUCTION_START_OFFSET;
        short[] insts = new short[instructionsSize];
        for (int i = 0; i < instructionsSize; i++) {
            insts[i] = (short) dexFile.readUshort(instructionsStartOffset);
            instructionsStartOffset += 2;
        }

        byte[] byteInsts = new byte[instructionsSize * 2];
        for (int i = 0, j = 0; i < instructionsSize; i++) {
            short s = insts[i];
            byteInsts[j++] = (byte) (s & 0xff);
            byteInsts[j++] = (byte) ((s >> 8) & 0xff);
        }
        return byteInsts;
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

}
