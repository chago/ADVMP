package buwai.android.shell.base;

import java.io.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

/**
 * Created by buwai on 2015/4/1.
 */
public class ZipHelper {

    /**
     * 解压缩zip文件中的单个文件。
     * @param file file zip文件。
     * @param outDir 被解压缩文件的输出目录。
     * @param filename 要解压缩的文件名。
     */
    public static void unZipSingle(File file, File outDir, String filename) {
        try {
            ZipFile zipFile = new ZipFile(file);
            ZipEntry entry = zipFile.getEntry(filename);//所解压的文件名
            InputStream input = zipFile.getInputStream(entry);
            OutputStream output = new FileOutputStream(outDir);
            int temp = 0;
            while ((temp = input.read()) != -1) {
                output.write(temp);
            }
            input.close();
            output.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * 压缩单个文件
     */
    public static void doZipSingle(File zipFile, File file) {
        try {
            InputStream input = new FileInputStream(file);
            ZipOutputStream zipOut = new ZipOutputStream(new FileOutputStream(zipFile));
            zipOut.putNextEntry(new ZipEntry(file.getName()));
            int temp = 0;
            while ((temp = input.read()) != -1) {
                zipOut.write(temp);
            }
            input.close();
            zipOut.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
