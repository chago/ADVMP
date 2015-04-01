package buwai.android.shell.controlcentre;

import buwai.android.shell.base.ZipHelper;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

/**
 * Created by Neptunian on 2015/4/1.
 */
public class ControlCentre {

    private File mApkFile;
    private File mOutDir;
    private File mWorkspace;

    public ControlCentre(File src, File outDir) throws IOException {
        mApkFile = src;
        mOutDir = outDir;
        prepare();
    }

    /**
     * 做一些准备工作。
     */
    private void prepare() throws IOException {
        // 创建工作目录。
        mWorkspace = Files.createTempDirectory(mOutDir.toPath(), "advmp").toFile();

        // 解压缩apk中的classes.dex文件。
        ZipHelper.unZipSingle(mApkFile, mWorkspace, "classes.dex");
    }

    /**
     * 加壳。
     * @return
     */
//    public boolean shell() {
//
//    }

}
