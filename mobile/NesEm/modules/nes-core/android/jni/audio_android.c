#include <aaudio/AAudio.h>
#include "core/audio.h"
#include <android/log.h>

#define TAG "NesAudio"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static AAudioStream *g_stream = NULL;
static float g_volume = 0.3f;

static aaudio_data_callback_result_t callback(
    AAudioStream *stream,
    void *userData,
    void *audioData,
    int32_t numFrames)
{
    (void)stream;
    (void)userData;
    apu_mix_samples((float *)audioData, (unsigned int)numFrames);
    float *buf = (float *)audioData;
    for (int32_t i = 0; i < numFrames; i++) {
        buf[i] *= g_volume;
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void android_audio_init(void) {
    AAudioStreamBuilder *builder = NULL;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("createStreamBuilder failed: %d", result);
        return;
    }

    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, 44100);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setDataCallback(builder, callback, NULL);

    result = AAudioStreamBuilder_openStream(builder, &g_stream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        LOGE("openStream failed: %d", result);
        return;
    }

    AAudioStream_requestStart(g_stream);
    LOGD("AAudio stream started");
}

void android_audio_destroy(void) {
    if (g_stream) {
        AAudioStream_requestStop(g_stream);
        AAudioStream_close(g_stream);
        g_stream = NULL;
        LOGD("AAudio stream stopped");
    }
}

void android_audio_set_volume(float v) {
    g_volume = (v < 0.0f) ? 0.0f : (v > 1.0f) ? 1.0f : v;
}
