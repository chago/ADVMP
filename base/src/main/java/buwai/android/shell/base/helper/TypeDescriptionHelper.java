package buwai.android.shell.base.helper;

import buwai.android.shell.base.TypeDescription;
import org.jf.dexlib2.iface.ClassDef;

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

    public static TypeDescription convertByClassDef (ClassDef classDef) {
        String type = classDef.getType();
        type = type.substring(1, type.length() - 1).replace('/', '.');
        return convertByFullClassName(type);
    }

}
