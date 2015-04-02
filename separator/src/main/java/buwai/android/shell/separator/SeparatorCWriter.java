package buwai.android.shell.separator;

import org.jf.dexlib2.iface.Method;

import java.util.List;

/**
 * Created by buwai on 2015/4/2.
 */
public class SeparatorCWriter {

    private List<Method> mSeparatedMethod;

    public SeparatorCWriter (List<Method> separatedMethod) {
        mSeparatedMethod = separatedMethod;
    }

    public void write() {
        // TODO separator 还未写到C文件中，先设计C文件进行解释执行的程序。
    }

}
