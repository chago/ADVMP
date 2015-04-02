package buwai.android.shell.advmpformat;

import org.jf.dexlib2.iface.TryBlock;
import org.jf.dexlib2.iface.debug.DebugItem;

import java.util.List;

/**
 * Created by buwai on 2015/4/2.
 */
public class YcFormat {

    public static final String MAGIC = "YC0000";

    public static final int SIZE_HEADER = MAGIC.length() + 4 + 4;

    /**
     * 文件头。
     */
    public YcFormat.Header header;

    /**
     *
     */
    public List<YcFormat.Method> methods;

    /**
     * 抽离器数据。
     */
    public List<YcFormat.SeparatorData> separatorDatas;

    public static class Header {

        /**
         * 魔术字。
         */
        public String magic;

        /**
         * Method数据距离文件起始的偏移。
         */
        public int methodOffset;

        /**
         * SeparatorData数据距离文件起始的偏移。
         */
        public int separatorDataOffset;

    }

    /**
     * 这个类中的字段对应dex文件中相应方法的信息。
     * 这些信息保证了在advmp中invoke方法时，可以
     * 通过jni函数找到该方法。
     */
    public static class Method {

        /**
         * 方法在dex中的索引。
         */
        public int methodIndex;

        /**
         * 方法所属类。
         * 以"Ljava/lang/System;"这样的格式保存。
         */
        public String definingClass;

        /**
         * 方法名。
         */
        public String name;

        /**
         * 方法签名。
         */
        public String sig;
    }

    /**
     * 抽离器数据。
     */
    public static class SeparatorData {

        /**
         * 这个索引表示当前SeparatorData结构在
         * SeparatorData结构数组中的索引。
         */
        public int methodIndex;

        /**
         * 抽取出来的方法指令。
         */
        public byte[] insts;

        /**
         * 方法的try...catch块。
         */
        public List<TryBlock> tryBlocks;

        /**
         * 方法的debug信息。
         */
        public List<DebugItem> debugItems;

    }

}
