package buwai.android.shell.controlcentre;

import buwai.android.shell.base.TypeDescription;
import buwai.android.shell.base.Utils;
import buwai.android.shell.base.ZipHelper;
import buwai.android.shell.base.helper.AndroidManifestHelper;
import buwai.android.shell.separator.InstructionInsert01;
import buwai.android.shell.separator.Separator;
import buwai.android.shell.separator.SeparatorOption;

import java.io.File;
import java.io.IOException;

/**
 * Created by Neptunian on 2015/4/1.
 */
public class ControlCentre {

    private ControlCentreOption mOpt;
    private File mApkUnpackDir;

    public ControlCentre(ControlCentreOption opt) throws IOException {
        mOpt = opt;
        prepare();
    }

    /**
     * 做一些准备工作。
     */
    private void prepare() throws IOException {
        mApkUnpackDir = new File(mOpt.workspace, "apk");
        mApkUnpackDir.mkdir();

        // 解压缩apk中的classes.dex文件。
        ZipHelper.unZipSingle(mOpt.apkFile, new File(mApkUnpackDir, "classes.dex"), "classes.dex");

        // 解压缩apk中的AndroidManifest.xml文件。
        ZipHelper.unZipSingle(mOpt.apkFile, new File(mApkUnpackDir, "AndroidManifest.xml"), "AndroidManifest.xml");
    }

    /**
     * 加壳。
     * @return
     */
    public boolean shell() {
        boolean bRet = false;
        try {
            // 插入指令。
            TypeDescription classDesc = AndroidManifestHelper.findFirstClass(new File(mApkUnpackDir, "AndroidManifest.xml"));
            InstructionInsert01 instructionInsert01 = new InstructionInsert01(new File(mApkUnpackDir, "classes.dex"), classDesc);
            instructionInsert01.insert();

            runSeparator();

            copyJniFiles();



            bRet = true;
        } catch (IOException e) {
            e.printStackTrace();
        }
        return bRet;
    }

    /**
     * 运行抽取器。
     * @return
     * @throws IOException
     */
    private boolean runSeparator() throws IOException {
        SeparatorOption opt = new SeparatorOption();
        opt.dexFile = new File(mApkUnpackDir, "classes.dex");
        File outDir = new File(mOpt.workspace, "separator");
        opt.outDexFile = new File(outDir, "classes.dex");
        opt.outYcFile = new File(outDir, "classes.yc");
        opt.outCPFile = new File(outDir, "advmp_separator.cpp");

        Separator separator = new Separator(opt);
        return separator.run();
    }

    /**
     * 将template中的jni目录拷贝到工作目录。
     * @throws IOException
     */
    private void copyJniFiles() throws IOException {
        // TODO 这里写死了。
        File jniTemplateDir = new File("E:\\MyProjects\\ADVMP\\template\\jni");
        mOpt.jniDir = new File(mOpt.workspace, "jni");
        Utils.copyFolder(jniTemplateDir.getAbsolutePath(), mOpt.jniDir.getAbsolutePath());
    }

    /**
     * 更新jni目录中的文件。
     */
    private void updateJniFiles() {
        File file = new File(mOpt.jniDir.getAbsolutePath() + File.separator + )
    }

}
