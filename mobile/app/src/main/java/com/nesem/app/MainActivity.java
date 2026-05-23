package com.nesem.app;

import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;

public class MainActivity extends Activity {
    private NesEmView nesEmView;
    private boolean romLoaded;
    private Handler fpsHandler = new Handler(Looper.getMainLooper());

    static {
        System.loadLibrary("nescore");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().getDecorView().setSystemUiVisibility(
            android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
            | android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | android.view.View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
            | android.view.View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
        );
        nesEmView = new NesEmView(this);
        setContentView(nesEmView);

        Runnable fpsPoller = new Runnable() {
            public void run() {
                if (romLoaded) {
                    nesEmView.setFps(NesCoreBridge.nativeGetFps());
                    nesEmView.invalidate();
                }
                fpsHandler.postDelayed(this, 500);
            }
        };
        fpsHandler.postDelayed(fpsPoller, 500);

        // Check for ROM URI passed from HomeActivity
        Uri romUri = getIntent().getData();
        if (romUri != null) {
            loadRom(romUri);
        }
    }

    private void loadRom(Uri uri) {
        try {
            InputStream is = getContentResolver().openInputStream(uri);
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            byte[] buf = new byte[8192];
            int n;
            while ((n = is.read(buf)) != -1) bos.write(buf, 0, n);
            is.close();

            byte[] rom = bos.toByteArray();

            if (romLoaded) {
                NesCoreBridge.nativeStopLoop();
                NesCoreBridge.nativeShutdown();
            }

            int rc = NesCoreBridge.nativeLoadRom(rom);
            if (rc != 0) return;

            NesCoreBridge.nativeStartLoop(nesEmView.getBitmap(), nesEmView);
            romLoaded = true;
            nesEmView.setRomLoaded(true);
        } catch (Exception ignored) {
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (romLoaded && !nesEmView.isPaused()) {
            nesEmView.setPaused(true);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (romLoaded && nesEmView.isPaused()) {
            nesEmView.setPaused(false);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        fpsHandler.removeCallbacksAndMessages(null);
        if (romLoaded) {
            NesCoreBridge.nativeStopLoop();
            NesCoreBridge.nativeShutdown();
        }
    }
}
