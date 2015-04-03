package buwai.android.shell.separator;

import buwai.android.dexlib2.helper.MethodHelper;
import buwai.android.shell.advmpformat.YcFile;
import buwai.android.shell.advmpformat.YcFormat;
import buwai.android.shell.base.Common;
import buwai.android.shell.separator.config.ConfigHelper;
import buwai.android.shell.separator.config.ConfigParse;
import org.jf.dexlib2.AccessFlags;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.dexbacked.DexBackedMethod;
import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.DexFile;
import org.jf.dexlib2.iface.Method;
import org.jf.dexlib2.immutable.ImmutableMethod;
import org.jf.dexlib2.rewriter.*;

import javax.annotation.Nonnull;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * 方法指令抽取器。
 * Created by buwai on 2015/4/1.
 */
public class Separator {
    private DexFile mDexFile;
    private ConfigHelper mConfigHelper;
    private DexRewriter mDexRewriter;
    private SeparatorOption mOpt;

    private List<Method> mSeparatedMethod = new ArrayList<>();
    private List<YcFormat.SeparatorData> mSeparatorData = new ArrayList<>();

    /**
     * @param opt
     */
    public Separator(SeparatorOption opt) throws IOException {
        mOpt = opt;
        mDexFile = DexFileFactory.loadDexFile(opt.apkFile, Common.API); // 加载dex。
        mDexRewriter = new SeparatorDexRewriter(new SeparatorRewriterModule());

        // 解析配置文件。
        mConfigHelper = new ConfigHelper(new ConfigParse(opt.configFile).parse());
    }

    /**
     * 抽取方法指令。
     *
     * @return true：成功。false：失败。
     */
    public boolean run() {
        boolean bRet = false;
        // 重新dex。
        DexFile newDexFile = mDexRewriter.rewriteDexFile(mDexFile);
        try {
            // 将新dex输出到文件。
            DexFileFactory.writeDexFile(mOpt.outDexFile.getAbsolutePath(), newDexFile);

            // 写Yc文件。
            writeYcFile();

            // 写C文件。
            writeCFile();

            bRet = true;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return bRet;
    }

    /**
     * 写Yc文件。
     *
     * @throws IOException
     */
    private void writeYcFile() throws IOException {
        YcFormat ycFormat = new YcFormat();
        // TODO 没有对methods字段赋值。
        ycFormat.separatorDatas = mSeparatorData;
        YcFile ycFile = new YcFile(mOpt.outYcFile, ycFormat);
        ycFile.write();
    }

    /**
     * 写C文件。
     */
    private void writeCFile() {

    }

    class SeparatorDexRewriter extends DexRewriter {

        public SeparatorDexRewriter(RewriterModule module) {
            super(module);
        }

        @Nonnull
        @Override
        public DexFile rewriteDexFile(@Nonnull DexFile dexFile) {
            return super.rewriteDexFile(dexFile);
        }
    }

    class SeparatorRewriterModule extends RewriterModule {
        @Nonnull
        @Override
        public Rewriter<ClassDef> getClassDefRewriter(@Nonnull Rewriters rewriters) {
            return new ClassDefRewriter(rewriters) {
                @Nonnull
                @Override
                public ClassDef rewrite(@Nonnull ClassDef classDef) {
                    return super.rewrite(classDef);
                }
            };
        }

        @Nonnull
        @Override
        public Rewriter<Method> getMethodRewriter(Rewriters rewriters) {
            return new MethodRewriter(rewriters) {
                @Nonnull
                @Override
                public Method rewrite(@Nonnull Method value) {
                    if (mConfigHelper.isValid(value)) {
                        mSeparatedMethod.add(value);
                        // 抽取代码。
                        YcFormat.SeparatorData separatorData = new YcFormat.SeparatorData();
                        separatorData.methodIndex = mSeparatorData.size();
                        separatorData.insts = MethodHelper.getInstructions((DexBackedMethod) value);
                        separatorData.instSize = separatorData.insts.length;
                        separatorData.size = 4 + 4 + separatorData.instSize + 4;
                        mSeparatorData.add(separatorData);

                        // 生成一个新的方法。
                        return new ImmutableMethod(value.getDefiningClass(), value.getName(), value.getParameters(), value.getReturnType(), value.getAccessFlags() | AccessFlags.NATIVE.getValue(), value.getAnnotations(), null);
                    }

                    return super.rewrite(value);
                }
            };
        }
    }

}
