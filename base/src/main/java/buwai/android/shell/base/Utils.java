package buwai.android.shell.base;

import buwai.android.shell.base.helper.TypeDescriptionHelper;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.DexFile;
import org.jf.dexlib2.iface.Method;
import org.jf.dexlib2.immutable.ImmutableClassDef;
import org.jf.dexlib2.rewriter.*;

import javax.annotation.Nonnull;
import java.io.File;
import java.io.IOException;

/**
 * Created by buwai on 25/4/1.
 */
public class Utils {

    /**
     * 向类的"<clinit>"中添加System.loadLibrary语句。
     * @param src dex文件。
     * @param classDesc 要添加System.loadLibrary语句的类。
     * @throws IOException
     */
    public static void addLoadLibrary(File src, final TypeDescription classDesc) throws IOException {
        DexFile dexFile = DexFileFactory.loadDexFile(src, Common.API); // 加载dex。
        DexFile newDexFile = new DexRewriter(new RewriterModule() {
            @Nonnull
            @Override
            public Rewriter<ClassDef> getClassDefRewriter(Rewriters rewriters) {
                return new ClassDefRewriter(rewriters) {
                    @Nonnull
                    @Override
                    public ClassDef rewrite(@Nonnull ClassDef classDef) {
                        if (classDesc.equals(TypeDescriptionHelper.convertByClassDef(classDef))) {
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

}
