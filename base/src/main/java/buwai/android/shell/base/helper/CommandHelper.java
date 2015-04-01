package buwai.android.shell.base.helper;

import org.apache.log4j.Logger;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

/**
 * Created by buwai on 2015/4/1.
 */
public class CommandHelper {

    private static final Logger log = Logger.getLogger(CommandHelper.class);

    /**
     * 运行命令行。同步log。
     * @param args
     * @return
     */
    public static int run(String[] args) {
        StringBuilder cmd = new StringBuilder();
        for (String arg : args) {
            cmd.append(arg);
            cmd.append(" ");
        }
        return run(cmd.toString());
    }

    /**
     * 运行命令行。
     * @param cmd
     * @return
     */
    public static int run(String cmd) {
        Process ps = null;
        int exitCode = -1;
        try {
            ps = Runtime.getRuntime().exec(cmd);
            exitCode = ps.waitFor();
            try (BufferedReader br = new BufferedReader(new InputStreamReader(ps.getInputStream()))){
                StringBuffer sb = new StringBuffer();
                String line;
                while ((line = br.readLine()) != null) {
                    sb.append(line).append("\n");
                }
                String result = sb.toString();
                log.info(result);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        return exitCode;
    }

}
