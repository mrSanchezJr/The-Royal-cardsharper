#include "audio.h"
#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define AUDIO_SR 22050

typedef enum { OSC_SINE, OSC_TRI, OSC_SAW } OscType;

static Sound g_sfx[SFX_COUNT];
static Music g_music;
static unsigned char* g_music_mem = NULL;
static int g_music_mem_size = 0;
static bool g_ready = false;
static float g_music_vol = 0.7f;
static float g_sfx_vol = 0.7f;

static void AddTone(float* buf, int n, float start, float dur,
                    float f0, float f1, float amp, OscType osc, float decay)
{
    int s0 = (int)(start * AUDIO_SR);
    int len = (int)(dur * AUDIO_SR);
    float phase = 0.0f;
    for (int i = 0; i < len; i++) {
        int idx = s0 + i;
        if (idx >= n) break;
        float t = (float)i / AUDIO_SR;
        float k = (float)i / len;
        float f = f0 + (f1 - f0) * k;
        phase += 2.0f * PI * f / AUDIO_SR;
        float p = fmodf(phase, 2.0f * PI) / (2.0f * PI);
        float v;
        if (osc == OSC_TRI)      v = (p < 0.25f) ? (4.0f * p) : (p < 0.75f) ? (2.0f - 4.0f * p) : (4.0f * p - 4.0f);
        else if (osc == OSC_SAW) v = 2.0f * p - 1.0f;
        else                     v = sinf(phase);
        float env = ((t < 0.005f) ? (t / 0.005f) : 1.0f) * expf(-decay * t);
        buf[idx] += v * amp * env;
    }
}

static void AddNoise(float* buf, int n, float start, float dur, float amp, int smooth, float decay)
{
    unsigned int seed = (unsigned int)(start * 1000.0f) + 12345u;
    int s0 = (int)(start * AUDIO_SR);
    int len = (int)(dur * AUDIO_SR);
    float prev = 0.0f;
    for (int i = 0; i < len; i++) {
        int idx = s0 + i;
        if (idx >= n) break;
        seed = seed * 1664525u + 1013904223u;
        float white = ((seed >> 9) / 8388608.0f) - 1.0f;
        float t = (float)i / AUDIO_SR;
        prev = prev + 0.35f * (white - prev);
        float v = smooth ? prev : white;
        float env = expf(-decay * t);
        buf[idx] += v * amp * env;
    }
}

static short* RenderToPcm(const float* mix, int n)
{
    short* pcm = (short*)malloc((size_t)n * sizeof(short));
    if (!pcm) return NULL;
    for (int i = 0; i < n; i++) {
        float v = mix[i];
        if (v > 1.0f) v = 1.0f;
        if (v < -1.0f) v = -1.0f;
        pcm[i] = (short)(v * 32767.0f);
    }
    return pcm;
}

