package com.nesem.app;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;

public class NesEmView extends View {
    public static final int GAME_W = 256;
    public static final int GAME_H = 224;

    private static final int KEY_A = 0, KEY_B = 1, KEY_UP = 2, KEY_DOWN = 3;
    private static final int KEY_LEFT = 4, KEY_RIGHT = 5, KEY_START = 6, KEY_SELECT = 7;

    private static final int KEY_PAUSE = 100;
    private static final int KEY_UNCAPPED = 101;
    private static final int KEY_ROM = 102;

    private DPad[] dpads = new DPad[4];

    private final Bitmap bitmap;
    private final Paint paint;
    private final RectF srcRect;
    private final RectF dstRect;

    private final RectF[] keyRects = new RectF[8];
    private final int[] pointerKey = new int[20];

    private final RectF pauseRect = new RectF();
    private final RectF uncapRect = new RectF();
    private final RectF romRect = new RectF();

    private boolean paused;
    private boolean uncapped;
    private int fps;
    private boolean romLoaded;
    private Runnable loadRomListener;
    private Runnable pauseListener;
    private ControlConfig controlConfig;

    public NesEmView(Context context) {
        super(context);
        setBackgroundColor(0xFF111111);
        bitmap = Bitmap.createBitmap(GAME_W, GAME_H, Bitmap.Config.ARGB_8888);
        paint = new Paint();
        paint.setFilterBitmap(false);
        srcRect = new RectF(0, 0, GAME_W, GAME_H);
        dstRect = new RectF();
        for (int i = 0; i < 8; i++) keyRects[i] = new RectF();
        for (int i = 0; i < pointerKey.length; i++) pointerKey[i] = -1;
    }

    private void initDPads(int h, int centerX) {
        Point center = new Point(centerX, h / 2);
        int padH = 150;
        int padW = 180;

        this.dpads[0] = new DPad(center);
        this.dpads[0].buildUp(padH, padW);

        this.dpads[1] = new DPad(center);
        this.dpads[1].buildRight(padH, padW);

        this.dpads[2] = new DPad(center);
        this.dpads[2].buildLeft(padH, padW);

        this.dpads[3] = new DPad(center);
        this.dpads[3].buildDown(padH, padW);
    }

    public Bitmap getBitmap() {
        return bitmap;
    }

    public boolean isPaused() { return paused; }
    public void setPaused(boolean p) {
        paused = p;
        if (p) NesCoreBridge.nativePauseLoop();
        else NesCoreBridge.nativeResumeLoop();
    }

    public boolean isUncapped() { return uncapped; }
    public void setUncapped(boolean u) {
        uncapped = u;
        NesCoreBridge.nativeSetUncapped(u);
    }

    public void setLoadRomListener(Runnable r) { loadRomListener = r; }
    public void setPauseListener(Runnable r) { pauseListener = r; }
    public void setRomLoaded(boolean v) { romLoaded = v; }
    public void setControlConfig(ControlConfig config) {
        this.controlConfig = config;
        invalidate();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        float scale = Math.min((float) w / GAME_W, (float) h / GAME_H);
        float bw = GAME_W * scale;
        float bh = GAME_H * scale;
        float dstCenterX = w / 2f + (w - bw) * 0.10f;
        dstRect.set(dstCenterX - bw / 2f, (h - bh) / 2f, dstCenterX + bw / 2f, (h + bh) / 2f);

        float d = getResources().getDisplayMetrics().density;
        float cx = w / 2f, cy = h / 2f;

        if (controlConfig != null) {
            applyConfig(w, h, d);
        } else {
            applyDefaultLayout(w, h, d, cx, cy);
        }
    }

