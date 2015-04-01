package buwai.android.shell.separator;

import buwai.android.shell.base.helper.AndroidManifestHelper;
import buwai.android.shell.base.TypeDescription;
import buwai.android.shell.base.Utils;
import org.apache.commons.cli.*;
import org.apache.log4j.Logger;

import java.io.File;
import java.io.IOException;

/**
 * Created by buwai on 2015/4/1.
 */
public class EntryPoint {

    private static final Logger log = Logger.getLogger(EntryPoint.class);

    private static Options options;

    static {
        options = new Options();
        options.addOption("h", false, "使用帮助");
        options.addOption("s", true, "dex文件路径");
        options.addOption("m", true, "AndroidManifest.xml文件路径");
        options.addOption("o", true, "输出目录");
        options.addOption("c", true, "配置文件");
    }

    public static void main(String[] args) {
        try {
            BasicParser parser = new BasicParser();
            CommandLine cl = null;
            cl = parser.parse(options, args);

            if (cl.hasOption('h')) {
                usage();
            }

            File srcFile = null;
            if (cl.hasOption('s')) {
                srcFile = new File(cl.getOptionValue('s'));
            }
            if (null == srcFile) {
                usage();
            }

            File manifestFile = null;
            if (cl.hasOption('m')) {
                manifestFile = new File(cl.getOptionValue('m'));
            }
            if (null == manifestFile) {
                usage();
            }

            File outDir = null;
            if (cl.hasOption('o')) {
                outDir = new File(cl.getOptionValue('o'));
            }
            if (null == outDir) {
                usage();
            }

            File configFile = null;
            if (cl.hasOption('c')) {
                configFile = new File(cl.getOptionValue('c'));
            }

            // [TODO] separator 这里是临时放置，以后要把下面两行语句放到control-centre中。
            TypeDescription classDesc = AndroidManifestHelper.findFirstClass(manifestFile);
            Utils.addLoadLibrary(srcFile, classDesc);

            log.info("------ 开始抽取 ------");
            Separator separator = new Separator(srcFile, manifestFile, outDir, configFile);
            if (separator.run()) {
                log.info("抽取成功。");
            } else {
                log.error("抽取失败。");
            }
            log.info("------ 抽取结束 ------");
        } catch (ParseException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * 用法。
     */
    private static void usage() {
        HelpFormatter help = new HelpFormatter();
        help.printHelp("-s <dex文件路径> -m <AndroidManifest.xml文件路径> -o <输出目录> [-c <配置文件路径>]", options);
        System.exit(-1);
    }

}
