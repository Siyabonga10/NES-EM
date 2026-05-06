package expo.modules.nescore;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.View;

public class NesEmView extends View {
    public static final int GAME_W = 256;
    public static final int GAME_H = 224;

    private final Bitmap bitmap;
    private final Paint paint;
    private final RectF srcRect;
    private final RectF dstRect;

    public NesEmView(Context context) {
        super(context);
        bitmap = Bitmap.createBitmap(GAME_W, GAME_H, Bitmap.Config.ARGB_8888);
        paint = new Paint(Paint.FILTER_BITMAP_FLAG);
        srcRect = new RectF(0, 0, GAME_W, GAME_H);
        dstRect = new RectF();
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
        float scale = Math.min((float) w / GAME_W, (float) h / GAME_H);
        float bw = GAME_W * scale;
        float bh = GAME_H * scale;
        dstRect.set((w - bw) / 2f, (h - bh) / 2f, (w + bw) / 2f, (h + bh) / 2f);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        canvas.drawBitmap(bitmap, null, dstRect, paint);
    }
}
