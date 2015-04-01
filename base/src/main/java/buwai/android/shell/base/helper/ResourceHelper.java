package buwai.android.shell.base.helper;

import java.io.*;
import java.nio.file.Files;

/**
 * Created by Neptunian on 2015/4/1.
 */
public class ResourceHelper {

    /**
     * 将资源拷贝到目标地址。
     * @param clazz Class对象。
     * @param resPath 资源在jar中的路径。如："resources/a.txt"。
     * @param outFile 输出文件路径。
     * @throws java.io.IOException
     */
    public static void copyResource(Class clazz, String resPath, File outFile) throws IOException {
        try (BufferedInputStream in = new BufferedInputStream(clazz.getClassLoader().getResourceAsStream(resPath))) {
            Files.copy(in, outFile.toPath());
        }
    }

    /**
     * 获得资源的文本内容。
     * @param clazz Class对象。
     * @param resPath 资源在jar中的路径。如："resources/a.txt"。
     * @throws IOException
     */
    public static String getResourceTxt(Class clazz, String resPath) throws IOException {
        StringBuffer sb = new StringBuffer();
        InputStream is = clazz.getClassLoader().getResourceAsStream(resPath);

        try (BufferedInputStream in = new BufferedInputStream(is)) {
            int byteRead = 0;
            byte[] buffer = new byte[1024];

            while ((byteRead = in.read(buffer)) != -1) {
                sb.append(new String(buffer, 0, byteRead));
            }
        }
        return sb.toString();
    }

}
