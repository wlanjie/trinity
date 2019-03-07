package com.trinity.camera;

import android.hardware.Camera.CameraInfo;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import com.trinity.encoder.MediaCodecSurfaceEncoder;

public class PreviewScheduler
        implements TrinityCamera.CameraCallback, TrinityPreviewView.PreviewViewCallback {
    private TrinityPreviewView mPreviewView;
    private TrinityCamera mCamera;
    private boolean isFirst = true;
    private boolean isSurfaceExsist = false;
    private boolean isStopped;
    private int defaultCameraFacingId = CameraInfo.CAMERA_FACING_FRONT;
    private long mHandle;

    public PreviewScheduler(TrinityPreviewView previewView, TrinityCamera camera) {
        isStopped = false;
        this.mPreviewView = previewView;
        this.mCamera = camera;
        this.mPreviewView.setCallback(this);
        this.mCamera.setCallback(this);
    }

    public void resetStopState() {
        isStopped = false;
    }

    public int getNumberOfCameras() {
        if (null != mCamera) {
            return mCamera.getNumberOfCameras();
        }
        return -1;
    }

    /**
     * 切换摄像头, 底层会在返回来调用configCamera, 之后在启动预览
     **/
    public void switchCameraFacing() {
        switchCamera(mHandle);
    }

    private native void switchCamera(long handle);

    @Override
    public void createSurface(Surface surface, int width, int height) {
        startPreview(surface, width, height, defaultCameraFacingId);
    }

    private void startPreview(Surface surface, int width, int height, final int cameraFacingId) {
        if (isFirst) {
            mHandle = create();
            prepareEGLContext(mHandle, surface, width, height, cameraFacingId);
            isFirst = false;
        } else {
            createWindowSurface(mHandle, surface);
        }
        isSurfaceExsist = true;
    }

    public void startPreview(final int cameraFacingId) {
        try {
            if (null != mPreviewView) {
                SurfaceHolder holder = mPreviewView.getHolder();
                if (null != holder) {
                    Surface surface = holder.getSurface();
                    if (null != surface) {
                        startPreview(surface, mPreviewView.getWidth(), mPreviewView.getHeight(), cameraFacingId);
                    }
                }
            }
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    private native void prepareEGLContext(long handle, Surface surface, int width, int height, int cameraFacingId);

    private native void createWindowSurface(long handle, Surface surface);

    @Override
    public void resetRenderSize(int width, int height) {
        setRenderSize(mHandle, width, height);
    }

    private native void setRenderSize(long handle, int width, int height);

    @Override
    public void destroySurface() {
        if (isStopped) {
            this.stopPreview();
        } else {
            this.destroyWindowSurface(mHandle);
        }
        isSurfaceExsist = false;
    }

    public void stop() {
        this.stopEncoding(mHandle);
        isStopped = true;
        if (!isSurfaceExsist) {
            this.stopPreview();
        }
    }

    private void stopPreview() {
        this.destroyEGLContext(mHandle);
        release(mHandle);
        mHandle = 0;
        isFirst = true;
        isSurfaceExsist = false;
        isStopped = false;
    }

    private native long create();

    private native void destroyWindowSurface(long handle);

    private native void destroyEGLContext(long handle);

    @Override
    public void notifyFrameAvailable() {
        onFrameAvailable(mHandle);
    }

    private native void onFrameAvailable(long handle);

    @Override
    public void onPermissionDismiss(String tip) {
        Log.i("problem", "onPermissionDismiss : " + tip);
    }

    @Override
    public void updateTexMatrix(float texMatrix[]) {
        updateTextureMatrix(mHandle, texMatrix);
    }

    private native void updateTextureMatrix(long handle, float[] textureMatrix);

    public void startEncoding(int width, int height, int videoBitRate, int frameRate, boolean useHardWareEncoding, int strategy) {
        startEncode(mHandle, width, height, videoBitRate, frameRate, useHardWareEncoding, strategy);
    }

    private native void startEncode(long handle, int width, int height, int videoBitRate, int frameRate, boolean useHardWareEncoding, int strategy);

    public native void stopEncoding(long handle);

    private native void release(long handle);

    private CameraConfigInfo mConfigInfo;

    /**
     * 当底层创建好EGLContext之后，回调回来配置Camera，返回Camera的配置信息，然后在EGLThread线程中回调回来继续做Camera未完的配置以及Preview
     **/
    public CameraConfigInfo configCameraFromNative(int cameraFacingId) {
        defaultCameraFacingId = cameraFacingId;
        mConfigInfo = mCamera.configCameraFromNative(cameraFacingId);
        return mConfigInfo;
    }

    /**
     * 当底层EGLThread创建初纹理之后,设置给Camera
     **/
    public void startPreviewFromNative(int textureId) {
        mCamera.setCameraPreviewTexture(textureId);
    }

    /**
     * 当底层EGLThread更新纹理的时候调用这个方法
     **/
    public void updateTexImageFromNative() {
        mCamera.updateTexImage();
    }

    /**
     * 释放掉当前的Camera
     **/
    public void releaseCameraFromNative() {
        mCamera.releaseCamera();
    }

    // encoder_
    protected MediaCodecSurfaceEncoder surfaceEncoder;
    Surface surface = null;

    public void createMediaCodecSurfaceEncoderFromNative(int width, int height, int bitRate, int frameRate) {
        try {
            surfaceEncoder = new MediaCodecSurfaceEncoder(width, height, bitRate, frameRate);
            surface = surfaceEncoder.getInputSurface();
        } catch (Exception e) {
            Log.e("problem", "createMediaCodecSurfaceEncoder failed");
        }
    }

    public void hotConfigEncoderFromNative(int width, int height, int bitRate, int fps) {
        try {
            if (surfaceEncoder != null) {
                surfaceEncoder.hotConfig(width, height, bitRate, fps);
                surface = surfaceEncoder.getInputSurface();
            }
        } catch (Exception e) {
            Log.e("problem", "hotConfigMediaCodecSurfaceEncoder failed");
        }
    }

    public long pullH264StreamFromDrainEncoderFromNative(byte[] returnedData) {
        return surfaceEncoder.pullH264StreamFromDrainEncoderFromNative(returnedData);
    }

    public long getLastPresentationTimeUsFromNative() {
        return surfaceEncoder.getLastPresentationTimeUs();
    }

    public Surface getEncodeSurfaceFromNative() {
        return surface;
    }

    public void reConfigureFromNative(int targetBitrate) {
        if (null != surfaceEncoder) {
            surfaceEncoder.reConfigureFromNative(targetBitrate);
        }
    }

    public void closeMediaCodecCalledFromNative() {
        if (null != surfaceEncoder) {
            surfaceEncoder.shutdown();
        }
    }

    public native void hotConfig(int bitRate, int fps, int gopSize);

    public native void setBeautifyParam(int key, float value);
}
