package com.nesem.app;

import android.graphics.Bitmap;
import android.view.View;

public class NesCoreBridge {
    public static native int nativeInit();
    public static native int nativeLoadRom(byte[] rom);
    public static native void nativeSetKey(int index, int pressed);
    public static native byte[] nativeGetKeys();
    public static native void nativeSetVolume(float volume);
    public static native void nativeShutdown();
    public static native void nativeStartLoop(Bitmap bitmap, View view);
    public static native void nativeStopLoop();
    public static native void nativePauseLoop();
    public static native void nativeResumeLoop();
    public static native void nativeSetUncapped(boolean uncapped);
    public static native int nativeGetFps();
    public static native int nativeSaveState(byte[] buffer);
    public static native int nativeLoadState(byte[] rom, byte[] state);
}
