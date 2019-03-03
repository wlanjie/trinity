package com.trinity.camera;

import android.content.Context;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;

public class TrinityPreviewView extends SurfaceView implements Callback {

	public TrinityPreviewView(Context context) {
		super(context);
		SurfaceHolder surfaceHolder = getHolder();
		surfaceHolder.addCallback(this);
		surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		Surface surface = holder.getSurface();
		int width = getWidth();
		int height = getHeight();
		if(null != mCallback){
			mCallback.createSurface(surface, width, height);
		}
	}

	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
							   int height) {
		if(null != mCallback){
			mCallback.resetRenderSize(width, height);
		}
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		if(null != mCallback){
			mCallback.destroySurface();
		}
	}
	
	private PreviewViewCallback mCallback;
	public void setCallback(PreviewViewCallback callback){
		this.mCallback = callback;
	}
	public interface PreviewViewCallback {
		void createSurface(Surface surface, int width, int height);
		void resetRenderSize(int width, int height);
		void destroySurface();
	}
}
