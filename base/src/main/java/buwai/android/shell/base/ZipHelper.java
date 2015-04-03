package buwai.android.shell.base;

import java.io.*;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

/**
 * Created by buwai on 2015/4/1.
 */
public class ZipHelper {

    /**
     * 解压缩zip文件中的单个文件。
     *
     * @param file     file zip文件。
     * @param outPath  输出路径。
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

    /**
     * 压缩一个文件或文件夹到zip文件。
     *
     * @param inputFilePath
     * @param zipPath
     * @throws Exception
     */
    public static void doZip(String inputFilePath, String zipPath) throws Exception {
        doZip(zipPath, new File(inputFilePath));
    }

    private static void doZip(String zipPath, File inputFile) throws Exception {
        try (ZipOutputStream out = new ZipOutputStream(new FileOutputStream(zipPath))) {
            doZip(out, inputFile, "");
        }
    }

    private static void doZip(ZipOutputStream out, File inputFile, String base) throws Exception {

        byte[] buffer = new byte[1024];

        if (inputFile.isDirectory()) {
            File[] fl = inputFile.listFiles();
            if (base.length() > 0) {
                out.putNextEntry(new ZipEntry(base + "/"));
            }
            base = base.length() == 0 ? "" : base + "/";
            for (int i = 0; i < fl.length; i++) {
                doZip(out, fl[i], base + fl[i].getName());
            }
        } else {
            out.putNextEntry(new ZipEntry(base));
            try (FileInputStream in = new FileInputStream(inputFile)) {
                int len;
                while ((len = in.read(buffer)) > 0) {
                    out.write(buffer, 0, len);
                }
            }
        }
    }

    /**
     * Unzip it
     *
     * @param zipFile      input zip file
     * @param outputFolder zip file output folder
     */
    public static void unZip(String zipFile, String outputFolder) {

        byte[] buffer = new byte[1024];
        ZipInputStream zis = null;

        // create output directory is not exists
        File folder = new File(outputFolder);
        if (!folder.exists()) {
            folder.mkdir();
        }

        try {


            // get the zip file content
            zis = new ZipInputStream(new FileInputStream(zipFile));
            // get the zipped file list entry
            ZipEntry ze = zis.getNextEntry();

            while (ze != null) {

                String fileName = ze.getName();
                File newFile = new File(outputFolder + File.separator + fileName);

                if (ze.isDirectory()) {
                    ze = zis.getNextEntry();
                    continue;
                }
                // create all non exists folders
                // else you will hit FileNotFoundException for compressed folder
                new File(newFile.getParent()).mkdirs();

                try (FileOutputStream fos = new FileOutputStream(newFile)) {

                    int len;
                    while ((len = zis.read(buffer)) > 0) {
                        fos.write(buffer, 0, len);
                    }

                    fos.close();
                }
                ze = zis.getNextEntry();
            }

        } catch (IOException ex) {
            ex.printStackTrace();
        } finally {
            if (null != zis) {
                try {
                    zis.closeEntry();
                } catch (IOException e1) {
                    e1.printStackTrace();
                }
                try {
                    zis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
    }

}
