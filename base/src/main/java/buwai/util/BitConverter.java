package buwai.util;

/**
 * Created by buwai on 2015/4/2.
 */
public class BitConverter {

    public static byte[] getBytes(int value) {
        byte[] result = new byte[4];

        result[0] = (byte) (value);
        result[1] = (byte) (value >> 8);
        result[2] = (byte) (value >> 16);
        result[3] = (byte) (value >> 24);

        return result;
    }

    public static byte[] getBytes(short value) {
        byte[] result = new byte[4];

        result[0] = (byte) (value);
        result[1] = (byte) (value >> 8);

        return result;
    }

    public static byte[] getBytes(short[] value) {
        byte[] result = new byte[value.length * 2];

        for (int i = 0, j = 0; i < result.length; j++) {
            byte[] tmp = getBytes(value[j]);
            result[i++] = tmp[0];
            result[i++] = tmp[1];
        }

        return result;
    }

}
