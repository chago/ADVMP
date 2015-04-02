package buwai.android.shell.base;

/**
 * 类型描述。
 * Created by buwai on 2015/4/1.
 * &：忽略。
 * *：通配符。
 */
public class TypeDescription {

    public String packageName;

    public String className;

    public String methodName;

    public String methodSig;


    @Override
    public boolean equals(Object obj) {
        TypeDescription other = (TypeDescription) obj;

        // 包名必须不能为null。
        if (!packageName.equals(other.packageName)) {
            return false;
        }

        // 判断类名。
        if (null == className && null == other.className) {
            return true;
        } else if (null == className || null == other.className) {
            return false;
        } else if (!className.equals(other.className)) {
            return  false;
        }

        // 判断方法名。
        if (null == methodName && null == other.methodName) {
            return true;
        } else if (null == methodName || null == other.methodName) {
            return false;
        } else if (!methodName.equals(other.methodName)) {
            return  false;
        }

        // 判断方法签名。
        if (null == methodSig && null == other.methodSig) {
            return true;
        } else if (null == methodSig || null == other.methodSig) {
            return false;
        } else if (!methodSig.equals(other.methodSig)) {
            return  false;
        }

        return true;
    }
}
