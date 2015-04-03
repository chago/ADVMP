package buwai.android.shell.controlcentre;

import org.apache.commons.cli.*;
import org.apache.log4j.Logger;

import java.io.File;
import java.io.IOException;

/**
 * Created by buwai on 2015/4/1.
 */
public class EntryPoint {

    private final static Logger log = Logger.getLogger(EntryPoint.class);

    private static Options options;

    static {
        options = new Options();
        options.addOption("h", false, "使用帮助");
        options.addOption("s", true, "apk文件路径。");
        options.addOption("o", true, "输出目录");
    }

    public static void main(String[] args) {
        log.info("------ 进入控制中心 ------");
        try {
            BasicParser parser = new BasicParser();
            CommandLine cl = null;
            cl = parser.parse(options, args);

            if (cl.hasOption('h')) {
                usage();
            }

            ControlCentreOption opt = new ControlCentreOption();
            if (cl.hasOption('s')) {
                opt.apkFile = new File(cl.getOptionValue('s'));
            }
            if (null == opt.apkFile) {
                usage();
            }

            if (cl.hasOption('o')) {
                opt.outDir = new File(cl.getOptionValue('o'));
            }
            if (null == opt.outDir) {
                usage();
            }

            ControlCentre controlCentre = new ControlCentre(opt);
            log.info("开始加固。");
            if (controlCentre.shell()) {
                //log.info
            }

        } catch (ParseException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        log.info("------ 离开控制中心 ------");
    }

    private static void usage() {
        HelpFormatter help = new HelpFormatter();
        help.printHelp("-s <dex文件路径> -o <输出目录> [-c <配置文件路径>]", options);
        System.exit(-1);
    }

}
