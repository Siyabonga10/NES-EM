package expo.modules.nescore;

import android.graphics.Bitmap;

public class NesCoreBridge {
    public static native int nativeInit();
    public static native int nativeLoadRom(byte[] rom);
    public static native byte[] nativeTick(byte[] keys);
    public static native byte[] nativeTickRender(byte[] keys, Bitmap bitmap);
    public static native void nativeSetVolume(float volume);
    public static native void nativeShutdown();
    public static native void nativeSetKey(int index, int pressed);
    public static native byte[] nativeGetKeys();
}