    private void applyConfig(float w, float h, float d) {
        if (controlConfig.btnA != null) {
            float acx = controlConfig.btnA.x * w;
            float acy = controlConfig.btnA.y * h;
            float r = 30 * d * controlConfig.btnA.scale;
            keyRects[KEY_A].set(acx - r, acy - r, acx + r, acy + r);
        }
        if (controlConfig.btnB != null) {
            float bcx = controlConfig.btnB.x * w;
            float bcy = controlConfig.btnB.y * h;
            float r = 30 * d * controlConfig.btnB.scale;
            keyRects[KEY_B].set(bcx - r, bcy - r, bcx + r, bcy + r);
        }
        if (controlConfig.btnStart != null) {
            float scx = controlConfig.btnStart.x * w;
            float scy = controlConfig.btnStart.y * h;
            float bw = 32 * d * controlConfig.btnStart.scale;
            float bh = 18 * d * controlConfig.btnStart.scale;
            keyRects[KEY_START].set(scx - bw, scy - bh, scx + bw, scy + bh);
        }
        if (controlConfig.btnSelect != null) {
            float scx = controlConfig.btnSelect.x * w;
            float scy = controlConfig.btnSelect.y * h;
            float bw = 32 * d * controlConfig.btnSelect.scale;
            float bh = 18 * d * controlConfig.btnSelect.scale;
            keyRects[KEY_SELECT].set(scx - bw, scy - bh, scx + bw, scy + bh);
        }
        if (controlConfig.dpad != null) {
            float dcx = controlConfig.dpad.x * w;
            float dcy = controlConfig.dpad.y * h;
            float s = controlConfig.dpad.scale;
            initDPadsWithConfig(dcx, dcy, s, d);
            
            float btnHalf = 26 * d * s;
            float dpadHalf = 78 * d * s;
            keyRects[KEY_UP].set(dcx - btnHalf, dcy - dpadHalf, dcx + btnHalf, dcy - dpadHalf + 52 * d * s);
            keyRects[KEY_DOWN].set(dcx - btnHalf, dcy + dpadHalf - 52 * d * s, dcx + btnHalf, dcy + dpadHalf);
            keyRects[KEY_LEFT].set(dcx - dpadHalf, dcy - btnHalf, dcx - dpadHalf + 52 * d * s, dcy + btnHalf);
            keyRects[KEY_RIGHT].set(dcx + dpadHalf - 52 * d * s, dcy - btnHalf, dcx + dpadHalf, dcy + btnHalf);
        }

        // Keep pause/uncap/rom buttons in default spots for now or until requested
        float actRight = w - 30 * d;
        float actLeft = actRight - 60 * d;
        pauseRect.set(actLeft, 4 * d, actRight, 30 * d);
        uncapRect.set(actLeft, 34 * d, actRight, 60 * d);
        romRect.set(w/2f - 80 * d, h/2f - 24 * d, w/2f + 80 * d, h/2f + 24 * d);
    }

    private void applyDefaultLayout(float w, float h, float d, float cx, float cy) {
        float dpadLeft = 30 * d;
        float dpadTop = cy - 78 * d;
        float dpadHalf = 78 * d;
        float btnHalf = 26 * d;
        keyRects[KEY_UP]    .set(dpadLeft + dpadHalf - btnHalf, dpadTop, dpadLeft + dpadHalf + btnHalf, dpadTop + 52 * d);
        keyRects[KEY_DOWN]  .set(dpadLeft + dpadHalf - btnHalf, dpadTop + 104 * d, dpadLeft + dpadHalf + btnHalf, dpadTop + 156 * d);
        keyRects[KEY_LEFT]  .set(dpadLeft, dpadTop + dpadHalf - btnHalf, dpadLeft + 52 * d, dpadTop + dpadHalf + btnHalf);
        keyRects[KEY_RIGHT] .set(dpadLeft + 104 * d, dpadTop + dpadHalf - btnHalf, dpadLeft + 156 * d, dpadTop + dpadHalf + btnHalf);

        float actRight = w - 30 * d;
        float actLeft = actRight - 60 * d;
        keyRects[KEY_B].set(actLeft, cy - 64 * d, actRight, cy - 64 * d + 60 * d);
        keyRects[KEY_A].set(actLeft, cy + 4 * d,  actRight, cy + 4 * d + 60 * d);

        keyRects[KEY_START]  .set(w - 94 * d, h - 66 * d, w - 30 * d, h - 30 * d);
        keyRects[KEY_SELECT].set(30 * d, h - 66 * d, 94 * d, h - 30 * d);

        pauseRect.set(actLeft, 4 * d, actRight, 30 * d);
        uncapRect.set(actLeft, 34 * d, actRight, 60 * d);

        romRect.set(cx - 80 * d, cy - 24 * d, cx + 80 * d, cy + 24 * d);

        int centerX = (int)(dstRect.left / 2f);
        initDPads((int)h, centerX);
    }

    private void initDPadsWithConfig(float cx, float cy, float scale, float d) {
        Point center = new Point((int)cx, (int)cy);
        int padH = (int)(150 * d * scale);
        int padW = (int)(180 * d * scale);

        this.dpads[0] = new DPad(center);
        this.dpads[0].buildUp(padH, padW);
        this.dpads[1] = new DPad(center);
        this.dpads[1].buildRight(padH, padW);
        this.dpads[2] = new DPad(center);
        this.dpads[2].buildLeft(padH, padW);
        this.dpads[3] = new DPad(center);
        this.dpads[3].buildDown(padH, padW);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(bitmap, null, dstRect, paint);
        drawDPads(canvas);
        drawButtons(canvas);
        drawOverlay(canvas);
    }

