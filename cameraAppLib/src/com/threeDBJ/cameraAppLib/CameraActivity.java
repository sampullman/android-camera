package com.threeDBJ.cameraAppLib;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.Window;
import android.widget.Button;
import android.view.View;
import android.view.View.OnClickListener;

public class CameraActivity extends Activity {
    private static final String TAG = "Camera::Activity";
    private CameraView mView;

    static {
	if (!OpenCVLoader.initDebug()) {
	    Log.e(TAG, "loader failed");
	    // Handle initialization error
	}
    }

    private BaseLoaderCallback  mOpenCVCallBack = new BaseLoaderCallback(this) {
	    @Override
	    public void onManagerConnected(int status) {
    		switch (status) {
		case LoaderCallbackInterface.SUCCESS:
		    Log.i(TAG, "OpenCV loaded successfully");
		    // Load native library after(!) OpenCV initialization
		    System.loadLibrary("native_sample");

		    // Create and set View
		    setupUI();
		    // Check native OpenCV camera
		    if( !mView.openCamera() ) {
			AlertDialog ad = new AlertDialog.Builder(mAppContext).create();
			ad.setCancelable(false); // This blocks the 'BACK' button
			ad.setMessage("Fatal error: can't open camera!");
			ad.setButton("OK", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
				    dialog.dismiss();
				    finish();
				}
			    });
			ad.show();
		    }
		    break;
		default:
		    super.onManagerConnected(status);
		    break;
		}
	    }
	};

    public CameraActivity() {
	Log.i(TAG, "Instantiated new " + this.getClass());
    }

    @Override
    protected void onPause() {
	Log.i(TAG, "onPause");
	super.onPause();
	if (null != mView)
	    mView.releaseCamera();
    }

    @Override
    protected void onResume() {
	Log.i(TAG, "onResume");
	super.onResume();
	if((null != mView) && !mView.openCamera() ) {
	    AlertDialog ad = new AlertDialog.Builder(this).create();
	    ad.setCancelable(false); // This blocks the 'BACK' button
	    ad.setMessage("Fatal error: can't open camera!");
	    ad.setButton("OK", new DialogInterface.OnClickListener() {
		    public void onClick(DialogInterface dialog, int which) {
			dialog.dismiss();
			finish();
		    }
		});
	    ad.show();
	}
    }

    public void setupUI() {
	setContentView(R.layout.main);
	mView = (CameraView) findViewById(R.id.camera_view);
	Button b = (Button) findViewById(R.id.capture);
	b.setOnClickListener(new OnClickListener() {
		public void onClick(View v) {
		    mView.handleCapture();
		}
	    });
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
	Log.i(TAG, "onCreate");
	super.onCreate(savedInstanceState);
	requestWindowFeature(Window.FEATURE_NO_TITLE);
	Log.i(TAG, "OpenCV loaded successfully");

	// Load native library after(!) OpenCV initialization
	System.loadLibrary("native_sample");

	// Create and set View
	setupUI();
	// Check native OpenCV camera
	if( !mView.openCamera() ) {
	    AlertDialog ad = new AlertDialog.Builder(this).create();
	    ad.setCancelable(false); // This blocks the 'BACK' button
	    ad.setMessage("Fatal error: can't open camera!");
	    ad.setButton("OK", new DialogInterface.OnClickListener() {
		    public void onClick(DialogInterface dialog, int which) {
			dialog.dismiss();
			finish();
		    }
		});
	    ad.show();
	}
	// if (!OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_2, this, mOpenCVCallBack))
	// {
	// 	Log.e(TAG, "Cannot connect to OpenCV Manager");
	// }
    }
}
