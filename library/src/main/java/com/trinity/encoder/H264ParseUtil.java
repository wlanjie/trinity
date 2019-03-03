package com.trinity.encoder;

/**
 * Created by xiaokai.zhan on 2018/9/12.
 */

public class H264ParseUtil {
    private  static byte bytesHeader[] = {0, 0, 0, 1};

    public static byte[] splitIDRFrame(byte[] h264Data) {
        byte[] result = h264Data;
        int offset = 0;
        do {
            offset = findAnnexbStartCodeIndex(h264Data, offset);
            int naluSpecIndex = offset;
            if(naluSpecIndex < h264Data.length) {
                int naluType = h264Data[naluSpecIndex] & 0x1F;
                if(naluType == 5) {
                    int idrFrameLength = h264Data.length - naluSpecIndex + 4;
                    result = new byte[idrFrameLength];
                    System.arraycopy(bytesHeader, 0, result, 0, 4);
                    System.arraycopy(h264Data, naluSpecIndex, result, 4, idrFrameLength - 4);
                    break;
                }
            }
        }while (offset < h264Data.length);
        return result;
    }

    public static int findAnnexbStartCodeIndex(byte[] data, int offset) {
        int index = offset;
        int cursor = offset;
        while (cursor < data.length) {
            int code = data[cursor++];
            if(is_h264_start_code(code) && cursor >= 3) {
                int firstPrefixCode = data[cursor - 3];
                int secondPrefixCode = data[cursor - 2];
                if(is_h264_start_code_prefix(firstPrefixCode) &&
                        is_h264_start_code_prefix(secondPrefixCode)){
                    break;
                }
            }
        }
        index = cursor;
        return index;
    }

    private static boolean is_h264_start_code(int code) {
        return (code == 0x01);
    }

    private static boolean is_h264_start_code_prefix(int code) {
        return (code == 0x00);
    }
}
