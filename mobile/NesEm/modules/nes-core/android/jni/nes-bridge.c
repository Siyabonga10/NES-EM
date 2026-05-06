#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <android/log.h>
#include <android/bitmap.h>
#include "core/cpu.h"
#include "core/bus.h"
#include "core/cartriadge.h"
#include "core/ppu.h"
#include "core/audio.h"
#include "core/controller.h"
#include "core/frameData.h"

#define LOG_TAG "NesCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define FW 256
#define FH 224
#define CLIP 8

static Cartriadge *g_cartridge = NULL;
static bool g_booted = false;

static jint nativeInit(JNIEnv *env, jclass clazz) {
    return 0;
}

static jint nativeLoadRom(JNIEnv *env, jclass clazz, jbyteArray rom) {
    jsize len = (*env)->GetArrayLength(env, rom);
    jbyte *buf = (*env)->GetByteArrayElements(env, rom, NULL);

    if (g_cartridge) {
        free(g_cartridge->pg_rom); free(g_cartridge->ch_rom);
        free(g_cartridge->chr_ram); free(g_cartridge->prg_ram);
        free(g_cartridge);
        g_cartridge = NULL;
    }

    g_cartridge = (Cartriadge *)calloc(1, sizeof(Cartriadge));
    int rc = load_cartridge_from_memory((unsigned char *)buf, len, g_cartridge);
    (*env)->ReleaseByteArrayElements(env, rom, buf, JNI_ABORT);

    if (rc != 0) return rc;

    if (!g_booted) {
        connect_controller_to_console();
        boot_nes_audio();
        boot_ppu();
        boot_cpu();
        LOGD("Boot complete");
        g_booted = true;
    }
    return 0;
}

static jbyteArray nativeTick(JNIEnv *env, jclass clazz, jbyteArray keys) {
    static ControllerKeyStates ks;
    jbyte *kbuf = (*env)->GetByteArrayElements(env, keys, NULL);
    ks.a_pressed     = kbuf[0]; ks.b_pressed      = kbuf[1];
    ks.up_pressed    = kbuf[2]; ks.down_pressed    = kbuf[3];
    ks.left_pressed  = kbuf[4]; ks.right_pressed   = kbuf[5];
    ks.start_pressed = kbuf[6]; ks.select_pressed  = kbuf[7];
    (*env)->ReleaseByteArrayElements(env, keys, kbuf, JNI_ABORT);

    tick_cpu(&ks);
    update_apu();
    return NULL;
}

static jbyteArray nativeTickRender(JNIEnv *env, jclass clazz, jbyteArray keys, jobject bitmap) {
    static ControllerKeyStates ks;
    jbyte *kbuf = (*env)->GetByteArrayElements(env, keys, NULL);
    ks.a_pressed     = kbuf[0]; ks.b_pressed      = kbuf[1];
    ks.up_pressed    = kbuf[2]; ks.down_pressed    = kbuf[3];
    ks.left_pressed  = kbuf[4]; ks.right_pressed   = kbuf[5];
    ks.start_pressed = kbuf[6]; ks.select_pressed  = kbuf[7];
    (*env)->ReleaseByteArrayElements(env, keys, kbuf, JNI_ABORT);

    FrameData *frame = tick_cpu(&ks);
    update_apu();

    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) return NULL;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) return NULL;

    void *pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return NULL;

    uint32_t *dst = (uint32_t *)pixels;
    for (int y = 0; y < FH; y++) {
        for (int x = 0; x < FW; x++) {
            int si = (y + CLIP) * FW + x;
            int di = y * FW + x;
            NesColor c = frame->data[si];
            dst[di] = ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.r;
        }
    }

    AndroidBitmap_unlockPixels(env, bitmap);
    return NULL;
}

static void nativeShutdown(JNIEnv *env, jclass clazz) {
    shutdown_cpu();
    kill_ppu();
    if (g_cartridge) {
        free(g_cartridge->pg_rom); free(g_cartridge->ch_rom);
        free(g_cartridge->chr_ram); free(g_cartridge->prg_ram);
        free(g_cartridge);
        g_cartridge = NULL;
    }
    g_booted = false;
}

static JNINativeMethod g_methods[] = {
    { "nativeInit",       "()I",      (void *)nativeInit },
    { "nativeLoadRom",    "([B)I",    (void *)nativeLoadRom },
    { "nativeTick",       "([B)[B",   (void *)nativeTick },
    { "nativeTickRender", "([BLandroid/graphics/Bitmap;)[B", (void *)nativeTickRender },
    { "nativeShutdown",   "()V",      (void *)nativeShutdown },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;
    jclass clazz = (*env)->FindClass(env, "expo/modules/nescore/NesCoreBridge");
    if (!clazz) return JNI_ERR;
    if ((*env)->RegisterNatives(env, clazz, g_methods, 5) < 0)
        return JNI_ERR;
    LOGD("RegisterNatives SUCCESS via JNI_OnLoad");
    return JNI_VERSION_1_6;
}
