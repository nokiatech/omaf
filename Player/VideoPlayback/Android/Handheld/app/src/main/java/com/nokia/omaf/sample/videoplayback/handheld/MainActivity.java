
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
package com.nokia.omaf.sample.videoplayback.handheld;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.AssetManager;
import android.graphics.Point;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.hardware.SensorEvent;
import android.hardware.Sensor;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.util.Log;
import android.util.LogPrinter;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import java.io.File;
import java.io.IOException;
import java.util.Dictionary;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import static android.opengl.GLES30.*;

public class MainActivity extends Activity implements SensorEventListener, GLSurfaceView.OnTouchListener
{
    private GLSurfaceView mGLView;
    private long mApplicationWrapper;
    private AssetManager mAssetManager;

    // Sensors for device rotation
    private SensorManager mSensorManager;
    private Sensor mSensor;

    // Video asset
    String mVideoURI = null;
    static
    {
        System.loadLibrary("OMAFPlayerWrapper");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        final Activity currentActivity = this;
        Intent intent = getIntent();
        mVideoURI = intent.getStringExtra("uri");
        System.out.println("Video URI: " + mVideoURI);
        // Ensure fullscreen immersion.
        setImmersiveSticky();

        getWindow().getDecorView().setOnSystemUiVisibilityChangeListener
        (
            new View.OnSystemUiVisibilityChangeListener()
            {
                @Override
                public void onSystemUiVisibilityChange(int visibility)
                {
                if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0)
                {
                    setImmersiveSticky();
                }
                }
        });

        // Get asset manager. Sample video is in samples.
        mAssetManager = getResources().getAssets();

        // Create a GLSurfaceView. We do the rendering with OpenGLES into this.
        mGLView = new GLSurfaceView(this);
        mGLView.setEGLContextClientVersion(2);
        mGLView.setEGLConfigChooser(8, 8, 8, 0, 16, 0);
        mGLView.setPreserveEGLContextOnPause(true);
        mGLView.setRenderer
        (
            new GLSurfaceView.Renderer()
            {
                @Override
                public void onSurfaceCreated(GL10 gl, EGLConfig config)
                {
                    // Get display metrics
                    DisplayMetrics metrics = getResources().getDisplayMetrics();
                    Display display = getWindowManager().getDefaultDisplay();
                    Point screenSize = new Point();
                    display.getRealSize(screenSize);
                    glClearColor(0f, 0f,1.0f, 0f);

                    // Create tha native library
                    mApplicationWrapper = nativeCreateApplication(mAssetManager,
                                                                 currentActivity,
                                                                 getApplicationContext().getExternalFilesDir(null).getPath(),
                                                                 getApplicationContext().getCacheDir().getPath(),
                                                                 screenSize.x,
                                                                 screenSize.y);
                    nativeLoadContent(mVideoURI);
                }

                @Override
                public void onSurfaceChanged(GL10 gl, int width, int height)
                {

                }

                @Override
                public void onDrawFrame(GL10 gl)
                {
                    nativeDraw(mApplicationWrapper);
                }
        });

        setContentView(mGLView);
        mGLView.setOnTouchListener(this);
        // Prevent screen from dimming/locking.
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        //Get sensor manager for device direction. This data is needed by the library
        mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        mSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
    }

    @Override
    protected void onPause()
    {
        super.onPause();
        nativeOnPause(mApplicationWrapper);
        mGLView.onPause();
        mSensorManager.unregisterListener(this);
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        nativeOnResume(mApplicationWrapper);
        mGLView.onResume();

        if (mSensor != null) {
            mSensorManager.registerListener(this, mSensor,
                    mSensor.getMinDelay(), SensorManager.SENSOR_DELAY_FASTEST);
        }
    }

    @Override
    protected void onDestroy()
    {
        super.onDestroy();
        mSensorManager.unregisterListener(this);
        // Destruction order is important; shutting down the GvrLayout will detach
        // the GLSurfaceView and stop the GL thread, allowing safe shutdown of
        // native resources from the UI thread.
        nativeDestroyApplication(mApplicationWrapper);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus)
    {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus)
        {
            setImmersiveSticky();
        }
    }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event)
  {
    return super.dispatchKeyEvent(event);
  }

    private void setImmersiveSticky()
    {
        getWindow().getDecorView().setSystemUiVisibility
                (
                        View.SYSTEM_UI_FLAG_LAYOUT_STABLE |
                                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                                View.SYSTEM_UI_FLAG_FULLSCREEN |
                                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                );
    }

  // from SensorEventListener

  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy) { }

  @Override
  public void onSensorChanged(SensorEvent event) {
        // Pass the rotation data to the native library.
        if (event.sensor.getType() == Sensor.TYPE_GAME_ROTATION_VECTOR) {
            nativeUpdateRotation(event.values[0],
                    event.values[1],
                    event.values[2],
                    event.values[3]);
        }
  }



    // from View.OnTouchListener
    @Override
    public boolean onTouch(View v, MotionEvent e) {
        nativeOnTapEvent();
        return true;
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)  {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
            onPause();
        }

        return super.onKeyDown(keyCode, event);
    }

    // Native functions. Implemented in ApplicationJNI.cpp
    private native long nativeCreateApplication(AssetManager assetManager, Activity activity, String externalStoragePath, String cachePath, int width, int height);
    private native void nativeDestroyApplication(long applicationWrapper);

    private native void nativeLoadContent(String uri);

    private native void nativeUpdateRotation(float x, float y, float z, float w);
    private native void nativeDraw(long applicationWrapper);

    private native void nativeOnPause(long applicationWrapper);
    private native void nativeOnResume(long applicationWrapper);

    private native void nativeOnTapEvent();
}
