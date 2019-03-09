package com.trinity.camera;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Parameters;
import android.hardware.Camera.Size;
import android.os.Build;
import android.util.Log;
import android.view.Surface;

import java.lang.reflect.Field;
import java.util.List;

public class TrinityCamera {
	private static final String TAG = "TrinityCamera";

	public static int VIDEO_WIDTH = 640;
	public static int DEFAULT_VIDEO_WIDTH = 640;
	public static int VIDEO_HEIGHT = 480;
	public static int DEFAULT_VIDEO_HEIGHT = 480;
	public static int videoFrameRate = 24;

	public static void forcePreviewSize_640_480() {
		VIDEO_WIDTH = 640;
		VIDEO_HEIGHT = 480;
		videoFrameRate = 15;
	}
	public static void forcePreviewSize_1280_720() {
		VIDEO_WIDTH = 1280;
		VIDEO_HEIGHT = 720;
		videoFrameRate = 24;
	}

	private Camera mCamera;
	private SurfaceTexture mCameraSurfaceTexture;
	private Context mContext;

	public TrinityCamera(Context context) {
		this.mContext = context;
	}

	public Camera getCamera() {
		return mCamera;
	}

	public CameraConfigInfo configCameraFromNative(int cameraFacingId) {
		if (null != mCamera) {
			releaseCamera();
		}
		if (cameraFacingId >= getNumberOfCameras()) {
			cameraFacingId = 0;
		}
		try {
			return setUpCamera(cameraFacingId);
		} catch (CameraParamSettingException e) {
			mCallback.onPermissionDismiss(e.getMessage());
		}
		int degress = 270;
		int previewWidth = VIDEO_WIDTH;
		int previewHeight = VIDEO_HEIGHT;
		if(null != mCamera){
			try {
				Size previewSize = mCamera.getParameters().getPreviewSize();
				previewWidth = previewSize.width;
				previewHeight = previewSize.height;
			} catch (Exception e) {
				e.printStackTrace();
			}
		}
		return new CameraConfigInfo(degress, previewWidth, previewHeight, cameraFacingId);
	}

