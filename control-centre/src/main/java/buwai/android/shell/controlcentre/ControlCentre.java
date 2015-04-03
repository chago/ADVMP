package buwai.android.shell.controlcentre;

import buwai.android.shell.base.TypeDescription;
import buwai.android.shell.base.Utils;
import buwai.android.shell.base.ZipHelper;
import buwai.android.shell.base.helper.AndroidManifestHelper;
import buwai.android.shell.base.helper.CommandHelper;
import buwai.android.shell.separator.InstructionInsert01;
import buwai.android.shell.separator.Separator;
import buwai.android.shell.separator.SeparatorOption;
import org.apache.log4j.Logger;

import java.io.*;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;

/**
 * Created by Neptunian on 2015/4/1.
 */
public class ControlCentre {

    private final static Logger log = Logger.getLogger(ControlCentre.class);

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
        // 创建工作目录。
        mOpt.workspace = Files.createTempDirectory(mOpt.outDir.toPath(), "advmp").toFile();
        log.info("workspack:" + mOpt.workspace);

        mApkUnpackDir = new File(mOpt.workspace, "apk");
        mApkUnpackDir.mkdir();

        // 解压缩apk中的classes.dex文件。
        ZipHelper.unZipSingle(mOpt.apkFile, new File(mApkUnpackDir, "classes.dex"), "classes.dex");

        // 解压缩apk中的AndroidManifest.xml文件。
        ZipHelper.unZipSingle(mOpt.apkFile, new File(mApkUnpackDir, "AndroidManifest.xml"), "AndroidManifest.xml");
    }

    /**
     * 加壳。
     *
     * @return
     */
    public boolean shell() {
        boolean bRet = false;
        try {
            // 插入指令。
            TypeDescription classDesc = AndroidManifestHelper.findFirstClass(new File(mApkUnpackDir, "AndroidManifest.xml"));
            InstructionInsert01 instructionInsert01 = new InstructionInsert01(new File(mApkUnpackDir, "classes.dex"), classDesc);
            instructionInsert01.insert();

            // 运行抽离器。
            runSeparator();

            // 从template目录中拷贝jni文件。
            copyJniFiles();

            // 更新jni文件的内容。
            updateJniFiles();

            // 编译native代码。
            buildNative();

            // 将libs目录重命名为lib。
            mOpt.libDir = new File(mOpt.jniDir.getParentFile(), "lib");
            new File(mOpt.jniDir.getParentFile(), "libs").renameTo(mOpt.libDir);

            // 拷贝apk。
            mOpt.outApkFile = new File(mOpt.outDir, mOpt.apkFile.getName() + ".shelled.apk");
            Files.copy(mOpt.apkFile.toPath(), mOpt.outApkFile.toPath(), StandardCopyOption.REPLACE_EXISTING);

            File assetsDir = new File(mOpt.workspace, "assets");
            assetsDir.mkdir();
            File newYcFile = new File(assetsDir, "classes.yc");
            Files.move(mOpt.outYcFile.toPath(), newYcFile.toPath());

            // 将文件更新到apk中。
            ZipHelper.doZip(mOpt.libDir.getAbsolutePath(), mOpt.outApkFile.getAbsolutePath());  // lib
            ZipHelper.doZip(newYcFile.getAbsolutePath(), mOpt.outApkFile.getAbsolutePath());    // assets

            bRet = true;
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return bRet;
    }

    /**
     * 运行抽取器。
     *
     * @return
     * @throws IOException
     */
    private boolean runSeparator() throws IOException {
        SeparatorOption opt = new SeparatorOption();
        opt.dexFile = new File(mApkUnpackDir, "classes.dex");
        File outDir = new File(mOpt.workspace, "separator");
        opt.outDexFile = new File(outDir, "classes.dex");
        opt.outYcFile = mOpt.outYcFile = new File(outDir, "classes.yc");
        opt.outCPFile = mOpt.outYcCPFile = new File(outDir, "advmp_separator.cpp");

        Separator separator = new Separator(opt);
        return separator.run();
    }

    /**
     * 将template中的jni目录拷贝到工作目录。
     *
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
    private void updateJniFiles() throws IOException {
        File file;
        File tmpFile;
        StringBuffer sb = new StringBuffer();

        // 更新avmp.cpp文件中的内容。
        try (BufferedReader reader = new BufferedReader(new FileReader(mOpt.outYcCPFile))) {
            String line = null;
            while (null != (line = reader.readLine())) {
                sb.append(line);
                sb.append(System.getProperty("line.separator"));
            }
        }

        file = new File(mOpt.jniDir.getAbsolutePath() + File.separator + "advmpc" + File.separator + "avmp.cpp");
        tmpFile = new File(mOpt.jniDir.getAbsolutePath() + File.separator + "advmpc" + File.separator + "avmp.cpp" + ".tmp");
        try (BufferedReader reader = new BufferedReader(new FileReader(file));
             BufferedWriter writer = new BufferedWriter(new FileWriter(tmpFile))) {
            String line = null;
            while (null != (line = reader.readLine())) {
                if ("#ifdef _AVMP_DEBUG_".equals(line)) {
                    writer.write("#if 0");
                    writer.newLine();
                } else if ("//+${replaceAll}".equals(line)) {
                    writer.write(sb.toString());
                } else {
                    writer.write(line);
                    writer.newLine();
                }
            }
        }
        file.delete();
        tmpFile.renameTo(file);
        sb.delete(0, sb.length());
    }

    /**
     * 编译native代码。
     * @throws FileNotFoundException
     */
    private void buildNative() throws FileNotFoundException {
        File ndkDir = new File(System.getenv("ANDROID_NDK_HOME"));
        if (ndkDir.exists()) {
            String ndkPath = ndkDir.getAbsolutePath() + File.separator + "ndk-build";
            if (Utils.isWindowsOS()) {
                ndkPath += ".cmd";
            }
            log.info("------ 开始编译native代码 ------");
            // 编译native代码。
            CommandHelper.exec(new String[]{ndkPath, "NDK_PROJECT_PATH=" + mOpt.jniDir.getParent()});
            log.info("------ 编译结束 ------");
        } else {
            throw new FileNotFoundException("未能通过环境变量\"ANDROID_NDK_HOME\"找到ndk目录！这个环境变量可能未设置。");
        }
    }

}