    private boolean isKeyPressed(int idx) {
        for (int k : pointerKey) if (k == idx) return true;
        return false;
    }

    public void drawDPads(Canvas canvas) {
        boolean[] dpadPressed = new boolean[4];
        int[] dKeyMap = {KEY_UP, KEY_RIGHT, KEY_LEFT, KEY_DOWN};
        for (int k : pointerKey) {
            if (k < 0) continue;
            for (int j = 0; j < 4; j++) {
                if (k == dKeyMap[j]) { dpadPressed[j] = true; break; }
            }
        }

        Paint fill = new Paint();
        fill.setStyle(Paint.Style.FILL);

        Paint outline = new Paint();
        outline.setStyle(Paint.Style.STROKE);
        outline.setStrokeWidth(2f);

        Paint triOutline = new Paint();
        triOutline.setStyle(Paint.Style.STROKE);
        triOutline.setStrokeWidth(2f);
        triOutline.setARGB(0xFF, 0xFF, 0xFF, 0xFF);

        for (int i = 0; i < 4; i++) {
            if (dpadPressed[i]) {
                fill.setARGB(0xE0, 0x55, 0x55, 0x55);
                outline.setARGB(0xFF, 0x99, 0x99, 0x99);
            } else {
                fill.setARGB(0xC0, 0x20, 0x20, 0x20);
                outline.setARGB(0xFF, 0x55, 0x55, 0x55);
            }
            dpads[i].draw(canvas, fill, outline);
            canvas.drawPath(dpads[i].directionPath, triOutline);
        }
    }

    private void drawButtons(Canvas canvas) {
        Paint fill = new Paint();
        fill.setStyle(Paint.Style.FILL);

        Paint outline = new Paint();
        outline.setStyle(Paint.Style.STROKE);
        outline.setStrokeWidth(2f);

        Paint text = new Paint();
        text.setColor(Color.WHITE);
        text.setTextAlign(Paint.Align.CENTER);
        text.setAntiAlias(true);

        /* A/B */
        for (int[] entry : new int[][]{{KEY_B, 0xFF888888}, {KEY_A, 0xFF888888}}) {
            int k = entry[0];
            RectF r = keyRects[k];
            boolean pressed = isKeyPressed(k);

            fill.setARGB(pressed ? 0xE0 : 0xC0, 0x20, 0x20, 0x20);
            outline.setARGB(0xFF, pressed ? 0x99 : 0x55, pressed ? 0x99 : 0x55, pressed ? 0x99 : 0x55);

            canvas.drawOval(r, fill);
            canvas.drawOval(r, outline);

            text.setTextSize(r.width() * 0.4f);
            canvas.drawText(k == KEY_B ? "B" : "A", r.centerX(), r.centerY() + text.getTextSize() * 0.35f, text);
        }

        /* Start / Select */
        for (int k : new int[]{KEY_START, KEY_SELECT}) {
            RectF r = keyRects[k];
            boolean pressed = isKeyPressed(k);

            fill.setARGB(pressed ? 0xE0 : 0xC0, 0x20, 0x20, 0x20);
            outline.setARGB(0xFF, pressed ? 0x99 : 0x55, pressed ? 0x99 : 0x55, pressed ? 0x99 : 0x55);

            canvas.drawRoundRect(r, 8f, 8f, fill);
            canvas.drawRoundRect(r, 8f, 8f, outline);

            float d = getResources().getDisplayMetrics().density;
            text.setTextSize(11 * d);
            canvas.drawText(k == KEY_START ? "Start" : "Sel", r.centerX(), r.centerY() + 4 * d, text);
        }
    }

