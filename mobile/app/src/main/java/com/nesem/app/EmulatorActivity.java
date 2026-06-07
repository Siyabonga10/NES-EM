package com.nesem.app;

import android.app.Activity;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.net.Uri;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

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
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class EmulatorActivity extends Activity {
    private NesEmView nesEmView;
    private View pauseMenu;
    private boolean romLoaded;
    private Uri currentRomUri;
    private byte[] romData;
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
        
        FrameLayout root = new FrameLayout(this);
        root.addView(nesEmView);
        
        pauseMenu = getLayoutInflater().inflate(R.layout.pause_menu, root, false);
        root.addView(pauseMenu);
        
        setContentView(root);

        setupPauseMenu();
        loadControlConfig();

        nesEmView.setPauseListener(this::showPauseMenu);
        nesEmView.setSaveListener(this::performSaveState);

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
            currentRomUri = romUri;
            loadRom(romUri);
            if (!isAutoSnapshotTaken(romUri)) {
                scheduleSnapshot(romUri);
            }
        }
    }

    private boolean isAutoSnapshotTaken(Uri uri) {
        try {
            File file = new File(getFilesDir(), "roms.json");
            if (!file.exists()) return false;

            FileInputStream fis = openFileInput("roms.json");
            BufferedReader reader = new BufferedReader(new InputStreamReader(fis, StandardCharsets.UTF_8));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) sb.append(line);
            fis.close();

            JSONArray array = new JSONArray(sb.toString());
            for (int i = 0; i < array.length(); i++) {
                JSONObject obj = array.getJSONObject(i);
                if (obj.getString("uri").equals(uri.toString())) {
                    return obj.optBoolean("autoSnapshotTaken", false);
                }
            }
        } catch (Exception ignored) {}
        return false;
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

    private void setupPauseMenu() {
        pauseMenu.findViewById(R.id.btn_resume).setOnClickListener(v -> hidePauseMenu());
        pauseMenu.findViewById(R.id.btn_reload).setOnClickListener(v -> {
            if (currentRomUri != null) {
                loadRom(currentRomUri);
                hidePauseMenu();
            }
        });
        pauseMenu.findViewById(R.id.btn_settings).setOnClickListener(v -> {
            Intent intent = new Intent(this, SettingsActivity.class);
            startActivity(intent);
        });
        pauseMenu.findViewById(R.id.btn_snapshot).setOnClickListener(v -> {
            if (currentRomUri != null && romLoaded) {
                takeSnapshot(currentRomUri);
                Toast.makeText(this, "Thumbnail updated", Toast.LENGTH_SHORT).show();
            }
        });

        pauseMenu.findViewById(R.id.btn_save_state).setOnClickListener(v -> performSaveState());

        SeekBar volumeBar = pauseMenu.findViewById(R.id.volume_seekbar);
        volumeBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                NesCoreBridge.nativeSetVolume(progress / 100f);
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });
    }

    private File getStatesDir(Uri romUri) {
        if (romUri == null) return null;
        String name = romUri.getLastPathSegment();
        File dir = new File(getFilesDir(), "states/" + name);
        if (!dir.exists()) dir.mkdirs();
        return dir;
    }

    private JSONArray loadStateIndex(Uri romUri) {
        File dir = getStatesDir(romUri);
        File file = new File(dir, "index.json");
        if (!file.exists()) return new JSONArray();
        try {
            FileInputStream fis = new FileInputStream(file);
            BufferedReader reader = new BufferedReader(new InputStreamReader(fis));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) sb.append(line);
            fis.close();
            return new JSONArray(sb.toString());
        } catch (Exception e) {
            return new JSONArray();
        }
    }

    private void saveStateIndex(Uri romUri, JSONArray index) {
        File dir = getStatesDir(romUri);
        File file = new File(dir, "index.json");
        try {
            FileOutputStream fos = new FileOutputStream(file);
            fos.write(index.toString().getBytes(StandardCharsets.UTF_8));
            fos.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void performSaveState() {
        if (!romLoaded || currentRomUri == null) return;
        try {
            byte[] buf = new byte[1024 * 1024];
            int written = NesCoreBridge.nativeSaveState(buf);
            if (written <= 0) return;

            String timestamp = new SimpleDateFormat("yyyyMMdd_HHmmssSSS", Locale.US).format(new Date());
            File dir = getStatesDir(currentRomUri);
            
            // Save state
            File stateFile = new File(dir, timestamp + "_state.bin");
            FileOutputStream fos = new FileOutputStream(stateFile);
            fos.write(buf, 0, written);
            fos.close();

            // Save thumbnail
            Bitmap bmp = nesEmView.getBitmap();
            if (bmp != null) {
                Bitmap scaled = Bitmap.createScaledBitmap(bmp, 160, 140, true);
                File thumbFile = new File(dir, timestamp + "_thumb.jpg");
                FileOutputStream out = new FileOutputStream(thumbFile);
                scaled.compress(Bitmap.CompressFormat.JPEG, 70, out);
                out.close();
            }

            // Update index
            JSONArray index = loadStateIndex(currentRomUri);
            JSONObject entry = new JSONObject();
            entry.put("timestamp", timestamp);
            entry.put("bytes", written);
            entry.put("thumb", timestamp + "_thumb.jpg");
            entry.put("date", new SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.US).format(new Date()));
            index.put(entry);
            saveStateIndex(currentRomUri, index);

            Toast.makeText(this, "State saved", Toast.LENGTH_SHORT).show();
            refreshStateList();
        } catch (Exception e) {
            e.printStackTrace();
            Toast.makeText(this, "Save failed", Toast.LENGTH_SHORT).show();
        }
    }

    private void refreshStateList() {
        if (currentRomUri == null) return;
        LinearLayout container = pauseMenu.findViewById(R.id.states_list_container);
        container.removeAllViews();
        
        JSONArray index = loadStateIndex(currentRomUri);
        View noStates = pauseMenu.findViewById(R.id.tv_no_states);
        
        if (index.length() == 0) {
            noStates.setVisibility(View.VISIBLE);
        } else {
            noStates.setVisibility(View.GONE);
            LayoutInflater inflater = getLayoutInflater();
            File dir = getStatesDir(currentRomUri);

            for (int i = index.length() - 1; i >= 0; i--) {
                try {
                    JSONObject entry = index.getJSONObject(i);
                    View item = inflater.inflate(R.layout.state_item, container, false);
                    
                    ((TextView) item.findViewById(R.id.state_date)).setText(entry.getString("date"));
                    ((TextView) item.findViewById(R.id.state_size)).setText((entry.getInt("bytes") / 1024) + " KB");
                    
                    ImageView thumbView = item.findViewById(R.id.state_thumb);
                    File thumbFile = new File(dir, entry.getString("thumb"));
                    if (thumbFile.exists()) {
                        thumbView.setImageURI(Uri.fromFile(thumbFile));
                    }

                    item.findViewById(R.id.btn_load).setOnClickListener(v -> loadState(entry));
                    item.findViewById(R.id.btn_delete).setOnClickListener(v -> {
                        try {
                            deleteStateFiles(currentRomUri, entry.getString("timestamp"));
                            refreshStateList();
                        } catch (Exception e) {}
                    });

                    container.addView(item);
                } catch (Exception e) {}
            }
        }
    }

    private void loadState(JSONObject entry) {
        if (!romLoaded || romData == null) return;
        try {
            File dir = getStatesDir(currentRomUri);
            File stateFile = new File(dir, entry.getString("timestamp") + "_state.bin");
            if (!stateFile.exists()) return;

            FileInputStream fis = new FileInputStream(stateFile);
            byte[] stateData = new byte[(int) stateFile.length()];
            fis.read(stateData);
            fis.close();

            int rc = NesCoreBridge.nativeLoadState(romData, stateData);
            if (rc == 0) {
                Toast.makeText(this, "State loaded", Toast.LENGTH_SHORT).show();
                hidePauseMenu();
            } else {
                Toast.makeText(this, "Load failed", Toast.LENGTH_SHORT).show();
            }
        } catch (Exception e) {
            Toast.makeText(this, "Load failed", Toast.LENGTH_SHORT).show();
        }
    }

    private void deleteStateFiles(Uri romUri, String timestamp) {
        File dir = getStatesDir(romUri);
        new File(dir, timestamp + "_state.bin").delete();
        new File(dir, timestamp + "_thumb.jpg").delete();
        
        JSONArray index = loadStateIndex(romUri);
        JSONArray newIndex = new JSONArray();
        for (int i = 0; i < index.length(); i++) {
            try {
                JSONObject entry = index.getJSONObject(i);
                if (!entry.getString("timestamp").equals(timestamp)) {
                    newIndex.put(entry);
                }
            } catch (Exception e) {}
        }
        saveStateIndex(romUri, newIndex);
    }

    private void showPauseMenu() {
        nesEmView.setPaused(true);
        refreshStateList();
        pauseMenu.setVisibility(View.VISIBLE);
    }

    private void hidePauseMenu() {
        pauseMenu.setVisibility(View.GONE);
        nesEmView.setPaused(false);
    }

    private void loadControlConfig() {
        try {
            File file = new File(getFilesDir(), "controls_config.json");
            if (file.exists()) {
                FileInputStream fis = new FileInputStream(file);
                byte[] data = new byte[(int) file.length()];
                int bytesRead = fis.read(data);
                fis.close();
                if (bytesRead > 0) {
                    ControlConfig config = ControlConfig.fromJson(new String(data, 0, bytesRead, StandardCharsets.UTF_8));
                    if (config != null) {
                        nesEmView.setControlConfig(config);
                    }
                }
            }
        } catch (Exception ignored) {}
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
                    obj.put("autoSnapshotTaken", true);
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
            romData = rom;

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
