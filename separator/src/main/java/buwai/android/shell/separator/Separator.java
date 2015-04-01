package buwai.android.shell.separator;

import buwai.android.shell.base.Common;
import buwai.android.shell.separator.config.ConfigHelper;
import buwai.android.shell.separator.config.ConfigParse;
import org.jf.dexlib2.DexFileFactory;
import org.jf.dexlib2.iface.DexFile;
import org.jf.dexlib2.iface.Method;
import org.jf.dexlib2.rewriter.*;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;
import java.io.File;
import java.io.IOException;

/**
 * 方法指令抽取器。
 * Created by buwai on 2015/4/1.
 */
public class Separator {
    private DexFile mDexFile;
    private File mOutDir;
    private ConfigHelper mConfigHelper;
    private DexRewriter mDexRewriter;

    /**
     * @param src    dex文件路径。
     * @param outDir 输出目录
     * @param config 配置文件路径。如果传入null，则解析jar包中保存的默认配置文件。
     */
    public Separator(File src, File manifestFile, File outDir, @Nullable File config) throws IOException {
        mDexFile = DexFileFactory.loadDexFile(src, Common.API); // 加载dex。
        mDexRewriter = new SeparatorDexRewriter(new SeparatorRewriterModule());

        mOutDir = outDir;

        // 解析配置文件。
        mConfigHelper = new ConfigHelper(new ConfigParse(config).parse());
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
            DexFileFactory.writeDexFile(mOutDir.getAbsolutePath() + File.separator + "classes.dex", newDexFile);

            bRet = true;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return bRet;
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
        public Rewriter<Method> getMethodRewriter(Rewriters rewriters) {
            return new MethodRewriter(rewriters) {
                @Nonnull
                @Override
                public Method rewrite(@Nonnull Method value) {
                    return super.rewrite(value);
                }
            };
        }
    }

}
