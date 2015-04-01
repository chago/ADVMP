package buwai.android.shell.controlcentre;

import org.apache.commons.cli.*;

import java.io.File;

/**
 * Created by buwai on 2015/4/1.
 */
public class EntryPoint {

    private static Options options;

    static {
        options = new Options();
        options.addOption("h", false, "使用帮助");
        options.addOption("s", true, "apk文件路径。");
        options.addOption("o", true, "输出目录");
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

            File outDir = null;
            if (cl.hasOption('o')) {
                outDir = new File(cl.getOptionValue('o'));
            }
            if (null == outDir) {
                usage();
            }

        } catch (ParseException e) {
            e.printStackTrace();
        }
    }

    private static void usage() {
        HelpFormatter help = new HelpFormatter();
        help.printHelp("-s <dex文件路径> -o <输出目录> [-c <配置文件路径>]", options);
        System.exit(-1);
    }

}
