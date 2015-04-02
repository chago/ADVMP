package buwai.android.shell.separator.config;

import buwai.android.shell.base.TypeDescription;
import org.jf.dexlib2.AccessFlags;
import org.jf.dexlib2.Opcode;

import java.util.ArrayList;
import java.util.List;

/**
 * 配置。
 * 当白名单与黑名单中的配置项有冲突时，以黑名单为准。
 * Created by buwai on 2015/4/1.
 */
public class Config {

    public static final String CONF_FILE_SYSTEM_PROPERTY = "system-property.conf";
    public static final String CONF_FILE_USER_DEFAULT = "user-default.conf";

    public static final String CONF_WHITE_LIST = "whitelist";
    public static final String CONF_BLACK_LIST = "blacklist";

    public static final String CONF_NON_SUPPORTS = "nonsupport";

    public static final String CONF_OPCODES = "opcodes";
    public static final String CONF_CLASS_ACC_FLAGS = "classAccFlags";
    public static final String CONF_METHOD_ACC_FLAGS = "methodAccFlags";

    public static final String CONF_TRY = "try";

    /**
     * 白名单。
     */
    public List<TypeDescription> whiteList = new ArrayList<>();

    /**
     * 黑名单 - 代表禁止。
     */
    public List<TypeDescription> blackList = new ArrayList<>();

    /**
     * 不支持的类的访问标志。
     */
    public List<AccessFlags> nonsupportClassAccFlags = new ArrayList<>();

    /**
     * 不支持的方法的访问标志。
     */
    public List<AccessFlags> nonsupportMethodAccFlags = new ArrayList<>();

    /**
     * 不支持的opcode。
     */
    public List<Opcode> nonsupportOpcodes = new ArrayList<>();

    /**
     * 是否支持方法中有异常处理的情况。
     * true：支持。false：不支持。
     */
    public boolean isSupportTryCatch;

}