static Sound MakeSoundFromPcm(short* pcm, int n)
{
    Wave w = { 0 };
    w.frameCount = n;
    w.sampleRate = AUDIO_SR;
    w.sampleSize = 16;
    w.channels = 1;
    w.data = pcm;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

static void BuildMusic(void)
{
    const float beat = 0.625f;
    const int beats = 16;
    const float loopDur = beats * beat;
    int n = (int)(loopDur * AUDIO_SR);

    float* mix = (float*)calloc((size_t)n, sizeof(float));
    if (!mix) return;

    static const float bassRoots[4] = { 110.0f, 87.31f, 65.41f, 98.0f };
    static const int mel[16] = { 3, -1, -1, 0, 1, -1, 2, -1, 3, 4, -1, 3, 2, -1, 0, -1 };
    static const float pent[5] = { 440.0f, 523.25f, 587.33f, 659.26f, 783.99f };
    static const float arpRatio[4] = { 1.0f, 1.5f, 2.0f, 1.5f };

    for (int c = 0; c < 4; c++) {
        float chordStart = c * 4 * beat;
        AddTone(mix, n, chordStart, 4 * beat, bassRoots[c], bassRoots[c], 0.20f, OSC_SINE, 0.9f);
        AddTone(mix, n, chordStart + 2 * beat, 2 * beat, bassRoots[c], bassRoots[c], 0.14f, OSC_SINE, 1.6f);
        for (int k = 0; k < 8; k++) {
            float f = bassRoots[c] * 2.0f * arpRatio[k % 4];
            AddTone(mix, n, chordStart + k * beat * 0.5f, beat * 0.55f, f, f, 0.13f, OSC_TRI, 7.0f);
        }
    }

    for (int b = 0; b < beats; b++) {
        if (mel[b] >= 0) {
            AddTone(mix, n, b * beat, beat * 1.2f, pent[mel[b]], pent[mel[b]], 0.11f, OSC_TRI, 4.0f);
        }
        AddNoise(mix, n, b * beat, 0.03f, (b % 4 == 0) ? 0.05f : 0.03f, 1, 60.0f);
    }

    short* pcm = RenderToPcm(mix, n);
    free(mix);
    if (!pcm) return;

    int dataLen = n * 2;
    g_music_mem_size = 44 + dataLen;
    g_music_mem = (unsigned char*)malloc((size_t)g_music_mem_size);
    if (!g_music_mem) { free(pcm); return; }

    unsigned char* m = g_music_mem;
    memcpy(m + 0, "RIFF", 4);
    *(unsigned int*)(m + 4) = (unsigned int)(g_music_mem_size - 8);
    memcpy(m + 8, "WAVE", 4);
    memcpy(m + 12, "fmt ", 4);
    *(unsigned int*)(m + 16) = 16;
    *(unsigned short*)(m + 20) = 1;
    *(unsigned short*)(m + 22) = 1;
    *(unsigned int*)(m + 24) = AUDIO_SR;
    *(unsigned int*)(m + 28) = AUDIO_SR * 2;
    *(unsigned short*)(m + 32) = 2;
    *(unsigned short*)(m + 34) = 16;
    memcpy(m + 36, "data", 4);
    *(unsigned int*)(m + 40) = (unsigned int)dataLen;
    memcpy(m + 44, pcm, (size_t)dataLen);
    free(pcm);
}

void InitGameAudio(void)
{
    InitAudioDevice();
    g_ready = IsAudioDeviceReady();
    if (!g_ready) return;

    for (int i = 0; i < SFX_COUNT; i++) g_sfx[i] = (Sound){ 0 };

    {
        int n = (int)(0.12f * AUDIO_SR);
        float* mix = (float*)calloc((size_t)n, sizeof(float));
        if (mix) {
            AddTone(mix, n, 0.0f, 0.09f, 190.0f, 110.0f, 0.45f, OSC_SINE, 30.0f);
            AddNoise(mix, n, 0.0f, 0.04f, 0.40f, 0, 70.0f);
            short* pcm = RenderToPcm(mix, n);
            free(mix);
            if (pcm) g_sfx[SFX_CARD_PLAY] = MakeSoundFromPcm(pcm, n);
        }
    }
    {
        int n = (int)(0.30f * AUDIO_SR);
        float* mix = (float*)calloc((size_t)n, sizeof(float));
        if (mix) {
            AddNoise(mix, n, 0.0f, 0.30f, 0.38f, 1, 9.0f);
            AddTone(mix, n, 0.0f, 0.25f, 320.0f, 140.0f, 0.10f, OSC_SINE, 12.0f);
            short* pcm = RenderToPcm(mix, n);
            free(mix);
            if (pcm) g_sfx[SFX_CARD_TAKE] = MakeSoundFromPcm(pcm, n);
        }
    }
    {
        int n = (int)(0.45f * AUDIO_SR);
        float* mix = (float*)calloc((size_t)n, sizeof(float));
        if (mix) {
            for (int k = 0; k < 6; k++) {
                float t0 = k * 0.06f;
                AddTone(mix, n, t0, 0.07f, 300.0f + k * 110.0f, 340.0f + k * 120.0f, 0.22f, OSC_SINE, 14.0f);
                AddNoise(mix, n, t0, 0.05f, 0.08f, 1, 40.0f);
            }
            short* pcm = RenderToPcm(mix, n);
            free(mix);
            if (pcm) g_sfx[SFX_CHEAT_SWAP] = MakeSoundFromPcm(pcm, n);
        }
    }
    {
        int n = (int)(0.80f * AUDIO_SR);
        float* mix = (float*)calloc((size_t)n, sizeof(float));
        if (mix) {
            AddTone(mix, n, 0.0f, 0.75f, 233.08f, 116.54f, 0.30f, OSC_SAW, 3.5f);
            AddTone(mix, n, 0.0f, 0.75f, 220.0f, 110.0f, 0.30f, OSC_SAW, 3.5f);
            AddTone(mix, n, 0.02f, 0.40f, 466.16f, 233.08f, 0.15f, OSC_SAW, 5.0f);
            short* pcm = RenderToPcm(mix, n);
            free(mix);
            if (pcm) g_sfx[SFX_CAUGHT] = MakeSoundFromPcm(pcm, n);
        }
    }
    {
        int n = (int)(0.60f * AUDIO_SR);
        float* mix = (float*)calloc((size_t)n, sizeof(float));
        if (mix) {
            AddTone(mix, n, 0.00f, 0.16f, 440.0f,  440.0f, 0.28f, OSC_TRI, 7.0f);
            AddTone(mix, n, 0.12f, 0.18f, 659.26f, 659.26f, 0.28f, OSC_TRI, 7.0f);
            AddTone(mix, n, 0.26f, 0.34f, 880.0f,  880.0f,  0.30f, OSC_TRI, 5.0f);
            AddTone(mix, n, 0.26f, 0.34f, 440.0f,  440.0f,  0.18f, OSC_SINE, 5.0f);
            short* pcm = RenderToPcm(mix, n);
            free(mix);
            if (pcm) g_sfx[SFX_ROUND_END] = MakeSoundFromPcm(pcm, n);
        }
    }

    BuildMusic();
    g_music = LoadMusicStreamFromMemory(".wav", g_music_mem, g_music_mem_size);
    g_music.looping = true;
    PlayMusicStream(g_music);
    ApplyAudioVolumes(g_music_vol, g_sfx_vol);
}

void CloseGameAudio(void)
{
    if (!g_ready) return;
    UnloadMusicStream(g_music);
    if (g_music_mem) { free(g_music_mem); g_music_mem = NULL; }
    for (int i = 0; i < SFX_COUNT; i++) UnloadSound(g_sfx[i]);
    CloseAudioDevice();
    g_ready = false;
}

void UpdateGameAudio(void)
{
    if (g_ready) UpdateMusicStream(g_music);
}

void ApplyAudioVolumes(float music_vol, float sfx_vol)
{
    if (music_vol < 0.0f) music_vol = 0.0f;
    if (music_vol > 1.0f) music_vol = 1.0f;
    if (sfx_vol < 0.0f) sfx_vol = 0.0f;
    if (sfx_vol > 1.0f) sfx_vol = 1.0f;
    g_music_vol = music_vol;
    g_sfx_vol = sfx_vol;
    if (!g_ready) return;
    SetMusicVolume(g_music, g_music_vol);
    for (int i = 0; i < SFX_COUNT; i++) SetSoundVolume(g_sfx[i], g_sfx_vol);
}

void PlayGameSfx(SoundFx fx)
{
    if (!g_ready || fx < 0 || fx >= SFX_COUNT) return;
    PlaySound(g_sfx[fx]);
}
