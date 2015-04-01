package buwai.android.shell.separator;

import buwai.android.shell.base.Common;
import buwai.android.shell.base.TypeDescription;
import buwai.android.shell.base.helper.TypeDescriptionHelper;
import org.jf.dexlib2.AccessFlags;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.Opcode;
import org.jf.dexlib2.iface.*;
import org.jf.dexlib2.iface.instruction.Instruction;
import org.jf.dexlib2.immutable.ImmutableClassDef;
import org.jf.dexlib2.immutable.ImmutableMethod;
import org.jf.dexlib2.immutable.ImmutableMethodImplementation;
import org.jf.dexlib2.immutable.ImmutableMethodParameter;
import org.jf.dexlib2.immutable.instruction.ImmutableInstruction21c;
import org.jf.dexlib2.immutable.instruction.ImmutableInstruction35c;
import org.jf.dexlib2.immutable.instruction.ImmutableInstruction3rc;
import org.jf.dexlib2.immutable.reference.ImmutableStringReference;
import org.jf.dexlib2.rewriter.*;

import javax.annotation.Nonnull;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * 向类的"<clinit>"中插入System.loadLibrary语句。
 * Created by buwai on 2015/4/1.
 */
public class InstructionInsert01 {

    DexFile mDexFile;
    TypeDescription mClassDesc;

    public InstructionInsert01(File dexFile, final TypeDescription classDesc) throws IOException {
        mDexFile = DexFileFactory.loadDexFile(dexFile, Common.API); // 加载dex。
        mClassDesc = classDesc;
    }

    public void insert() {
        DexFile newDexFile = new DexRewriter(new RewriterModule() {
            @Nonnull
            @Override
            public Rewriter<ClassDef> getClassDefRewriter(Rewriters rewriters) {
                return new ClassDefRewriter(rewriters) {
                    @Nonnull
                    @Override
                    public ClassDef rewrite(@Nonnull ClassDef classDef) {
                        classDef.getMethods().
                        if (mClassDesc.equals(TypeDescriptionHelper.convertByClassDef(classDef))) {
                            boolean isFindClinit = false;
                            for (Method method : classDef.getMethods()) {
                                if (method.getName().equals("<clinit>")) {
                                    isFindClinit = true;
                                    ImmutableClassDef newClassDef = new ImmutableClassDef()
                                }
                            }
                            if (!isFindClinit) {
                                Im
                            }
                        } else {
                            return super.rewrite(classDef);
                        }
                    }
                };
            }
        }).rewriteDexFile(dexFile);
        DexFileFactory.writeDexFile(src.getAbsolutePath(), newDexFile);
    }

    /**
     * 生成System.loadLibrary指令。
     *
     * @param method
     * @return
     */
    private ImmutableMethod genLoadLibrary(Method method) {
        MethodImplementation oldMethod = method.getImplementation();
        List<Instruction> newInsts = new ArrayList<>();
        int regIndex = oldMethod.getRegisterCount();
        ImmutableStringReference str = new ImmutableStringReference(Common.SO_NAME);
        // const-string vX, SO_NAME
        ImmutableInstruction21c i0 = new ImmutableInstruction21c(Opcode.CONST_STRING, regIndex, str);
        newInsts.add(i0);

        List<MethodParameter> params = new ArrayList<>();
        params.add(new ImmutableMethodParameter("Ljava/lang/String;", null, null));
        ImmutableMethod m = new ImmutableMethod("Ljava/lang/System", "loadLibrary", params, "V", AccessFlags.PUBLIC.getValue(), null, null);

        if (regIndex > 15) {
            ImmutableInstruction3rc i1 = new ImmutableInstruction3rc(Opcode.INVOKE_STATIC_RANGE, regIndex, 1, m);
            newInsts.add(i1);
        } else {
            ImmutableInstruction35c i1 = new ImmutableInstruction35c(Opcode.INVOKE_STATIC, 1, regIndex, 0, 0, 0, 0, m);
            newInsts.add(i1);
        }

        for (Instruction ins : oldMethod.getInstructions()) {
            newInsts.add(ins);
        }


        ImmutableMethodImplementation newMethodImp = new ImmutableMethodImplementation(regIndex, newInsts, oldMethod.getTryBlocks(), oldMethod.getDebugItems());
        ImmutableMethod newMethod = new ImmutableMethod(method.getDefiningClass(), method.getName(), method.getParameters(), method.getReturnType(), method.getAccessFlags(), method.getAnnotations(), newMethodImp);
        return newMethod;
    }

}
