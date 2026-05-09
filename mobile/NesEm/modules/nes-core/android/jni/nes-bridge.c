#include <jni.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <android/log.h>
#include <android/bitmap.h>
#include <pthread.h>
#include <time.h>
#include "core/cpu.h"
#include "core/bus.h"
#include "core/rom_loader.h"
#include "core/ppu.h"
#include "core/audio.h"
#include "core/controller.h"
#include "core/frameData.h"

extern void android_audio_init(void);
extern void android_audio_destroy(void);
extern void android_audio_set_volume(float v);

#define LOG_TAG "NesCore"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

#define FW 256
#define FH 224
#define CLIP 8
#define FRAME_NS 16666667L

static Cartriadge *g_cartridge = NULL;
static bool g_booted = false;
static ControllerKeyStates g_ks = {0};

/* --- Native game loop state --- */
static JavaVM   *g_jvm = NULL;
static jobject   g_bitmap = NULL;
static jobject   g_view = NULL;
static jmethodID g_postInvalidate = NULL;
static pthread_t g_thread;
static volatile bool g_running = false;
static volatile bool g_paused = false;
static volatile bool g_uncapped = false;
static volatile int  g_fps = 0;

static void *game_loop(void *arg) {
    (void)arg;
    JNIEnv *env = NULL;
    (*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL);

    struct timespec t_start, t_end, fps_start;
    int frame_count = 0;
    clock_gettime(CLOCK_MONOTONIC, &fps_start);

    while (g_running) {
        clock_gettime(CLOCK_MONOTONIC, &t_start);

        if (!g_paused) {
            FrameData *frame = tick_cpu(&g_ks);
            update_apu();

            void *pixels = NULL;
            AndroidBitmapInfo info;
            if (AndroidBitmap_getInfo(env, g_bitmap, &info) >= 0 &&
                info.format == ANDROID_BITMAP_FORMAT_RGBA_8888 &&
                AndroidBitmap_lockPixels(env, g_bitmap, &pixels) >= 0) {

                uint32_t *dst = (uint32_t *)pixels;
                NesColor *src = frame->data + CLIP * FW;
                int total = FH * FW;
                for (int i = 0; i < total; i++) {
                    NesColor c = src[i];
                    dst[i] = ((uint32_t)c.a << 24) | ((uint32_t)c.b << 16) | ((uint32_t)c.g << 8) | (uint32_t)c.r;
                }
                AndroidBitmap_unlockPixels(env, g_bitmap);
            }

            if (g_view && g_postInvalidate)
                (*env)->CallVoidMethod(env, g_view, g_postInvalidate);
        }

        clock_gettime(CLOCK_MONOTONIC, &t_end);
        long elapsed = (t_end.tv_sec - t_start.tv_sec) * 1000000000L
                     + (t_end.tv_nsec - t_start.tv_nsec);
        long rem = FRAME_NS - elapsed;
        if (!g_uncapped && rem > 0) {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = rem };
            clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
        }

        frame_count++;
        if (frame_count >= 60) {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double elapsed = (now.tv_sec - fps_start.tv_sec) + (now.tv_nsec - fps_start.tv_nsec) / 1e9;
            g_fps = (int)(frame_count / elapsed);
            frame_count = 0;
            clock_gettime(CLOCK_MONOTONIC, &fps_start);
        }
    }

    (*g_jvm)->DetachCurrentThread(g_jvm);
    return NULL;
}

/* --- JNI bridge --- */

static jint nativeInit(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
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
        android_audio_init();
        boot_ppu();
        boot_cpu();
        LOGD("Boot complete");
        g_booted = true;
    }
    return 0;
}

static void nativeSetKey(JNIEnv *env, jclass clazz, jint index, jint pressed) {
    (void)env; (void)clazz;
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
    if (!result) return NULL;
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
    android_audio_destroy();
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

static void nativeSetVolume(JNIEnv *env, jclass clazz, jfloat volume) {
    (void)env; (void)clazz;
    android_audio_set_volume((float)volume);
}

static void nativeStartLoop(JNIEnv *env, jclass clazz, jobject bitmap, jobject view) {
    (void)clazz;
    if (g_running) return;

    if (g_bitmap) (*env)->DeleteGlobalRef(env, g_bitmap);
    if (g_view)   (*env)->DeleteGlobalRef(env, g_view);

    g_bitmap = (*env)->NewGlobalRef(env, bitmap);
    g_view   = (*env)->NewGlobalRef(env, view);

    jclass viewClass = (*env)->GetObjectClass(env, g_view);
    g_postInvalidate  = (*env)->GetMethodID(env, viewClass, "postInvalidate", "()V");

    g_running = true;
    g_paused  = false;
    pthread_create(&g_thread, NULL, game_loop, NULL);
    LOGD("Game loop started");
}

static void nativeStopLoop(JNIEnv *env, jclass clazz) {
    (void)clazz;
    if (!g_running) return;
    g_running = false;
    pthread_join(g_thread, NULL);

    if (g_bitmap) { (*env)->DeleteGlobalRef(env, g_bitmap); g_bitmap = NULL; }
    if (g_view)   { (*env)->DeleteGlobalRef(env, g_view);   g_view   = NULL; }
    g_postInvalidate = NULL;
    LOGD("Game loop stopped");
}

static void nativePauseLoop(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    g_paused = true;
}

static void nativeResumeLoop(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    g_paused = false;
}

static void nativeSetUncapped(JNIEnv *env, jclass clazz, jboolean uncapped) {
    (void)env; (void)clazz;
    g_uncapped = (uncapped != JNI_FALSE);
}

static jint nativeGetFps(JNIEnv *env, jclass clazz) {
    (void)env; (void)clazz;
    return g_fps;
}

static JNINativeMethod g_methods[] = {
    { "nativeInit",         "()I",         (void *)nativeInit },
    { "nativeLoadRom",      "([B)I",       (void *)nativeLoadRom },
    { "nativeSetKey",       "(II)V",       (void *)nativeSetKey },
    { "nativeGetKeys",      "()[B",        (void *)nativeGetKeys },
    { "nativeSetVolume",    "(F)V",        (void *)nativeSetVolume },
    { "nativeShutdown",     "()V",         (void *)nativeShutdown },
    { "nativeStartLoop",    "(Landroid/graphics/Bitmap;Landroid/view/View;)V", (void *)nativeStartLoop },
    { "nativeStopLoop",     "()V",         (void *)nativeStopLoop },
    { "nativePauseLoop",    "()V",         (void *)nativePauseLoop },
    { "nativeResumeLoop",   "()V",         (void *)nativeResumeLoop },
    { "nativeSetUncapped",  "(Z)V",        (void *)nativeSetUncapped },
    { "nativeGetFps",       "()I",         (void *)nativeGetFps },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)reserved;
    g_jvm = vm;

    JNIEnv *env;
    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK)
        return JNI_ERR;
    jclass clazz = (*env)->FindClass(env, "expo/modules/nescore/NesCoreBridge");
    if (!clazz) return JNI_ERR;
    if ((*env)->RegisterNatives(env, clazz, g_methods, 12) < 0)
        return JNI_ERR;
    LOGD("RegisterNatives SUCCESS via JNI_OnLoad");
    return JNI_VERSION_1_6;
}
