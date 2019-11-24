package com.trinity.decoder;

import android.graphics.SurfaceTexture;
import android.view.Surface;

public class SurfaceTextureBridge {
    private static SurfaceTexture texture;
    private static Surface surface;
    private static float[] matrix = new float[16];

    public static Surface getSurface(int name){
        texture = new SurfaceTexture(name);
        surface = new Surface(texture);
        HwDecodeBridge.setOutputSurface(surface);
        return surface;
    }

    public static void updateTexImage(){
        texture.updateTexImage();
    }

    public static float[] getTransformMatrix(){
        texture.getTransformMatrix(matrix);
        return matrix;
    }

    public static void release(){
        texture.release();
        surface.release();
    }
}
