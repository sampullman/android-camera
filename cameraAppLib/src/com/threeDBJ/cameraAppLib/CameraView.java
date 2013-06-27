package com.threeDBJ.cameraAppLib;

import android.content.Context;
import android.graphics.Bitmap;
import android.util.Log;
import android.util.AttributeSet;
import android.graphics.BitmapFactory;
import android.graphics.Color;

class CameraView extends CameraBase {

    private int mFrameSize;
    private byte[] dataCur, dataSet;
    private Bitmap mBitmap, bmpCur, bmpResult;
    private int[] mRGBA;

    public CameraView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    protected void onPreviewStarted(int previewWidth, int previewHeight) {
	mFrameSize = previewWidth * previewHeight;
	mRGBA = new int[mFrameSize];
	dataCur = new byte[mFrameSize*4];
	dataSet = new byte[mFrameSize*4];
	mBitmap = Bitmap.createBitmap(previewWidth, previewHeight, Bitmap.Config.ARGB_8888);
	bmpResult = Bitmap.createBitmap(previewWidth, previewHeight, Bitmap.Config.ARGB_8888);
    }

    @Override
    protected void onPreviewStopped() {
	if(mBitmap != null) {
	    mBitmap.recycle();
	    mBitmap = null;
	}
	mRGBA = null;
    }

    @Override
    protected Bitmap processFrame(byte[] data) {
	if(mode == 2) {
	    //Log.e("CameraApp", "old");	    mThreadRun = false;
	    return bmpResult;
	} else {
	    System.arraycopy(data, 0, dataCur, 0, data.length);
	    //dataCur = data;
	    int[] rgba = mRGBA;
	    //FindFeatures(getFrameWidth(), getFrameHeight(), data, rgba);
	    ByteToInt(getFrameWidth(), getFrameHeight(), data, rgba);
	    bmpCur = mBitmap;
	    bmpCur.setPixels(rgba, 0/* offset */, getFrameWidth() /* stride */, 0, 0, getFrameWidth(), getFrameHeight());
	    //Log.e("CameraApp", "made a baby");
	    return bmpCur;
	}
    }


    public void handleCapture() {
	mode = (mode + 1) % 3;
	try{Thread.sleep(100);}catch(Exception e){};
	Log.e("CameraApp", "captured");
	switch(mode) {
	case 1:
	    System.arraycopy(dataCur, 0, dataSet, 0, dataCur.length);
	    break;
	case 2:
	    //mCamera.stopPreview();
	    Log.e("CameraApp", "wtf");
	    int[] rgba = mRGBA;
	    //for(int i=0;i<rgba.length;i+=1) rgba[i] = Color.BLACK;
	    //bmpCur = mBitmap;
	    MatchHomography(getFrameWidth(), getFrameHeight(), dataSet, dataCur, rgba);
	    bmpResult.setPixels(rgba, 0/* offset */, getFrameWidth() /* stride */, 0, 0, getFrameWidth(), getFrameHeight());
	    break;
	case 0:
	    //mCamera.startPreview();
	    break;
	}
    }

    public native void FindFeatures(int width, int height, byte yuv[], int[] rgba);
    public native void ByteToInt(int width, int height, byte yuv[], int[] rgba);
    public native void MatchHomography(int width, int height, byte yuv1[], byte yuv2[], int[] rgba);
}
