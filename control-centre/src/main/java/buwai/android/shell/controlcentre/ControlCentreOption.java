package buwai.android.shell.controlcentre;

import java.io.File;

/**
 * Created by Neptunian on 2015/4/3.
 */
public class ControlCentreOption {

    /**
     * apk原文件目录。
     */
    public File apkFile;

    /**
     * 输出的apk文件。
     */
    public File outApkFile;

    /**
     * 加固后apk的输出目录。
     */
    public File outDir;

    /**
     * 工作目录。
     */
    public File workspace;

    /**
     * 抽离器生成的yc文件。
     */
    public File outYcFile;

    /**
     * 抽离器生成的yc的cpp文件。
     */
    public File outYcCPFile;

    /**
     * 在工作目录中的jni文件存放目录。
     */
    public File jniDir;

    /**
     * lib目录。
     */
    public File libDir;

}
