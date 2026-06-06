package com.nesem.app;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class LayoutEditorView extends View {
    private ControlConfig config;
    private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint outlinePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint textPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final DPad[] editorDpads = new DPad[4];

    public enum ComponentType { NONE, DPAD, BTN_A, BTN_B, BTN_START, BTN_SELECT }
    private ComponentType selectedComponent = ComponentType.NONE;
    private OnComponentSelectedListener listener;

    public interface OnComponentSelectedListener {
        void onComponentSelected(ComponentType type, float scale);
    }

    public LayoutEditorView(Context context, AttributeSet attrs) {
        super(context, attrs);
        outlinePaint.setStyle(Paint.Style.STROKE);
        outlinePaint.setStrokeWidth(2f);
        outlinePaint.setColor(Color.WHITE);
        textPaint.setColor(Color.WHITE);
        textPaint.setTextAlign(Paint.Align.CENTER);
    }

    public void setConfig(ControlConfig config) {
        this.config = config;
        invalidate();
    }

    public void setListener(OnComponentSelectedListener listener) {
        this.listener = listener;
    }

    public ComponentType getSelectedType() {
        return selectedComponent;
    }

    public void setScale(float scale) {
        if (config == null || selectedComponent == ComponentType.NONE) return;
        ControlConfig.Component comp = getComponent(selectedComponent);
        if (comp != null) {
            comp.scale = scale;
            invalidate();
        }
    }

    private ControlConfig.Component getComponent(ComponentType type) {
        if (config == null) return null;
        switch (type) {
            case DPAD: return config.dpad;
            case BTN_A: return config.btnA;
            case BTN_B: return config.btnB;
            case BTN_START: return config.btnStart;
            case BTN_SELECT: return config.btnSelect;
            default: return null;
        }
    }

    @Override
    protected void onDraw(Canvas canvas) {
        if (config == null) return;
        float w = getWidth();
        float h = getHeight();
        float d = getResources().getDisplayMetrics().density;

        drawDpad(canvas, config.dpad, w, h, d);
        drawButton(canvas, config.btnA, "A", w, h, d, ComponentType.BTN_A);
        drawButton(canvas, config.btnB, "B", w, h, d, ComponentType.BTN_B);
        drawRectButton(canvas, config.btnStart, "Start", w, h, d, ComponentType.BTN_START);
        drawRectButton(canvas, config.btnSelect, "Select", w, h, d, ComponentType.BTN_SELECT);
    }

    private void drawDpad(Canvas canvas, ControlConfig.Component comp, float w, float h, float d) {
        if (comp == null) return;
        float cx = comp.x * w;
        float cy = comp.y * h;

        int padH = (int)(150 * comp.scale);
        int padW = (int)(180 * comp.scale);
        
        android.graphics.Point center = new android.graphics.Point((int)cx, (int)cy);
        for (int i = 0; i < 4; i++) {
            if (editorDpads[i] == null) {
                editorDpads[i] = new DPad(center);
            } else {
                editorDpads[i].center.set((int)cx, (int)cy);
                editorDpads[i].path.reset();
                editorDpads[i].directionPath.reset();
            }
        }

        editorDpads[0].buildUp(padH, padW);
        editorDpads[1].buildRight(padH, padW);
        editorDpads[2].buildLeft(padH, padW);
        editorDpads[3].buildDown(padH, padW);

        Paint fill = new Paint();
        fill.setARGB(selectedComponent == ComponentType.DPAD ? 0x80 : 0x40, 0x55, 0x55, 0x55);
        
        Paint outline = new Paint(outlinePaint);
        outline.setARGB(0xFF, 0x99, 0x99, 0x99);

        Paint triOutline = new Paint();
        triOutline.setStyle(Paint.Style.STROKE);
        triOutline.setStrokeWidth(2f);
        triOutline.setColor(Color.WHITE);

        for (int i = 0; i < 4; i++) {
            editorDpads[i].draw(canvas, fill, outline);
            canvas.drawPath(editorDpads[i].directionPath, triOutline);
        }
    }

    private void drawButton(Canvas canvas, ControlConfig.Component comp, String label, float w, float h, float d, ComponentType type) {
        if (comp == null) return;
        float cx = comp.x * w;
        float cy = comp.y * h;
        float radius = 30 * d * comp.scale;

        paint.setARGB(selectedComponent == type ? 0x80 : 0x40, 0x88, 0x88, 0x88);
        canvas.drawCircle(cx, cy, radius, paint);
        canvas.drawCircle(cx, cy, radius, outlinePaint);

        textPaint.setTextSize(radius * 0.8f);
        canvas.drawText(label, cx, cy + textPaint.getTextSize() * 0.35f, textPaint);
    }

    private void drawRectButton(Canvas canvas, ControlConfig.Component comp, String label, float w, float h, float d, ComponentType type) {
        if (comp == null) return;
        float cx = comp.x * w;
        float cy = comp.y * h;
        float bw = 64 * d * comp.scale;
        float bh = 36 * d * comp.scale;

        paint.setARGB(selectedComponent == type ? 0x80 : 0x40, 0x88, 0x88, 0x88);
        canvas.drawRoundRect(cx - bw/2, cy - bh/2, cx + bw/2, cy + bh/2, 8 * d, 8 * d, paint);
        canvas.drawRoundRect(cx - bw/2, cy - bh/2, cx + bw/2, cy + bh/2, 8 * d, 8 * d, outlinePaint);

        textPaint.setTextSize(11 * d * comp.scale);
        canvas.drawText(label, cx, cy + 4 * d * comp.scale, textPaint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();
        float w = getWidth();
        float h = getHeight();

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                selectedComponent = findComponent(x, y, w, h);
                if (selectedComponent != ComponentType.NONE && listener != null) {
                    listener.onComponentSelected(selectedComponent, getComponent(selectedComponent).scale);
                }
                invalidate();
                return true;
            case MotionEvent.ACTION_MOVE:
                if (selectedComponent != ComponentType.NONE) {
                    ControlConfig.Component comp = getComponent(selectedComponent);
                    if (comp != null) {
                        comp.x = x / w;
                        comp.y = y / h;
                        invalidate();
                    }
                }
                return true;
        }
        return super.onTouchEvent(event);
    }

    private ComponentType findComponent(float x, float y, float w, float h) {
        float d = getResources().getDisplayMetrics().density;
        
        // Check buttons in reverse order of drawing or specific order
        if (hitTestCircle(x, y, config.btnA, w, h, 30 * d)) return ComponentType.BTN_A;
        if (hitTestCircle(x, y, config.btnB, w, h, 30 * d)) return ComponentType.BTN_B;
        if (hitTestRect(x, y, config.btnStart, w, h, 64 * d, 36 * d)) return ComponentType.BTN_START;
        if (hitTestRect(x, y, config.btnSelect, w, h, 64 * d, 36 * d)) return ComponentType.BTN_SELECT;
        if (hitTestRect(x, y, config.dpad, w, h, 156 * d, 156 * d)) return ComponentType.DPAD;

        return ComponentType.NONE;
    }

    private boolean hitTestCircle(float x, float y, ControlConfig.Component comp, float w, float h, float radius) {
        if (comp == null) return false;
        float cx = comp.x * w;
        float cy = comp.y * h;
        float r = radius * comp.scale;
        return Math.pow(x - cx, 2) + Math.pow(y - cy, 2) <= Math.pow(r, 2);
    }

    private boolean hitTestRect(float x, float y, ControlConfig.Component comp, float w, float h, float rw, float rh) {
        if (comp == null) return false;
        float cx = comp.x * w;
        float cy = comp.y * h;
        float halfW = (rw * comp.scale) / 2;
        float halfH = (rh * comp.scale) / 2;
        return x >= cx - halfW && x <= cx + halfW && y >= cy - halfH && y <= cy + halfH;
    }
}
