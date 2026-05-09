package expo.modules.nescore;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Point;

public class DPad {
    public final Path path;
    public final Path directionPath = new Path();
    public final Point center;
    public final Point[] points = new Point[5];
    public boolean pressed;

    public DPad(Point center) {
        this.center = center;
        this.path = new Path();
        for (int i = 0; i < 5; i++) points[i] = new Point();
    }

    public void buildUp(int h, int w) {
        int d = w / 2;
        path.moveTo(center.x, center.y);
        points[0].set(center.x, center.y);

        path.rLineTo(-d, -d);
        points[1].set(center.x - d, center.y - d);

        path.rLineTo(0, -h);
        points[2].set(center.x - d, center.y - d - h);

        path.rLineTo(w, 0);
        points[3].set(center.x - d + w, center.y - d - h);

        path.rLineTo(0, h);
        points[4].set(center.x - d + w, center.y - d);

        path.close();

        int triW = w / 4;
        int triHt = h / 5;
        int bodyCenterY = center.y - d - h / 2;
        directionPath.moveTo(center.x, bodyCenterY - triHt);
        directionPath.lineTo(center.x - triW, bodyCenterY + triHt / 2);
        directionPath.lineTo(center.x + triW, bodyCenterY + triHt / 2);
        directionPath.close();
    }

    public void buildDown(int h, int w) {
        int d = w / 2;
        path.moveTo(center.x, center.y);
        points[0].set(center.x, center.y);

        path.rLineTo(-d, d);
        points[1].set(center.x - d, center.y + d);

        path.rLineTo(0, h);
        points[2].set(center.x - d, center.y + d + h);

        path.rLineTo(w, 0);
        points[3].set(center.x - d + w, center.y + d + h);

        path.rLineTo(0, -h);
        points[4].set(center.x - d + w, center.y + d);

        path.close();

        int triW = w / 4;
        int triHt = h / 5;
        int bodyCenterY = center.y + d + h / 2;
        directionPath.moveTo(center.x, bodyCenterY + triHt);
        directionPath.lineTo(center.x - triW, bodyCenterY - triHt / 2);
        directionPath.lineTo(center.x + triW, bodyCenterY - triHt / 2);
        directionPath.close();
    }

    public void buildLeft(int h, int w) {
        int d = w / 2;
        path.moveTo(center.x, center.y);
        points[0].set(center.x, center.y);

        path.rLineTo(-d, -d);
        points[1].set(center.x - d, center.y - d);

        path.rLineTo(-h, 0);
        points[2].set(center.x - d - h, center.y - d);

        path.rLineTo(0, w);
        points[3].set(center.x - d - h, center.y - d + w);

        path.rLineTo(h, 0);
        points[4].set(center.x - d, center.y - d + w);

        path.close();

        int triW = w / 4;
        int triHt = h / 5;
        int bodyCenterX = center.x - d - h / 2;
        directionPath.moveTo(bodyCenterX - triHt, center.y);
        directionPath.lineTo(bodyCenterX + triHt / 2, center.y - triW);
        directionPath.lineTo(bodyCenterX + triHt / 2, center.y + triW);
        directionPath.close();
    }

    public void buildRight(int h, int w) {
        int d = w / 2;
        path.moveTo(center.x, center.y);
        points[0].set(center.x, center.y);

        path.rLineTo(d, -d);
        points[1].set(center.x + d, center.y - d);

        path.rLineTo(h, 0);
        points[2].set(center.x + d + h, center.y - d);

        path.rLineTo(0, w);
        points[3].set(center.x + d + h, center.y - d + w);

        path.rLineTo(-h, 0);
        points[4].set(center.x + d, center.y - d + w);

        path.close();

        int triW = w / 4;
        int triHt = h / 5;
        int bodyCenterX = center.x + d + h / 2;
        directionPath.moveTo(bodyCenterX + triHt, center.y);
        directionPath.lineTo(bodyCenterX - triHt / 2, center.y - triW);
        directionPath.lineTo(bodyCenterX - triHt / 2, center.y + triW);
        directionPath.close();
    }

    public boolean pressed(int x, int y) {
        float dx = x - center.x;
        float dy = y - center.y;

        float maxRadiusSq = 0;
        for (Point p : points) {
            float rdx = p.x - center.x;
            float rdy = p.y - center.y;
            float distSq = rdx * rdx + rdy * rdy;
            if (distSq > maxRadiusSq) maxRadiusSq = distSq;
        }
        if (dx * dx + dy * dy > maxRadiusSq) return false;

        boolean inside = false;
        for (int i = 0, j = 4; i < 5; j = i++) {
            float xi = points[i].x, yi = points[i].y;
            float xj = points[j].x, yj = points[j].y;
            if ((yi > y) != (yj > y) && x < (xj - xi) * (y - yi) / (yj - yi) + xi) {
                inside = !inside;
            }
        }
        return inside;
    }

    public void draw(Canvas canvas, Paint fill, Paint outline) {
        canvas.drawPath(path, fill);
        canvas.drawPath(path, outline);
    }
}
