package buwai.android.shell.advmpformat;

import buwai.util.BitConverter;
import com.google.common.primitives.Ints;

import java.io.*;
import java.util.ArrayList;
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
        List<Byte> methods = writeMethod();
        List<Byte> separatorDatas = writeSeparatorData();


        // 创建文件头。
        mFormat.header = new YcFormat.Header();
        mFormat.header.magic = YcFormat.MAGIC;

        List<YcFormat.AdvmpMethod> ycformat_methods = mFormat.methods;
        if (null == ycformat_methods || 0 == ycformat_methods.size()) {
            mFormat.header.methodSize = 0;
            mFormat.header.methodOffset = 0;
        } else {
            mFormat.header.methodSize = ycformat_methods.size();
            mFormat.header.methodOffset = YcFormat.SIZE_HEADER;
        }

        mFormat.header.separatorDataSize = mFormat.separatorDatas.size();
        mFormat.header.separatorDataOffset = YcFormat.SIZE_HEADER + methods.size();

        // 最后再生成文件头。
        List<Byte> header = writeHeader();

        List<Byte> bytes = new ArrayList<>();
        bytes.addAll(header);
        bytes.addAll(methods);
        bytes.addAll(separatorDatas);

        byte[] arrBytes = new byte[bytes.size()];
        for (int i = 0; i < bytes.size(); i++) {
            arrBytes[i] = bytes.get(i);
        }
        try (BufferedOutputStream os = new BufferedOutputStream(new FileOutputStream(mOutFile))) {
            os.write(arrBytes);
        }
    }

    /**
     * 写文件头。
     * @throws IOException
     */
    private List<Byte> writeHeader() throws IOException {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        YcFormat.Header header = mFormat.header;


        byte[] xxx = Ints.toByteArray(10);

        os.write(header.magic.getBytes());
        os.write(BitConverter.getBytes(header.methodSize));
        os.write(BitConverter.getBytes(header.methodOffset));
        os.write(BitConverter.getBytes(header.separatorDataSize));
        os.write(BitConverter.getBytes(header.separatorDataOffset));

        List<Byte> bytes = new ArrayList<>();
        for (byte b : os.toByteArray()) {
            bytes.add(b);
        }
        return bytes;
    }

    /**
     * 向文件写入YcFormat.Method。
     * @throws IOException
     */
    private List<Byte> writeMethod() throws IOException {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        List<YcFormat.AdvmpMethod> methods = mFormat.methods;
        if (null != methods) {
            for (YcFormat.AdvmpMethod m : methods) {
                os.write(BitConverter.getBytes(m.methodIndex));
                os.write(m.definingClass.getBytes());
                os.write(m.name.getBytes());
                os.write(m.sig.getBytes());
            }
        }

        List<Byte> bytes = new ArrayList<>();
        for (byte b : os.toByteArray()) {
            bytes.add(b);
        }
        return bytes;
    }

    /**
     * 写SeparatorData。
     * @throws IOException
     */
    private List<Byte> writeSeparatorData() throws IOException {
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        List<YcFormat.SeparatorData> separatorDatas = mFormat.separatorDatas;
        for (YcFormat.SeparatorData s : separatorDatas) {
            os.write(BitConverter.getBytes(s.methodIndex));
            os.write(BitConverter.getBytes(s.instSize));
            os.write(BitConverter.getBytes(s.insts));
            // TODO ycformat 这里先不处理try...catch和调试信息。
        }

        List<Byte> bytes = new ArrayList<>();
        for (byte b : os.toByteArray()) {
            bytes.add(b);
        }
        return bytes;
    }

}
