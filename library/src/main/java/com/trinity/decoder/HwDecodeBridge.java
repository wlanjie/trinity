package com.trinity.decoder;

import android.media.MediaCodec;
import android.media.MediaFormat;
import android.view.Surface;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class HwDecodeBridge {
    private static MediaCodec codec = null;
    private static MediaFormat format = null;
    private static Surface outputSurface = null;
    public static void init(String codecName, int width, int height, ByteBuffer csd0, ByteBuffer csd1){
        try {
            codec = MediaCodec.createDecoderByType(codecName);
            format = new MediaFormat();
            format.setString(MediaFormat.KEY_MIME, codecName);
            format.setInteger(MediaFormat.KEY_WIDTH, width);
            format.setInteger(MediaFormat.KEY_HEIGHT, height);
            switch (codecName){
                case "video/avc":
                    if(csd0 != null && csd1 != null){
                        format.setByteBuffer("csd-0", csd0);
                        format.setByteBuffer("csd-1", csd1);
                    }
                    break;
                case "video/hevc":
                    if(csd0 != null){
                        format.setByteBuffer("csd-0", csd0);
                    }
                    break;
                case "video/mp4v-es":
                    format.setByteBuffer("csd-0", csd0);
                    break;
                case "video/3gpp":
                    format.setByteBuffer("csd-0", csd0);
                    break;
                default:
                    break;
            }
            codec.configure(format, outputSurface, null, 0);
            codec.start();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static void stop(){
        codec.stop();
    }

    public static void flush(){
        codec.flush();
    }

    public static int dequeueInputBuffer(long timeout){
        return codec.dequeueInputBuffer(timeout);
    }

    public static ByteBuffer getInputBuffer(int id){
//        return codec.getInputBuffer(id);
        return codec.getInputBuffers()[id];
    }
    public static void queueInputBuffer(int id, int size, long pts, int flags){
        codec.queueInputBuffer(id, 0, size, pts, flags);
    }

    private static ByteBuffer bf = ByteBuffer.allocateDirect(16);
    private static ByteBuffer bf2 = ByteBuffer.allocateDirect(12);
    static{
        bf.order(ByteOrder.BIG_ENDIAN);
        bf2.order(ByteOrder.BIG_ENDIAN);
    }
    private static MediaCodec.BufferInfo output_buffer_info = new MediaCodec.BufferInfo();
    public static ByteBuffer dequeueOutputBufferIndex(long timeout){
        int id = codec.dequeueOutputBuffer(output_buffer_info, timeout);
        bf.position(0);
        bf.putInt(id);
        if(id >= 0){
            bf.putInt(output_buffer_info.offset);
            bf.putLong(output_buffer_info.presentationTimeUs);
        }
        return bf;
    }

    public static void releaseOutPutBuffer(int id){
        try{
            codec.releaseOutputBuffer(id, true);
        }catch (Exception e){
            System.out.println("catch exception when releaseOutPutBuffer  id==>" + id);
            e.printStackTrace();
        }

    }

    public static ByteBuffer formatChange(){
        MediaFormat newFormat = codec.getOutputFormat();
        int width = newFormat.getInteger(MediaFormat.KEY_WIDTH);
        int height = newFormat.getInteger(MediaFormat.KEY_HEIGHT);
        int color_format = newFormat.getInteger(MediaFormat.KEY_COLOR_FORMAT);
        bf2.position(0);
        bf2.putInt(width);
        bf2.putInt(height);
        bf2.putInt(color_format);
        return bf2;
    }

    public static ByteBuffer getOutputBuffer(int id){
//        ByteBuffer ret = codec.getOutputBuffer(id);
        return codec.getOutputBuffers()[id];
    }

    public static void release(){
        codec = null;
        format = null;
    }

    public static void setOutputSurface(Surface sur){
        outputSurface = sur;
    }
}
