package buwai.android.shell.separator;

import buwai.android.dexlib2.helper.ClassDefHelper;
import buwai.android.dexlib2.helper.MethodHelper;
import buwai.android.shell.base.Common;
import buwai.android.shell.base.TypeDescription;
import buwai.android.shell.base.helper.TypeDescriptionHelper;
import org.jf.dexlib2.AccessFlags;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.Opcode;
import org.jf.dexlib2.iface.*;
import org.jf.dexlib2.iface.instruction.Instruction;
import org.jf.dexlib2.immutable.ImmutableMethod;
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

    File mSrc;
    DexFile mDexFile;
    TypeDescription mClassDesc;

    public InstructionInsert01(File dexFile, final TypeDescription classDesc) throws IOException {
        mSrc = dexFile;
        mDexFile = DexFileFactory.loadDexFile(dexFile, Common.API); // 加载dex。
        mClassDesc = classDesc;
    }

    public void insert() throws IOException {
        DexFile newDexFile = new DexRewriter(new RewriterModule() {
            @Nonnull
            @Override
            public Rewriter<ClassDef> getClassDefRewriter(Rewriters rewriters) {
                return new ClassDefRewriter(rewriters) {
                    @Nonnull
                    @Override
                    public ClassDef rewrite(@Nonnull ClassDef classDef) {
                        // 首先找到相应的类。
                        if (mClassDesc.equals(TypeDescriptionHelper.convertByClassDef(classDef))) {
                            Method clinit = ClassDefHelper.isclinitMethodExit(classDef);
                            Method newMethod;
                            if (null == clinit) {
                                newMethod = createClinit(classDef);
                            } else {
                                newMethod = genLoadLibrary(clinit);
                            }
                            return ClassDefHelper.addOrReplaceMethod(classDef, newMethod);
                        } else {
                            return super.rewrite(classDef);
                        }
                    }
                };
            }
        }).rewriteDexFile(mDexFile);
        DexFileFactory.writeDexFile(mSrc.getAbsolutePath(), newDexFile);
    }

    /**
     * 生成System.loadLibrary指令。
     *
     * @param method
     * @return
     */
    private Method genLoadLibrary(Method method) {
        List<Instruction> newInsts = new ArrayList<>();
        int regIndex = method.getImplementation().getRegisterCount();
        ImmutableStringReference str = new ImmutableStringReference(Common.SO_NAME);
        // const-string vX, SO_NAME
        ImmutableInstruction21c i0 = new ImmutableInstruction21c(Opcode.CONST_STRING, regIndex, str);
        newInsts.add(i0);

        List<MethodParameter> params = new ArrayList<>();
        params.add(new ImmutableMethodParameter("Ljava/lang/String;", null, null));
        ImmutableMethod m = new ImmutableMethod("Ljava/lang/System;", "loadLibrary", params, "V", AccessFlags.PUBLIC.getValue(), null, null);

        if (regIndex > 15) {
            ImmutableInstruction3rc i1 = new ImmutableInstruction3rc(Opcode.INVOKE_STATIC_RANGE, regIndex, 1, m);
            newInsts.add(i1);
        } else {
            ImmutableInstruction35c i1 = new ImmutableInstruction35c(Opcode.INVOKE_STATIC, 1, regIndex, 0, 0, 0, 0, m);
            newInsts.add(i1);
        }

        return MethodHelper.insertInstructionInStart(method, newInsts, 1);
    }

    private Method createClinit(ClassDef classDef) {
       // ImmutableMethod newMethod = new ImmutableMethod(classDef.getType(), )
        return null;
    }

}
