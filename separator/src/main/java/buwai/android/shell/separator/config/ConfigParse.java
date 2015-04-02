package buwai.android.shell.separator.config;

import buwai.android.shell.base.Common;
import buwai.android.shell.base.TypeDescription;
import buwai.android.shell.base.helper.ResourceHelper;
import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.JSONArray;
import com.alibaba.fastjson.JSONObject;
import com.alibaba.fastjson.TypeReference;
import org.apache.log4j.Logger;
import org.jf.dexlib2.AccessFlags;
import org.jf.dexlib2.Opcodes;

import javax.annotation.Nullable;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.Map;

/**
 * Created by buwai on 2015/4/1.
 */
public class ConfigParse {

    private final static Logger log = Logger.getLogger(ConfigParse.class);

    private File mConfigFile;

    private Opcodes mOpcodes;

    /**
     * 构造方法。
     * @param configFile 配置文件。如果传入null，则解析jar包中保存的默认配置文件。
     */
    public ConfigParse(@Nullable File configFile) {
        mConfigFile = configFile;
        mOpcodes = new Opcodes(Common.API);
    }

    /**
     * 解析配置文件。
     */
    public Config parse() {
        Config config = null;
        try {
            config = new Config();
            // 解析system-property.conf文件内容。
            parseSystemProperty(config);

            // 解析用户的配置文件。
            parseUserConfig(config);

        } catch (IOException e) {
            config = null;
            e.printStackTrace();
        }
        return config;
    }

    /**
     * 解析system-property.conf文件内容。
     * @param config 解析出来的数据对config赋值。
     */
    private void parseSystemProperty(Config config) throws IOException {
        // 先读取系统的配置属性。
        String systemProperty = ResourceHelper.getResourceTxt(this.getClass(), Config.CONF_FILE_SYSTEM_PROPERTY);
        Map<String, Object> sysConf = JSON.parseObject(systemProperty, new TypeReference<Map<String, Object>>() {
        });
        // 解析系统配置文件中的黑名单。
        JSONArray blacklist = (JSONArray) sysConf.get(Config.CONF_BLACK_LIST);
        for (int i = 0; i < blacklist.size(); i++) {
            String item = blacklist.getString(i);
            TypeDescription td = parseTypeDescription(item);
            if (!config.blackList.contains(td)) {
                config.blackList.add(td);
            }
        }

        JSONObject no_supports = (JSONObject) sysConf.get(Config.CONF_NON_SUPPORTS);
        // 解析不支持的类的访问标志。
        JSONArray no_supports_class_acc_flags = (JSONArray) no_supports.get(Config.CONF_CLASS_ACC_FLAGS);
        for (int i = 0; i < no_supports_class_acc_flags.size(); i++) {
            String item = no_supports_class_acc_flags.getString(i);
            if (!config.nonsupportClassAccFlags.contains(item)) {
                config.nonsupportClassAccFlags.add(AccessFlags.getAccessFlag(item));
            }
        }
        // 解析不支持的方法的访问标志。
        JSONArray no_supports_method_acc_flags = (JSONArray) no_supports.get(Config.CONF_METHOD_ACC_FLAGS);
        for (int i = 0; i < no_supports_method_acc_flags.size(); i++) {
            String item = no_supports_method_acc_flags.getString(i);
            if (!config.nonsupportMethodAccFlags.contains(item)) {
                config.nonsupportMethodAccFlags.add(AccessFlags.getAccessFlag(item));
            }
        }
        // 解析不支持的Opcode。
        JSONArray no_supports_opcodes = (JSONArray) no_supports.get(Config.CONF_OPCODES);
        for (int i = 0; i < no_supports_opcodes.size(); i++) {
            String item = no_supports_opcodes.getString(i);
            if (!config.nonsupportOpcodes.contains(item)) {
                config.nonsupportOpcodes.add(mOpcodes.getOpcodeByName(item));
            }
        }

        // 是否支持方法中有try...catch块。
        String tryValue = no_supports.getString(Config.CONF_TRY);
        if (tryValue.equals("yes")) {
            config.isSupportTryCatch = true;
        } else {
            config.isSupportTryCatch = false;
        }
    }

    /**
     * 解析用户的配置文件。
     * @param config 解析出来的数据对config赋值。
     */
    private void parseUserConfig(Config config) throws IOException {
        String userProperty = null;
        // 然后读取用户的配置文件。
        if (null == mConfigFile) {   // 如果userConfigFile传入null，则使用默认的配置文件。
            userProperty = ResourceHelper.getResourceTxt(this.getClass(), Config.CONF_FILE_USER_DEFAULT);
        } else {
            userProperty = new String(Files.readAllBytes(mConfigFile.toPath()));
        }

        Map<String, Object> userConf = JSON.parseObject(userProperty, new TypeReference<Map<String, Object>>() {
        });

        // 解析黑名单。
        JSONArray blackList = (JSONArray) userConf.get(Config.CONF_BLACK_LIST);
        if (null != blackList) {
            for (int i = 0; i < blackList.size(); i++) {
                String item = blackList.getString(i);
                TypeDescription td = parseTypeDescription(item);
                if (!config.blackList.contains(td)) {
                    config.blackList.add(td);
                }
            }
        }

        // 解析白名单。
        JSONArray whiteList = (JSONArray) userConf.get(Config.CONF_WHITE_LIST);
        if (null != whiteList) {
            for (int i = 0; i < whiteList.size(); i++) {
                String item = whiteList.getString(i);
                TypeDescription td = parseTypeDescription(item);
                if (!config.whiteList.contains(td)) {
                    config.whiteList.add(td);
                }
            }
        }
    }

    /**
     * 解析类型描述。
     * @param item 黑白名单中的一项。
     * @return 返回类型描述对象。
     */
    private TypeDescription parseTypeDescription(String item) {
        String[] items = item.split("#");
        int len = items.length;
        TypeDescription td = new TypeDescription();
        if (3 == len) {
            String methodInfo = items[2];
            int methodSigStart = methodInfo.indexOf('(');
            if (-1 != methodSigStart) {
                int methodSigEnd = methodInfo.indexOf(')');
                if (-1 == methodSigEnd) {
                    log.error("黑白名单中的项写的有问题，方法签名写入错误：" + methodInfo);
                }
                td.methodName = methodInfo.substring(0, methodSigStart);
                td.methodSig = methodInfo.substring(methodSigStart);
            } else {
                td.methodName = methodInfo;
            }
        }
        if (len >= 2) {
            td.className = items[1];
        }
        td.packageName = items[0];
        return td;
    }
}