	@TargetApi(Build.VERSION_CODES.JELLY_BEAN)
	public void setCameraPreviewTexture(int textureId) {
		Log.i(TAG, "setCameraPreviewTexture...");
		mCameraSurfaceTexture = new SurfaceTexture(textureId);
		try {
			mCamera.setPreviewTexture(mCameraSurfaceTexture);
			mCameraSurfaceTexture.setOnFrameAvailableListener(new SurfaceTexture.OnFrameAvailableListener() {
				@Override
				public void onFrameAvailable(SurfaceTexture surfaceTexture) {
					if (null != mCallback) {
//						Log.d("RecordingPublisher", "surfaceTexture time stamp is "+surfaceTexture.getTimestamp()/1000000000.0f);
						mCallback.notifyFrameAvailable();
					}
				}
			});
			mCamera.startPreview();
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@TargetApi(Build.VERSION_CODES.HONEYCOMB)
	public void updateTexImage() {
		// Log.i(TAG, "UpdateTexImage...");
		try {
			if (null != mCameraSurfaceTexture) {
				mCameraSurfaceTexture.updateTexImage();
				
				//去掉这个没用的调用
//				float[] mTmpMatrix = new float[16];
//				mCameraSurfaceTexture.getTransformMatrix(mTmpMatrix);
//				
//				if (null != mCallback) {
//					mCallback.updateTexMatrix(mTmpMatrix);
//				}
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	public int getNumberOfCameras() {
		return Camera.getNumberOfCameras();
	}

	@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
	public void releaseCamera() {
		try {
			if (mCameraSurfaceTexture != null) {
				// this causes a bunch of warnings that appear harmless but might
				// confuse someone:
				// W BufferQueue: [unnamed-3997-2] cancelBuffer: BufferQueue has
				// been abandoned!
				mCameraSurfaceTexture.release();
				mCameraSurfaceTexture = null;
			}
			if (null != mCamera) {
				mCamera.setPreviewCallback(null);
				mCamera.release();
				mCamera = null;
			}
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	@TargetApi(Build.VERSION_CODES.ICE_CREAM_SANDWICH)
	private CameraConfigInfo setUpCamera(final int id) throws CameraParamSettingException {
//		 forcePreviewSize_640_480();
		forcePreviewSize_1280_720();
		// printStackTrace(CameraLoader.class);
		try {
			// 1、开启Camera
			try {
				mCamera = getCameraInstance(id);
			} catch (CameraParamSettingException e) {
				throw e;
			}
			boolean mHasPermission = hasPermission();
			if (!mHasPermission) {
				throw new CameraParamSettingException("拍摄权限被禁用或被其他程序占用, 请确认后再录制");
			}
			Parameters parameters = mCamera.getParameters();

			// 2、设置预览照片的图像格式
			List<Integer> supportedPreviewFormats = parameters.getSupportedPreviewFormats();
			if (supportedPreviewFormats.contains(ImageFormat.NV21)) {
				parameters.setPreviewFormat(ImageFormat.NV21);
			} else {
				throw new CameraParamSettingException("视频参数设置错误:设置预览图像格式异常");
			}

			// 3、设置预览照片的尺寸
			List<Size> supportedPreviewSizes = parameters.getSupportedPreviewSizes();
			int previewWidth = VIDEO_WIDTH;
			int previewHeight = VIDEO_HEIGHT;
			boolean isSupportPreviewSize = isSupportPreviewSize(supportedPreviewSizes, previewWidth, previewHeight);

			if (isSupportPreviewSize) {
				parameters.setPreviewSize(previewWidth, previewHeight);
			} else {
				previewWidth = DEFAULT_VIDEO_WIDTH;
				previewHeight = DEFAULT_VIDEO_HEIGHT;
				isSupportPreviewSize = isSupportPreviewSize(
						supportedPreviewSizes, previewWidth, previewHeight);
				if (isSupportPreviewSize) {
					VIDEO_WIDTH = DEFAULT_VIDEO_WIDTH;
					VIDEO_HEIGHT = DEFAULT_VIDEO_HEIGHT;
					parameters.setPreviewSize(previewWidth, previewHeight);
				} else {
					throw new CameraParamSettingException("视频参数设置错误:设置预览的尺寸异常");
				}
			}
			//下面这行设置 有可能导致 返回的图像尺寸和预期不一致
//			parameters.setRecordingHint(true);

			// 4、设置视频记录的连续自动对焦模式
			if (parameters.getSupportedFocusModes().contains(Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
				parameters.setFocusMode(Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
			}

			try {
				mCamera.setParameters(parameters);
			} catch (Exception e) {
				throw new CameraParamSettingException("视频参数设置错误");
			}

			int degress = getCameraDisplayOrientation((Activity) mContext, id);
			int cameraFacing = getCameraFacing(id);
			return new CameraConfigInfo(degress, previewWidth, previewHeight, cameraFacing);
		} catch (Exception e) {
			throw new CameraParamSettingException(e.getMessage());
		}
	}

	private boolean hasPermission() {
		boolean mHasPermission = true;
		if (null == mCamera) {
			mHasPermission = false;
		} else {
			try {
				Class<? extends Camera> class1 = mCamera.getClass();
				Field filed = class1.getDeclaredField("mHasPermission");
				if (null != filed) {
					filed.setAccessible(true);
					mHasPermission = (Boolean) filed.get(mCamera);
				}
			} catch (Exception e1) {
			}
		}
		return mHasPermission;
	}

	private boolean isSupportPreviewSize(List<Size> supportedPreviewSizes, int previewWidth, int previewHeight) {
		boolean isSupportPreviewSize = false;
		for (Size size : supportedPreviewSizes) {
			if (previewWidth == size.width && previewHeight == size.height) {
				isSupportPreviewSize = true;
				break;
			}
		}
		return isSupportPreviewSize;
	}

	/** A safe way to Get an instance_ of the Camera object. */
	private Camera getCameraInstance(final int id) throws CameraParamSettingException {
		Log.i("problem", "getCameraInstance id is" + id);
		Camera c = null;
		try {
			c = Camera.open(id);
		} catch (Exception e) {
			throw new CameraParamSettingException("拍摄权限被禁用或被其他程序占用, 请确认后再录制");
		}
		return c;
	}

	public static int getCameraFacing(final int cameraId) {
		int result;
		CameraInfo info = new CameraInfo();
		Camera.getCameraInfo(cameraId, info);
		if (info.facing == CameraInfo.CAMERA_FACING_FRONT) {
			result = 1;
		} else { // back-facing
			result = 0;
		}
		return result;
	}
	public static int getCameraDisplayOrientation(final Activity activity, final int cameraId) {
		int rotation = activity.getWindowManager().getDefaultDisplay().getRotation();
		int degrees = 0;
		switch (rotation) {
		case Surface.ROTATION_0:
			degrees = 0;
			break;
		case Surface.ROTATION_90:
			degrees = 90;
			break;
		case Surface.ROTATION_180:
			degrees = 180;
			break;
		case Surface.ROTATION_270:
			degrees = 270;
			break;
        default:
            break;
		}
		int result;
		CameraInfo info = new CameraInfo();
		Camera.getCameraInfo(cameraId, info);
		if (info.facing == CameraInfo.CAMERA_FACING_FRONT) {
			result = (info.orientation + degrees) % 360;
		} else { // back-facing
			result = (info.orientation - degrees + 360) % 360;
		}
		return result;
	}

	private CameraCallback mCallback;

	public interface CameraCallback {
		public void onPermissionDismiss(String tip);
		
		public void notifyFrameAvailable();
		
		public void updateTexMatrix(float texMatrix[]);
	}

	public void setCallback(CameraCallback callback) {
		this.mCallback = callback;
	}
}
