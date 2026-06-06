package com.nesem.app;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.SeekBar;
import android.widget.TextView;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

public class SettingsActivity extends Activity {
    private LayoutEditorView editorView;
    private SeekBar scaleSeekBar;
    private TextView selectedLabel;
    private ControlConfig config;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);

        editorView = findViewById(R.id.layout_editor);
        scaleSeekBar = findViewById(R.id.scale_seekbar);
        selectedLabel = findViewById(R.id.selected_label);

        loadConfig();

        editorView.setListener((type, scale) -> {
            selectedLabel.setText(String.format(Locale.US, "Selected: %s (Scale: %.1f)", type.name(), scale));
            scaleSeekBar.setVisibility(View.VISIBLE);
            scaleSeekBar.setProgress((int) (scale * 100));
        });

        scaleSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    float scale = progress / 100f;
                    editorView.setScale(scale);
                    selectedLabel.setText(String.format(Locale.US, "Selected: %s (Scale: %.1f)", editorView.getSelectedType().name(), scale));
                }
            }
            @Override public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        findViewById(R.id.btn_save_exit).setOnClickListener(v -> {
            saveConfig();
            finish();
        });
    }

    private void loadConfig() {
        try {
            File file = new File(getFilesDir(), "controls_config.json");
            if (file.exists()) {
                FileInputStream fis = new FileInputStream(file);
                byte[] data = new byte[(int) file.length()];
                int bytesRead = fis.read(data);
                fis.close();
                if (bytesRead > 0) {
                    config = ControlConfig.fromJson(new String(data, 0, bytesRead, StandardCharsets.UTF_8));
                }
            }
        } catch (Exception ignored) {}

        if (config == null) {
            config = new ControlConfig();
            // We need screen dimensions to set defaults.
            editorView.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    editorView.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                    initDefaults();
                    editorView.setConfig(config);
                }
            });
        } else {
            editorView.setConfig(config);
        }
    }

    private void initDefaults() {
        float w = editorView.getWidth();
        float h = editorView.getHeight();
        float d = getResources().getDisplayMetrics().density;
        float cy = h / 2f;

        config.dpad = new ControlConfig.Component(108 * d / w, cy / h, 1.0f);
        config.btnB = new ControlConfig.Component((w - 60 * d) / w, (cy - 34 * d) / h, 1.0f);
        config.btnA = new ControlConfig.Component((w - 60 * d) / w, (cy + 34 * d) / h, 1.0f);
        config.btnStart = new ControlConfig.Component((w - 62 * d) / w, (h - 48 * d) / h, 1.0f);
        config.btnSelect = new ControlConfig.Component(62 * d / w, (h - 48 * d) / h, 1.0f);
    }

    private void saveConfig() {
        if (config == null) return;
        try {
            FileOutputStream fos = openFileOutput("controls_config.json", Context.MODE_PRIVATE);
            fos.write(config.toJsonObject().toString().getBytes(StandardCharsets.UTF_8));
            fos.close();
        } catch (Exception ignored) {}
    }
}
