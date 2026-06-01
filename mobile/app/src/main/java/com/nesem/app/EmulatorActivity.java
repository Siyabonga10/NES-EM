package com.nesem.app;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;

public class EmulatorActivity extends Activity {
    private NesEmView nesEmView;
    private boolean romLoaded;
    private final Handler fpsHandler = new Handler(Looper.getMainLooper());
    private final Handler snapshotHandler = new Handler(Looper.getMainLooper());

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

        Uri romUri = null;
        if (savedInstanceState != null) {
            String uriStr = savedInstanceState.getString("rom_uri");
            if (uriStr != null) romUri = Uri.parse(uriStr);
        }
        
        if (romUri == null) {
            romUri = getIntent().getData();
        }

        if (romUri != null) {
            loadRom(romUri);
            scheduleSnapshot(romUri);
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        Uri romUri = getIntent().getData();
        if (romUri != null) {
            outState.putString("rom_uri", romUri.toString());
        }
    }

    private void scheduleSnapshot(Uri uri) {
        snapshotHandler.postDelayed(() -> takeSnapshot(uri), 45000);
    }

    private void takeSnapshot(Uri uri) {
        if (!romLoaded) return;
        Bitmap bmp = nesEmView.getBitmap();
        if (bmp == null) return;

        try {
            // Save bitmap to file
            String filename = "thumb_" + Math.abs(uri.toString().hashCode()) + ".png";
            File file = new File(getFilesDir(), filename);
            FileOutputStream out = new FileOutputStream(file);
            bmp.compress(Bitmap.CompressFormat.PNG, 90, out);
            out.flush();
            out.close();

            updateRomThumbnail(uri.toString(), file.getAbsolutePath());
        } catch (Exception ignored) {}
    }

    private void updateRomThumbnail(String uriString, String path) {
        try {
            File file = new File(getFilesDir(), "roms.json");
            if (!file.exists()) return;

            FileInputStream fis = openFileInput("roms.json");
            BufferedReader reader = new BufferedReader(new InputStreamReader(fis, StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) sb.append(line);
            fis.close();

            JSONArray array = new JSONArray(sb.toString());
            for (int i = 0; i < array.length(); i++) {
                JSONObject obj = array.getJSONObject(i);
                if (obj.getString("uri").equals(uriString)) {
                    obj.put("thumbnail", path);
                    break;
                }
            }

            FileOutputStream fos = openFileOutput("roms.json", Context.MODE_PRIVATE);
            fos.write(array.toString().getBytes(StandardCharsets.UTF_8));
            fos.close();
        } catch (Exception ignored) {}
    }

    private void loadRom(Uri uri) {
        try {
            InputStream is = getContentResolver().openInputStream(uri);
            if (is == null) return;
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
    protected void onStop() {
        super.onStop();
        snapshotHandler.removeCallbacksAndMessages(null);
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
        snapshotHandler.removeCallbacksAndMessages(null);
        if (romLoaded) {
            NesCoreBridge.nativeStopLoop();
            NesCoreBridge.nativeShutdown();
        }
    }
}
