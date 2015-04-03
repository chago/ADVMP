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
     *
     * @param file     file zip文件。
     * @param outPath   输出路径。
     * @param filename 要解压缩的文件名。
     */
    public static void unZipSingle(File file, File outPath, String filename) throws IOException {
        try (ZipFile zipFile = new ZipFile(file)) {
            ZipEntry entry = zipFile.getEntry(filename);//所解压的文件名
            try (InputStream input = zipFile.getInputStream(entry); OutputStream output = new FileOutputStream(outPath)) {
                int temp = 0;
                while ((temp = input.read()) != -1) {
                    output.write(temp);
                }
            }
        }
    }

    /**
     * 压缩单个文件
     */
    public static void doZipSingle(File zipFile, File file) throws IOException {
        try (InputStream input = new FileInputStream(file); ZipOutputStream zipOut = new ZipOutputStream(new FileOutputStream(zipFile));) {
            zipOut.putNextEntry(new ZipEntry(file.getName()));
            int temp = 0;
            while ((temp = input.read()) != -1) {
                zipOut.write(temp);
            }
        }
    }

}
