package expo.modules.nescore;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
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
    private DPad[] dpads = new DPad[4];

    private final Bitmap bitmap;
    private final Paint paint;
    private final RectF srcRect;
    private final RectF dstRect;

    /* Hit rects — recalculated on size change */
    private final RectF[] keyRects = new RectF[8];
    private final int[] pointerKey = new int[20]; /* finger id → key index, -1 = none */

    public NesEmView(Context context) {
        super(context);
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

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();
        NesCoreModule.Companion.setCurrentView(this);
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        NesCoreModule.Companion.setCurrentView(null);
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);

        /* Game area — float scaling, nearest-neighbor sharp */
        float scale = Math.min((float) w / GAME_W, (float) h / GAME_H);
        float bw = GAME_W * scale;
        float bh = GAME_H * scale;
        float dstCenterX = w / 2f + (w - bw) * 0.10f;
        dstRect.set(dstCenterX - bw / 2f, (h - bh) / 2f, dstCenterX + bw / 2f, (h + bh) / 2f);

        /* Hit rects — match React Native layout (dp -> px) */
        float d = getResources().getDisplayMetrics().density;
        float cx = w / 2f, cy = h / 2f;

        /* D-pad: left 30dp, vertically centered, 156dp square */
        float dpadLeft = 30 * d;
        float dpadTop = cy - 78 * d;
        float dpadHalf = 78 * d;
        float btnHalf = 26 * d;
        keyRects[KEY_UP]    .set(dpadLeft + dpadHalf - btnHalf, dpadTop, dpadLeft + dpadHalf + btnHalf, dpadTop + 52 * d);
        keyRects[KEY_DOWN]  .set(dpadLeft + dpadHalf - btnHalf, dpadTop + 104 * d, dpadLeft + dpadHalf + btnHalf, dpadTop + 156 * d);
        keyRects[KEY_LEFT]  .set(dpadLeft, dpadTop + dpadHalf - btnHalf, dpadLeft + 52 * d, dpadTop + dpadHalf + btnHalf);
        keyRects[KEY_RIGHT] .set(dpadLeft + 104 * d, dpadTop + dpadHalf - btnHalf, dpadLeft + 156 * d, dpadTop + dpadHalf + btnHalf);

        /* A/B: right 30dp, vertically centered, 60dp buttons, 8dp gap. B on top, A below */
        float actRight = w - 30 * d;
        float actLeft = actRight - 60 * d;
        keyRects[KEY_B].set(actLeft, cy - 64 * d, actRight, cy - 64 * d + 60 * d);
        keyRects[KEY_A].set(actLeft, cy + 4 * d,  actRight, cy + 4 * d + 60 * d);

        /* Start: right 30dp, bottom 30dp, 64x36dp */
        keyRects[KEY_START]  .set(w - 94 * d, h - 66 * d, w - 30 * d, h - 30 * d);
        /* Select: left 30dp, bottom 30dp, 64x36dp */
        keyRects[KEY_SELECT].set(30 * d, h - 66 * d, 94 * d, h - 30 * d);

        int centerX = (int)(dstRect.left / 2f);
        initDPads(h, centerX);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(bitmap, null, dstRect, paint);
        drawDPads(canvas);
    }

    public void drawDPads(Canvas canvas) {
        boolean[] dpadPressed = new boolean[4];
        int[] dKeyMap = {KEY_UP, KEY_RIGHT, KEY_LEFT, KEY_DOWN};
        for (int i = 0; i < pointerKey.length; i++) {
            int k = pointerKey[i];
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

    private int findKey(float x, float y) {
        int[] dKeyMap = {KEY_UP, KEY_RIGHT, KEY_LEFT, KEY_DOWN};
        for (int i = 0; i < 4; i++) {
            if (dpads[i].pressed((int) x, (int) y)) return dKeyMap[i];
        }
        for (int k : new int[]{KEY_A, KEY_B, KEY_START, KEY_SELECT}) {
            if (keyRects[k].contains(x, y)) return k;
        }
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
}