    private void drawOverlay(Canvas canvas) {
        float d = getResources().getDisplayMetrics().density;
        float topY = 28 * d;

        /* FPS */
        Paint fpsBg = new Paint();
        fpsBg.setARGB(0x80, 0, 0, 0);
        canvas.drawRect(4 * d, topY - 12 * d, 80 * d, topY + 4 * d, fpsBg);

        Paint fpsText = new Paint();
        fpsText.setColor(0xFF00FF00);
        fpsText.setTextSize(10 * d);
        fpsText.setAntiAlias(true);
        canvas.drawText(fps + " fps  " + (paused ? "⏸" : "▶"), 6 * d, topY + 2 * d, fpsText);

        /* Pause button */
        Paint btnFill = new Paint();
        btnFill.setStyle(Paint.Style.FILL);
        btnFill.setARGB(paused ? 0xFF : 0xC0, 0x20, paused ? 0x55 : 0x20, 0x20);

        Paint btnOutline = new Paint();
        btnOutline.setStyle(Paint.Style.STROKE);
        btnOutline.setStrokeWidth(1.5f);
        btnOutline.setARGB(0xFF, 0x55, 0x55, 0x55);

        canvas.drawRoundRect(pauseRect, 4f, 4f, btnFill);
        canvas.drawRoundRect(pauseRect, 4f, 4f, btnOutline);

        Paint btnText = new Paint();
        btnText.setColor(Color.WHITE);
        btnText.setTextAlign(Paint.Align.CENTER);
        btnText.setTextSize(10 * d);
        btnText.setAntiAlias(true);
        canvas.drawText(paused ? "▶" : "⏸", pauseRect.centerX(), pauseRect.centerY() + 4 * d, btnText);

        /* Uncapped button */
        btnFill.setARGB(uncapped ? 0xFF : 0xC0, 0x20, uncapped ? 0x55 : 0x20, 0x20);
        canvas.drawRoundRect(uncapRect, 4f, 4f, btnFill);
        canvas.drawRoundRect(uncapRect, 4f, 4f, btnOutline);
        canvas.drawText(uncapped ? "∞" : "60", uncapRect.centerX(), uncapRect.centerY() + 4 * d, btnText);

        /* Load ROM button */
        if (!romLoaded) {
            btnFill.setARGB(0xC0, 0x20, 0x55, 0x20);
            canvas.drawRoundRect(romRect, 8f, 8f, btnFill);
            canvas.drawRoundRect(romRect, 8f, 8f, btnOutline);
            btnText.setTextSize(14 * d);
            canvas.drawText("Load ROM", romRect.centerX(), romRect.centerY() + 5 * d, btnText);
        }
    }

    private int findKey(float x, float y) {
        /* D-pads */
        int[] dKeyMap = {KEY_UP, KEY_RIGHT, KEY_LEFT, KEY_DOWN};
        for (int i = 0; i < 4; i++) {
            if (dpads[i].pressed((int) x, (int) y)) return dKeyMap[i];
        }
        /* A/B/Start/Select */
        for (int k : new int[]{KEY_A, KEY_B, KEY_START, KEY_SELECT}) {
            if (keyRects[k].contains(x, y)) return k;
        }
        /* Pause */
        if (pauseRect.contains(x, y)) return KEY_PAUSE;
        /* Uncapped */
        if (uncapRect.contains(x, y)) return KEY_UNCAPPED;
        if (!romLoaded && romRect.contains(x, y)) return KEY_ROM;
        return -1;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getActionMasked();
        int idx = event.getActionIndex();
        int pid = event.getPointerId(idx);
        float x = event.getX(idx);
        float y = event.getY(idx);

        switch (action) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN: {
                int found = findKey(x, y);
                if (found == KEY_PAUSE) {
                    if (pauseListener != null) {
                        pauseListener.run();
                    } else {
                        paused = !paused;
                        if (paused) NesCoreBridge.nativePauseLoop();
                        else NesCoreBridge.nativeResumeLoop();
                        invalidate();
                    }
                    break;
                }
                if (found == KEY_UNCAPPED) {
                    uncapped = !uncapped;
                    NesCoreBridge.nativeSetUncapped(uncapped);
                    invalidate();
                    break;
                }
                if (found == KEY_ROM) {
                    if (loadRomListener != null) loadRomListener.run();
                    break;
                }
                if (found >= 0) {
                    NesCoreBridge.nativeSetKey(found, 1);
                    pointerKey[pid] = found;
                }
                break;
            }
            case MotionEvent.ACTION_MOVE: {
                for (int i = 0; i < event.getPointerCount(); i++) {
                    int id = event.getPointerId(i);
                    float tx = event.getX(i);
                    float ty = event.getY(i);
                    int old = pointerKey[id];
                    int found = findKey(tx, ty);
                    if (found == KEY_PAUSE || found == KEY_UNCAPPED || found == KEY_ROM) found = -1;
                    if (found != old) {
                        if (old >= 0) NesCoreBridge.nativeSetKey(old, 0);
                        if (found >= 0) NesCoreBridge.nativeSetKey(found, 1);
                        pointerKey[id] = found;
                    }
                }
                break;
            }
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP: {
                int old = pointerKey[pid];
                if (old >= 0) {
                    NesCoreBridge.nativeSetKey(old, 0);
                    pointerKey[pid] = -1;
                }
                break;
            }
            case MotionEvent.ACTION_CANCEL: {
                for (int i = 0; i < pointerKey.length; i++) {
                    if (pointerKey[i] >= 0) {
                        NesCoreBridge.nativeSetKey(pointerKey[i], 0);
                        pointerKey[i] = -1;
                    }
                }
                break;
            }
        }
        return true;
    }

    public void setFps(int fps) {
        this.fps = fps;
    }
}
