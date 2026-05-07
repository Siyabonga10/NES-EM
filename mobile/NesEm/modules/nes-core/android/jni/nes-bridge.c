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
static ControllerKeyStates g_ks = {0};

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
    (void)keys;
    tick_cpu(&g_ks);
    update_apu();
    return NULL;
}

static jbyteArray nativeTickRender(JNIEnv *env, jclass clazz, jbyteArray keys, jobject bitmap) {
    (void)keys;
    FrameData *frame = tick_cpu(&g_ks);
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

static void nativeSetKey(JNIEnv *env, jclass clazz, jint index, jint pressed) {
    if (index < 0 || index > 7) return;
    unsigned char val = pressed ? 1 : 0;
    switch (index) {
        case 0: g_ks.a_pressed = val; break;
        case 1: g_ks.b_pressed = val; break;
        case 2: g_ks.up_pressed = val; break;
        case 3: g_ks.down_pressed = val; break;
        case 4: g_ks.left_pressed = val; break;
        case 5: g_ks.right_pressed = val; break;
        case 6: g_ks.start_pressed = val; break;
        case 7: g_ks.select_pressed = val; break;
    }
}

static jbyteArray nativeGetKeys(JNIEnv *env, jclass clazz) {
    jbyteArray result = (*env)->NewByteArray(env, 8);
    jbyte *out = (*env)->GetByteArrayElements(env, result, NULL);
    out[0] = g_ks.a_pressed;
    out[1] = g_ks.b_pressed;
    out[2] = g_ks.up_pressed;
    out[3] = g_ks.down_pressed;
    out[4] = g_ks.left_pressed;
    out[5] = g_ks.right_pressed;
    out[6] = g_ks.start_pressed;
    out[7] = g_ks.select_pressed;
    (*env)->ReleaseByteArrayElements(env, result, out, 0);
    return result;
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
    memset(&g_ks, 0, sizeof(g_ks));
}

static JNINativeMethod g_methods[] = {
    { "nativeInit",       "()I",      (void *)nativeInit },
    { "nativeLoadRom",    "([B)I",    (void *)nativeLoadRom },
    { "nativeTick",       "([B)[B",   (void *)nativeTick },
    { "nativeTickRender", "([BLandroid/graphics/Bitmap;)[B", (void *)nativeTickRender },
    { "nativeSetKey",     "(II)V",    (void *)nativeSetKey },
    { "nativeGetKeys",    "()[B",     (void *)nativeGetKeys },
    { "nativeShutdown",   "()V",      (void *)nativeShutdown },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;
    jclass clazz = (*env)->FindClass(env, "expo/modules/nescore/NesCoreBridge");
    if (!clazz) return JNI_ERR;
    if ((*env)->RegisterNatives(env, clazz, g_methods, 7) < 0)
        return JNI_ERR;
    LOGD("RegisterNatives SUCCESS via JNI_OnLoad");
    return JNI_VERSION_1_6;
}
