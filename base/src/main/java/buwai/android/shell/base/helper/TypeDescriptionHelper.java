package buwai.android.shell.base.helper;

import buwai.android.dexlib2.helper.MethodHelper;
import buwai.android.shell.base.TypeDescription;
import org.jf.dexlib2.iface.ClassDef;
import org.jf.dexlib2.iface.Method;

import java.util.Iterator;
import java.util.List;

/**
 * Created by Neptunian on 2015/4/1.
 */
public class TypeDescriptionHelper {

    /**
     * 将class的完整路径转换为TypeDescription类型。
     * @param fullClassName class的完整路径。
     * @return 返回TypeDescription类型。
     */
    public static TypeDescription convertByFullClassName(String fullClassName) {
        TypeDescription td = new TypeDescription();
        int lastIndex = fullClassName.lastIndexOf('.');
        td.packageName = fullClassName.substring(0, lastIndex);
        td.className = fullClassName.substring(lastIndex + 1);
        return td;
    }

    /**
     * 提取ClassDef中的信息，生成TypeDescription对象。
     * @param classDef
     * @return
     */
    public static TypeDescription convertByClassDef (ClassDef classDef) {
        String type = classDef.getType();
        type = type.substring(1, type.length() - 1).replace('/', '.');
        return convertByFullClassName(type);
    }

    /**
     * 提取Method中的信息，生成TypeDescription对象。
     * @param method
     * @return
     */
    public static TypeDescription convertByMethod (Method method) {
        String type = method.getDefiningClass();
        type = type.substring(1, type.length() - 1).replace('/', '.');
        TypeDescription td = convertByFullClassName(type);
        td.methodName = method.getName();
        td.methodSig = MethodHelper.genMethodSig(method);
        return td;
    }

    /**
     *
     * @param blackList
     * @param typeDescription
     * @return
     */
    public static boolean isMatchBlackList (List<TypeDescription> blackList, TypeDescription typeDescription) {
        if (isMatchClassInBlackList(blackList, typeDescription) && isMatchMethodInBlackList(blackList, typeDescription)) {
            return true;
        } else {
            return false;
        }
    }

    public static boolean isMatchWhiteList (List<TypeDescription> whiteList, TypeDescription typeDescription) {
        if (isMatchClassInWhiteList(whiteList, typeDescription) && isMatchMethodInWhiteList(whiteList, typeDescription)) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * 判断类是否包含在白名单中。 如果是整个包，那么也判定这个类是包含的。 如果是类中有内部类，如果这个类包含，那么这个内部类也包含。
     *
     * @param list
     *            名单列表。
     * @param typeDescription
     * @return true: 包含。false: 不包含。
     */
    public static boolean isMatchClassInWhiteList(List<TypeDescription> list, TypeDescription typeDescription) {
        Iterator<TypeDescription> it = list.iterator();
        while (it.hasNext()) {
            TypeDescription td = it.next();

            String packageName = typeDescription.packageName, className = typeDescription.className;

            if (isMatchPackage(td.packageName, packageName)) {
                // 如果类名为null，则说明这个项的作用域为包。
                if (null == td.className) {
                    return true;
                } else {
                    int index_0 = -1;
                    if ("*".equals(td.className)) {
                        return true;
                    } else if ('*' == td.className.charAt(td.className.length() - 1)) {
                        if (0 == className.indexOf(td.className.substring(0, td.className.length() - 1))) {
                            return true;
                        }
                    } else if (td.className.equals(className)) {
                        return true;
                    } else if (-1 != (index_0 = className.indexOf('$'))) {
                        if (className.substring(0, index_0).equals(td.className)) {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    /**
     * 判断类是否包含在黑名单中。 如果是整个包，那么也判定这个类是包含的。 如果是类中有内部类，如果这个类包含，那么这个内部类也包含。
     *
     * @param list
     *            名单列表。
     * @param typeDescription
     * @return true: 包含。false: 不包含。
     */
    public static boolean isMatchClassInBlackList(List<TypeDescription> list, TypeDescription typeDescription) {
        Iterator<TypeDescription> it = list.iterator();
        while (it.hasNext()) {
            TypeDescription td = it.next();

            String packageName = typeDescription.packageName;
            String className = typeDescription.className;

            if (isMatchPackage(td.packageName, packageName)) {
                // 如果类名为null，则说明这个项的作用域为包。
                if (null == td.className) {
                    return true;
                } else {
                    int index_0 = -1;
                    if ("*".equals(td.className)) {
                        if (null == td.methodName)
                            return true;
                    } else if ('*' == td.className.charAt(td.className.length() - 1)) {
                        if (0 == className.indexOf(td.className.substring(0, td.className.length() - 1))) {
                            if (null == td.methodName)
                                return true;
                        }
                    } else if (td.className.equals(className)) {
                        if (null == td.methodName)
                            return true;
                    } else if (-1 != (index_0 = className.indexOf('$'))) {
                        if (className.substring(0, index_0).equals(td.className)) {
                            if (null == td.methodName)
                                return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    /**
     * 匹配包名。
     * @param packageNameInList
     * @param packageName
     * @return
     */
    private static boolean isMatchPackage(String packageNameInList, String packageName) {
        if (null == packageNameInList) {
            if ("".equals(packageName))
                return true;
            else
                return false;
        }
        // *代表通配符。
        if ('*' == packageNameInList.charAt(packageNameInList.length() - 1)) {
            if (0 != packageName.indexOf(packageNameInList.substring(0, packageNameInList.length() - 1))) {
                return false;
            } else {
                return true;
            }
        } else if (packageName.equals(packageNameInList)) {
            return true;
        } else if (0 == packageName.indexOf(packageNameInList)) {
            return true;
        }
        return false;
    }

    /**
     * 判断方法是否包含在黑白名单中。 如果方法名为null，则认为包含。 如果方法名相同但方法签名为null，则认为包含。
     *
     * @param list
     *            名单。
     * @param typeDescription
     *            方法引用。
     * @return true: 包含。false: 不包含。
     */
    public static boolean isMatchMethodInWhiteList(List<TypeDescription> list, TypeDescription typeDescription) {
        for (TypeDescription td : list) {
            if (null == td.methodName) {
                return true;
            } else if ('*' == td.methodName.charAt(td.methodName.length() - 1)) {
                if (0 == typeDescription.methodName.indexOf(td.methodName.substring(0, td.methodName.length() - 1))) {
                    return true;
                }
            } else if (td.methodName.equals(typeDescription.methodName)) {
                if (null == td.methodSig) {
                    return true;
                } else if (td.methodSig.equals(typeDescription.methodSig)) {
                    return true;
                }
            }
        }
        return false;
    }

    public static boolean isMatchMethodInBlackList(List<TypeDescription> list, TypeDescription typeDescription) {
        for (TypeDescription td : list) {
            if (null == td.methodName) {
                return false;
            } else if ('*' == td.methodName.charAt(td.methodName.length() - 1)) {
                if (0 == typeDescription.methodName.indexOf(td.methodName.substring(0, td.methodName.length() - 1))) {
                    return true;
                }
            } else if (td.methodName.equals(typeDescription.methodName)) {
                if (null == td.methodSig) {
                    return true;
                } else if (td.methodSig.equals(typeDescription.methodSig)) {
                    return true;
                }
            }
        }
        return false;
    }

}
