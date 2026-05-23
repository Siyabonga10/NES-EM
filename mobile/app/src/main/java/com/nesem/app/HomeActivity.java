package com.nesem.app;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.io.InputStream;
import java.util.Locale;

public class HomeActivity extends Activity {
    LinearLayout list_view;

    @Override
    public void onCreate(Bundle savedInstances) {
        super.onCreate(savedInstances);
        setContentView(R.layout.activity_home);

        list_view = findViewById(R.id.list_view);
        findViewById(R.id.add_rom_button).setOnClickListener(v -> pickRom());
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

            // In a real app, we'd store the URI or name. For now, just show it in the list.
            String fileName = uri.getLastPathSegment();
            list_view.addView(createViewFromRom(fileName, uri));

        } catch (Exception ignored) {
        }
    }

    private View createViewFromRom(String name, Uri uri) {
        View romView = LayoutInflater.from(this).inflate(R.layout.list_item, list_view, false);
        TextView romLabel = romView.findViewById(R.id.item_text);
        romLabel.setText(name);

        romView.setOnClickListener(v -> {
            Intent intent = new Intent(this, MainActivity.class);
            intent.setData(uri);
            startActivity(intent);
        });

        return romView;
    }
}
