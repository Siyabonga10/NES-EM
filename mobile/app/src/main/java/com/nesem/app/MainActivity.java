package com.nesem.app;

import android.app.Activity;
import android.content.Intent;
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

        nesEmView.setOnLongClickListener(v -> {
            pickRom();
            return true;
        });

        nesEmView.setLoadRomListener(this::pickRom);
    }

    private void pickRom() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, 1);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != 1 || resultCode != RESULT_OK || data == null) return;

        try {
            Uri uri = data.getData();
            if (uri == null) return;
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
