package buwai.android.shell.separator;

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

            SeparatorOption opt = new SeparatorOption();

            if (cl.hasOption('s')) {
                opt.dexFile = new File(cl.getOptionValue('s'));
            }
            if (null == opt.dexFile) {
                usage();
            }

            File outDir = null;
            if (cl.hasOption('o')) {
                outDir = new File(cl.getOptionValue('o'));
            }
            if (null == outDir) {
                usage();
            }
            opt.outDexFile = new File(outDir, "classes.dex");
            opt.outYcFile = new File(outDir, "classes.yc");
            opt.outCPFile = new File(outDir, "advmp_separator.cpp");

            if (cl.hasOption('c')) {
                opt.configFile = new File(cl.getOptionValue('c'));
            }

            //Utils.addLoadLibrary(srcFile, classDesc);

            log.info("------ 开始抽取 ------");
            Separator separator = new Separator(opt);
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
        help.printHelp("-s <dex文件路径> -o <输出目录> [-c <配置文件路径>]", options);
        System.exit(-1);
    }

}
