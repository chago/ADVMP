package buwai.android.dexlib2.helper;

import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.Method;
import org.jf.dexlib2.immutable.ImmutableClassDef;

import java.util.ArrayList;
import java.util.List;

/**
 * 增强对ClassDef类的操作。
 * Created by buwai on 2015/4/1.
 */
public class ClassDefHelper {

    /**
     * 判断类中是否存在相应的方法。
     * 判断：
     * 类定义、方法名、返回类型、参数。
     * @param classDef 类定义。
     * @param method 方法。
     * @return true：存在。false：不存在。
     */
    public static boolean isMethodExist(ClassDef classDef, Method method) {
        for (Method m : classDef.getMethods()) {
            // 判断类名、方法名。
            if (m.getDefiningClass().equals(method.getDefiningClass()) &&
                m.getName().equals(method.getName()) &&
                m.getReturnType().equals(method.getReturnType()) &&
                (m.getParameters().size() == method.getParameters().size())) {
                int size = m.getParameters().size();
                boolean bRet = true;
                for (int i = 0; i < size; i++) {
                    if (!m.getParameters().get(i).equals(method.getParameters().get(i))) {
                        bRet = false;
                        break;
                    }
                }
                return bRet;
            }
        }
        return false;
    }

    /**
     * 判断<clinit>方法是否存在。
     * @param classDef 类定义对象。
     * @return 如果存在，则返回<clinit>的方法对象。如果不存在，则返回null。
     */
    public static Method isclinitMethodExit(ClassDef classDef) {
        for (Method m : classDef.getMethods()) {
            if ("<clinit>".equals(m.getName())) {
                return m;
            }
        }
        return null;
    }

    /**
     * 向ClassDef中添加一个新方法。
     * @param classDef ClassDef对象。
     * @param method 要插入的方法。
     * @return 返回新的类定义。
     */
    public static ClassDef addMethod(ClassDef classDef, Method method) {
        List<Method> newMethods = new ArrayList<>();
        newMethods.add(method);
        for (Method m : classDef.getMethods()) {
            newMethods.add(m);
        }
        return new ImmutableClassDef(classDef.getType(), classDef.getAccessFlags(), classDef.getSuperclass(), classDef.getInterfaces(), classDef.getSourceFile(), classDef.getAnnotations(), classDef.getFields(), newMethods);
    }

    /**
     * 如果类中不存在相应的方法，则添加。如果存在，则替换。
     * @param classDef 类定义。
     * @param method 方法。
     * @return 返回新的类定义。
     */
    public static ClassDef addOrReplaceMethod(ClassDef classDef, Method method) {
        if (!isMethodExist(classDef, method)) {
            return addMethod(classDef, method);
        }

        List<Method> newMethods = new ArrayList<>();
        for (Method m : classDef.getMethods()) {
            if (m.getDefiningClass().equals(method.getDefiningClass()) &&
                    m.getName().equals(method.getName()) &&
                    m.getReturnType().equals(method.getReturnType()) &&
                    (m.getParameters().size() == method.getParameters().size())) {
                int size = m.getParameters().size();
                boolean bRet = true;
                for (int i = 0; i < size; i++) {
                    if (!m.getParameters().get(i).getType().equals(method.getParameters().get(i).getType())) {
                        bRet = false;
                        break;
                    }
                }
                if (bRet) {
                    newMethods.add(method);
                } else {
                    newMethods.add(m);
                }
            } else {
                newMethods.add(m);
            }
        }
        return new ImmutableClassDef(classDef.getType(), classDef.getAccessFlags(), classDef.getSuperclass(), classDef.getInterfaces(), classDef.getSourceFile(), classDef.getAnnotations(), classDef.getFields(), newMethods);
    }

    /**
     * 替换方法，不做这个方法是否已经存在的判断。
     * 所以如果传入的方法不存在，那么造成的后果是未知的。
     * @param classDef 类定义。
     * @param method 要替换的方法。
     * @return 返回新的类定义。
     */
    public static ClassDef replaceMethod(ClassDef classDef, Method method) {
        List<Method> newMethods = new ArrayList<>();
        for (Method m : classDef.getMethods()) {
            if (m.getDefiningClass().equals(method.getDefiningClass()) &&
                    m.getName().equals(method.getName()) &&
                    m.getReturnType().equals(method.getReturnType()) &&
                    (m.getParameters().size() == method.getParameters().size())) {
                int size = m.getParameters().size();
                boolean bRet = true;
                for (int i = 0; i < size; i++) {
                    if (!m.getParameters().get(i).getType().equals(method.getParameters().get(i).getType())) {
                        bRet = false;
                        break;
                    }
                }
                if (bRet) {
                    newMethods.add(method);
                } else {
                    newMethods.add(m);
                }
            } else {
                newMethods.add(m);
            }
        }
        return new ImmutableClassDef(classDef.getType(), classDef.getAccessFlags(), classDef.getSuperclass(), classDef.getInterfaces(), classDef.getSourceFile(), classDef.getAnnotations(), classDef.getFields(), newMethods);
    }

}
