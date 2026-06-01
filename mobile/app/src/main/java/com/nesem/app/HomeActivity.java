package com.nesem.app;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class HomeActivity extends Activity {
    private static final String ROMS_FILE = "roms.json";
    private GridLayout gridView;
    private TextView emptyView;
    private List<LocalRomInfo> romList = new ArrayList<>();

    @Override
    public void onCreate(Bundle savedInstances) {
        super.onCreate(savedInstances);
        setContentView(R.layout.activity_home);

        gridView = findViewById(R.id.list_view);
        emptyView = findViewById(R.id.empty_view);

        findViewById(R.id.add_rom_button).setOnClickListener(v -> pickRom());
    }

    @Override
    protected void onResume() {
        super.onResume();
        loadPersistedRoms();
        updateUI();
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

        Uri uri = data.getData();
        if (uri == null) return;

        getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);

        String uriString = uri.toString();
        for (LocalRomInfo existing : romList) {
            if (existing.getUriString().equals(uriString)) {
                Toast.makeText(this, "ROM already in list", Toast.LENGTH_SHORT).show();
                return;
            }
        }

        String rawName = getFileName(uri);
        String gameName = parseGameName(rawName);

        LocalRomInfo info = new LocalRomInfo(uriString, gameName, "", System.currentTimeMillis(), false);
        romList.add(info);
        saveRoms();
        updateUI();
    }

    private String getFileName(Uri uri) {
        String result = null;
        if ("content".equals(uri.getScheme())) {
            try (Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) {
                    int idx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    if (idx != -1) result = cursor.getString(idx);
                }
            }
        }
        if (result == null) {
            result = uri.getPath();
            if (result != null) {
                int cut = result.lastIndexOf('/');
                if (cut != -1) result = result.substring(cut + 1);
            }
        }
        return result;
    }

    private String parseGameName(String fileName) {
        if (fileName == null) return "Unknown";
        String name = fileName;
        int lastSlash = name.lastIndexOf('/');
        if (lastSlash != -1) name = name.substring(lastSlash + 1);
        int dot = name.lastIndexOf(".nes");
        if (dot != -1) name = name.substring(0, dot);
        return name;
    }

    private void updateUI() {
        gridView.removeAllViews();
        Collections.sort(romList, (a, b) -> Long.compare(b.getLastOpened(), a.getLastOpened()));

        if (romList.isEmpty()) {
            emptyView.setVisibility(View.VISIBLE);
        } else {
            emptyView.setVisibility(View.GONE);
            for (LocalRomInfo info : romList) {
                View card = createViewFromRom(info);
                
                // Set GridParams for 2-column layout
                GridLayout.LayoutParams params = new GridLayout.LayoutParams();
                params.width = 0;
                params.height = ViewGroup.LayoutParams.WRAP_CONTENT;
                params.columnSpec = GridLayout.spec(GridLayout.UNDEFINED, 1f);
                params.setMargins(16, 16, 16, 16);
                card.setLayoutParams(params);
                
                gridView.addView(card);
            }
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    private View createViewFromRom(LocalRomInfo info) {
        View romView = LayoutInflater.from(this).inflate(R.layout.list_item, gridView, false);
        TextView romLabel = romView.findViewById(R.id.item_text);
        ImageView romImage = romView.findViewById(R.id.item_image);
        romLabel.setText(info.getName());

        if (info.getThumbnailPath() != null && !info.getThumbnailPath().isEmpty()) {
            File imgFile = new File(info.getThumbnailPath());
            if (imgFile.exists()) {
                romImage.setImageBitmap(BitmapFactory.decodeFile(imgFile.getAbsolutePath()));
            }
        }

        // Tactile scale feedback
        romView.setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    v.animate().scaleX(0.95f).scaleY(0.95f).setDuration(100).start();
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    v.animate().scaleX(1.0f).scaleY(1.0f).setDuration(100).start();
                    break;
            }
            return false;
        });

        romView.setOnClickListener(v -> {
            Uri uri = Uri.parse(info.getUriString());
            if (fileExists(uri)) {
                info.setLastOpened(System.currentTimeMillis());
                saveRoms();
                Intent intent = new Intent(this, EmulatorActivity.class);
                intent.setData(uri);
                startActivity(intent);
            } else {
                Toast.makeText(this, "File not found. Removing from list.", Toast.LENGTH_SHORT).show();
                romList.remove(info);
                saveRoms();
                updateUI();
            }
        });

        return romView;
    }

    private boolean fileExists(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
            return cursor != null && cursor.moveToFirst();
        } catch (Exception e) {
            return false;
        }
    }

    private void saveRoms() {
        try {
            JSONArray array = new JSONArray();
            for (LocalRomInfo info : romList) {
                array.put(info.toJsonObject());
            }
            FileOutputStream fos = openFileOutput(ROMS_FILE, Context.MODE_PRIVATE);
            fos.write(array.toString().getBytes(StandardCharsets.UTF_8));
            fos.close();
        } catch (Exception ignored) {}
    }

    private void loadPersistedRoms() {
        try {
            File file = new File(getFilesDir(), ROMS_FILE);
            if (!file.exists()) return;
            FileInputStream fis = openFileInput(ROMS_FILE);
            InputStreamReader isr = new InputStreamReader(fis, StandardCharsets.UTF_8);
            BufferedReader reader = new BufferedReader(isr);
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) sb.append(line);
            fis.close();

            JSONArray array = new JSONArray(sb.toString());
            romList.clear();
            for (int i = 0; i < array.length(); i++) {
                romList.add(LocalRomInfo.fromJsonObject(array.getJSONObject(i)));
            }
        } catch (Exception ignored) {}
    }
}
