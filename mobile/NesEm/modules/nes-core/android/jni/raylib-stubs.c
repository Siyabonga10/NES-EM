/* Raylib stubs — provides empty implementations of raylib functions
   used by the core, so the .so has zero undefined symbols. */

#include <stdlib.h>
#include <stdbool.h>

/* Audio types */
typedef struct AudioStream { int dummy; } AudioStream;
typedef void (*AudioCallback)(void*, unsigned int);

/* Color for DrawRectangle */
typedef struct { unsigned char r, g, b, a; } Color;

/* Vector2 for DrawLineEx */
typedef struct { float x, y; } Vector2;

/* Stubs */
void ClearBackground(Color c) {}
void DrawRectangle(int x, int y, int w, int h, Color c) {}
void DrawRectangleLines(int x, int y, int w, int h, Color c) {}
void DrawLineEx(Vector2 a, Vector2 b, float t, Color c) {}
void DrawText(const char *s, int x, int y, int siz, Color c) {}
int GetScreenWidth(void) { return 1024; }
bool IsKeyPressed(int key) { return false; }
int MeasureText(const char *s, int siz) { return 0; }
const char *TextFormat(const char *fmt, ...) { return ""; }

AudioStream LoadAudioStream(unsigned int rate, unsigned int bits, unsigned int ch) {
    AudioStream s = {0}; return s;
}
void SetAudioStreamCallback(AudioStream s, AudioCallback cb) {}
void SetMasterVolume(float v) {}
void PlayAudioStream(AudioStream s) {}
