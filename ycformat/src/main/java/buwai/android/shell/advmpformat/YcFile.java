package buwai.android.shell.advmpformat;

import java.io.*;
import java.util.List;

/**
 * Created by buwai on 2015/4/2.
 */
public class YcFile {

    private File mOutFile;
    private YcFormat mFormat;

    public YcFile(File outFile, YcFormat format) {
        mOutFile = outFile;
        mFormat = format;
    }

    /**
     * 写出yc文件。
     */
    public void write() throws IOException {
        try (BufferedOutputStream os = new BufferedOutputStream(new FileOutputStream(mOutFile))) {
            writeHeader(os);
            writeMethod(os);
            writeSeparatorData(os);
        }
    }

    /**
     * 写文件头。
     * @param os 输出流。
     * @throws IOException
     */
    private void writeHeader(OutputStream os) throws IOException {
        YcFormat.Header header = mFormat.header;
        os.write(header.magic.getBytes());
        os.write(header.methodOffset);
        os.write(header.separatorDataOffset);
    }

    /**
     * 向文件写入YcFormat.Method。
     * @param os 输出流。
     * @throws IOException
     */
    private void writeMethod(OutputStream os) throws IOException {
        List<YcFormat.Method> methods = mFormat.methods;
        if (null != methods) {
            for (YcFormat.Method m : methods) {
                os.write(m.methodIndex);
                os.write(m.definingClass.getBytes());
                os.write(m.name.getBytes());
                os.write(m.sig.getBytes());
            }
        }
        os.write(-1);   // -1代表段结尾。
    }

    /**
     * 写SeparatorData。
     * @param os 输出流。
     * @throws IOException
     */
    private void writeSeparatorData(OutputStream os) throws IOException {
        List<YcFormat.SeparatorData> separatorDatas = mFormat.separatorDatas;
        for (YcFormat.SeparatorData s : separatorDatas) {
            os.write(s.methodIndex);
            os.write(s.insts);
            // TODO ycformat 这里先不处理try...catch和调试信息。
        }
    }

}
