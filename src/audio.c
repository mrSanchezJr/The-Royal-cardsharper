#include "audio.h"
#include "raylib.h"
#include "btn_press.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define AUDIO_SR 22050

typedef enum { OSC_SINE, OSC_TRI, OSC_SAW } OscType;

static Sound g_sfx[SFX_COUNT];
static Music g_tracks[MUSIC_COUNT];
static unsigned char* g_trackMems[MUSIC_COUNT];
static int g_trackMemSizes[MUSIC_COUNT];
static Music g_btn_music;
static bool g_btn_loaded = false;
static bool g_ready = false;
static float g_music_vol = 0.7f;
static float g_sfx_vol = 0.7f;
static MusicTrack g_curTrack = MUSIC_MENU;

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

static bool BuildWavFromMix(float* mix, int n, unsigned char** outMem, int* outSize)
{
    short* pcm = RenderToPcm(mix, n);
    free(mix);
    if (!pcm) return false;
    int dataLen = n * 2;
    int total = 44 + dataLen;
    unsigned char* m = (unsigned char*)malloc((size_t)total);
    if (!m) { free(pcm); return false; }
    memcpy(m + 0, "RIFF", 4);
    *(unsigned int*)(m + 4) = (unsigned int)(total - 8);
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
    *outMem = m;
    *outSize = total;
    return true;
}

static void BuildMenuMusic(void)
{
    const float beat = 0.75f;
    const int beats = 12;
    int n = (int)(beats * beat * AUDIO_SR);
    float* mix = (float*)calloc((size_t)n, sizeof(float));
    if (!mix) return;
    static const float roots[3] = { 98.0f, 110.0f, 123.47f };
    for (int c = 0; c < 3; c++) {
        float s = c * 4 * beat;
        AddTone(mix, n, s, 4*beat, roots[c], roots[c], 0.14f, OSC_SINE, 0.7f);
        AddTone(mix, n, s+2*beat, 2*beat, roots[c]*1.5f, roots[c]*1.5f, 0.07f, OSC_SINE, 1.2f);
        for (int k = 0; k < 8; k++) {
            float f = roots[c] * 2.0f * (k%2 ? 1.5f : 1.0f);
            AddTone(mix, n, s + k*0.5f*beat, beat*0.6f, f, f, 0.09f, OSC_TRI, 6.0f);
        }
    }
    static const float pent[5] = { 440.0f, 523.25f, 587.33f, 659.25f, 783.99f };
    static const int mel[12] = { 2, -1, 3, 1, 2, 0, 4, -1, 1, 2, 0, -1 };
    for (int b = 0; b < beats; b++) if (mel[b] >= 0)
        AddTone(mix, n, b*beat, beat*1.1f, pent[mel[b]], pent[mel[b]], 0.10f, OSC_SINE, 2.5f);
    for (int b = 0; b < beats; b++) AddNoise(mix, n, b*beat, 0.02f, 0.02f, 1, 70.0f);
    BuildWavFromMix(mix, n, &g_trackMems[MUSIC_MENU], &g_trackMemSizes[MUSIC_MENU]);
}

static void BuildGameCalmMusic(void)
{
    const float beat = 0.68f;
    const int beats = 16;
    int n = (int)(beats * beat * AUDIO_SR);
    float* mix = (float*)calloc((size_t)n, sizeof(float));
    if (!mix) return;
    float loopDur = beats * beat;
    AddTone(mix, n, 0, loopDur, 110.0f, 110.0f, 0.07f, OSC_SINE, 0.12f);
    AddTone(mix, n, 0, loopDur, 164.81f, 164.81f, 0.045f, OSC_SINE, 0.14f);
    for (int b = 0; b < beats; b++) {
        float amp = (b%4==0) ? 0.09f : (b%2==0 ? 0.05f : 0.03f);
        AddTone(mix, n, b*beat, 0.16f, 72.0f, 48.0f, amp, OSC_SINE, 18.0f);
        AddNoise(mix, n, b*beat, 0.04f, amp*0.18f, 1, 55.0f);
    }
    static const float dor[8] = { 440.0f, 493.88f, 523.25f, 587.33f, 659.25f, 739.99f, 783.99f, 880.0f };
    static const int mel[16] = { 4, -1, 3,2, 4,3,2,1, 3,2,1,0, 1,2,4,-1 };
    for (int b = 0; b < beats; b++) if (mel[b] >= 0) {
        AddTone(mix, n, b*beat, beat*0.92f, dor[mel[b]], dor[mel[b]], 0.13f, OSC_TRI, 3.2f);
        AddTone(mix, n, b*beat, beat*0.92f, dor[mel[b]]*0.5f, dor[mel[b]]*0.5f, 0.035f, OSC_SINE, 1.2f);
    }
    for (int b = 0; b < beats; b+=4) {
        float f = dor[(b/4)%8];
        AddTone(mix, n, b*beat, 0.4f, f*1.5f, f*1.5f, 0.05f, OSC_TRI, 7.0f);
    }
    BuildWavFromMix(mix, n, &g_trackMems[MUSIC_GAME_CALM], &g_trackMemSizes[MUSIC_GAME_CALM]);
}

static void BuildDuskRoadMusic(void)
{
    // Middle between GREY ROAD (2) and BAGPIPES (3), closer to 2 - calm but with hint of tension
    const float beat = 0.66f; // between 0.68 and 0.625, weighted to calm
    const int beats = 16;
    int n = (int)(beats * beat * AUDIO_SR);
    float* mix = (float*)calloc((size_t)n, sizeof(float));
    if (!mix) return;
    float loopDur = beats * beat;
    // drones: SINE base like calm (0.07/0.045) + faint SAW hint from bagpipes (very low)
    AddTone(mix, n, 0, loopDur, 110.0f, 110.0f, 0.075f, OSC_SINE, 0.12f);
    AddTone(mix, n, 0, loopDur, 164.81f, 164.81f, 0.050f, OSC_SINE, 0.14f);
    AddTone(mix, n, 0, loopDur, 110.0f, 110.0f, 0.032f, OSC_SAW, 0.20f); // faint bagpipe colour
    AddTone(mix, n, 0, loopDur, 165.4f, 165.4f, 0.018f, OSC_SINE, 0.22f);
    AddTone(mix, n, 0, loopDur, 55.0f, 55.0f, 0.040f, OSC_SINE, 0.10f);
    for (int b = 0; b < beats; b++) {
        float amp = (b%4==0) ? 0.10f : (b%2==0 ? 0.055f : 0.030f); // between calm 0.09 and bagpipes 0.13
        AddTone(mix, n, b*beat, 0.15f, 72.0f, 48.0f, amp, OSC_SINE, 16.0f);
        AddNoise(mix, n, b*beat, 0.04f, amp*0.19f, 1, 48.0f);
    }
    static const float dor[8] = { 440.0f, 493.88f, 523.25f, 587.33f, 659.25f, 739.99f, 783.99f, 880.0f };
    // melody between calm (- TRI only) and bagpipes (- SAW + grace) - TRI main + soft grace
    static const int mel[16] = { 4, -1, 3,2, 4,3,2,1, 4,2,1,0, 1,2,4,-1 };
    for (int b = 0; b < beats; b++) if (mel[b] >= 0) {
        int g = mel[b]+1; if (g>7) g=7;
        if (b>0 && b%2==0) AddTone(mix, n, b*beat-0.06f, 0.05f, dor[g], dor[g], 0.035f, OSC_TRI, 13.0f); // occasional grace, softer
        AddTone(mix, n, b*beat, beat*0.92f, dor[mel[b]], dor[mel[b]], 0.125f, OSC_TRI, 3.0f); // TRI like calm, slightly louder
        AddTone(mix, n, b*beat, beat*0.92f, dor[mel[b]]*0.5f, dor[mel[b]]*0.5f, 0.030f, OSC_SINE, 1.2f);
        if (mel[b]>=3) AddTone(mix, n, b*beat, beat*0.92f, dor[mel[b]], dor[mel[b]], 0.035f, OSC_SAW, 5.0f); // faint SAW overtone on higher notes
    }
    for (int b = 0; b < beats; b+=4) {
        float f = dor[(b/2)%8];
        AddTone(mix, n, b*beat, 0.38f, f*1.5f, f*1.5f, 0.042f, OSC_TRI, 8.0f);
    }
    // also add faint lute every 8 beats from bagpipes side
    for (int b = 0; b < beats; b+=8) {
        float f = dor[(b/8)%8];
        AddTone(mix, n, b*beat+0.3f, 0.30f, f*2.0f, f*2.0f, 0.022f, OSC_TRI, 9.0f);
    }
    BuildWavFromMix(mix, n, &g_trackMems[MUSIC_DUSK_ROAD], &g_trackMemSizes[MUSIC_DUSK_ROAD]);
}

static void BuildGameMusic(void)
{
    // Calmer but same tempo 0.625 - less harsh via softer detune & TRI grace
    const float beat = 0.625f;
    const int beats = 16;
    int n = (int)(beats * beat * AUDIO_SR);
    float* mix = (float*)calloc((size_t)n, sizeof(float));
    if (!mix) return;
    float loopDur = beats * beat;
    // drones: main SAW kept, detuned pair softened via SINE (less beating harshness)
    AddTone(mix, n, 0, loopDur, 110.0f, 110.0f, 0.12f, OSC_SAW, 0.13f);
    AddTone(mix, n, 0, loopDur, 110.7f, 110.7f, 0.04f, OSC_SINE, 0.18f);
    AddTone(mix, n, 0, loopDur, 164.81f, 164.81f, 0.07f, OSC_SAW, 0.16f);
    AddTone(mix, n, 0, loopDur, 165.4f, 165.4f, 0.025f, OSC_SINE, 0.20f);
    AddTone(mix, n, 0, loopDur, 55.0f, 55.0f, 0.055f, OSC_SINE, 0.09f);
    for (int b = 0; b < beats; b++) {
        float amp = (b%4==0) ? 0.13f : (b%2==0 ? 0.07f : 0.035f); // softer than 0.20/0.11/0.06
        AddTone(mix, n, b*beat, 0.15f, 78.0f, 42.0f, amp, OSC_SINE, 13.0f);
        AddNoise(mix, n, b*beat, 0.04f, amp*0.20f, 1, 42.0f);
    }
    static const float dor[8] = { 440.0f, 493.88f, 523.25f, 587.33f, 659.25f, 739.99f, 783.99f, 880.0f };
    static const int mel[16] = { 4, -1, 3,2, 4,3,2,1, 4,3,2,1, 0,1,2,-1 };
    for (int b = 0; b < beats; b++) if (mel[b] >= 0) {
        int g = mel[b]+1; if (g>7) g=7;
        if (b>0) AddTone(mix, n, b*beat-0.07f, 0.06f, dor[g], dor[g], 0.05f, OSC_TRI, 12.0f); // TRI grace, was SAW 0.10
        AddTone(mix, n, b*beat, beat*0.95f, dor[mel[b]], dor[mel[b]], 0.145f, OSC_SAW, 3.2f); // 0.20->0.145
        AddTone(mix, n, b*beat, beat*0.95f, dor[mel[b]]*0.5f, dor[mel[b]]*0.5f, 0.028f, OSC_SINE, 1.3f);
    }
    for (int b = 0; b < beats; b+=2) {
        float f = dor[ (b/2)%8 ];
        AddTone(mix, n, b*beat, 0.35f, f*2.0f, f*2.0f, 0.045f, OSC_TRI, 10.5f);
    }
    BuildWavFromMix(mix, n, &g_trackMems[MUSIC_GAME], &g_trackMemSizes[MUSIC_GAME]);
}

static void BuildEpilogueMusic(void)
{
    const float beat = 0.90f;
    const int beats = 12;
    int n = (int)(beats * beat * AUDIO_SR);
    float* mix = (float*)calloc((size_t)n, sizeof(float));
    if (!mix) return;
    static const float padRoots[4] = { 130.81f, 146.83f, 164.81f, 130.81f };
    for (int c = 0; c < 4; c++) {
        float s = c*3*beat;
        // longer sustain (smaller decay) to keep pad loud till loop edge
        AddTone(mix, n, s, 3*beat, padRoots[c], padRoots[c], 0.11f, OSC_SINE, 0.22f);
        AddTone(mix, n, s, 3*beat, padRoots[c]*2.0f, padRoots[c]*2.0f, 0.045f, OSC_TRI, 0.55f);
        AddTone(mix, n, s, 3*beat, padRoots[c]*3.0f/2.0f, padRoots[c]*3.0f/2.0f, 0.032f, OSC_SINE, 0.40f);
    }
    static const float lyr[5] = { 523.25f, 587.33f, 659.25f, 783.99f, 880.0f };
    // last note was rest (-1) creating 0.9s silence before loop - replaced with 0 for continuity
    static const int mel[12] = { 2, 3, 4, 2, 1, 0, 1, 2, 3, 2, 1, 0 };
    for (int b = 0; b < beats; b++) if (mel[b] >= 0) {
        AddTone(mix, n, b*beat+0.12f, beat*0.85f, lyr[mel[b]], lyr[mel[b]], 0.14f, OSC_SINE, 1.25f);
        AddTone(mix, n, b*beat+0.12f, beat*0.85f, lyr[mel[b]]*2.0f, lyr[mel[b]]*2.0f, 0.025f, OSC_TRI, 1.9f);
    }
    for (int g = 0; g < 4; g++) {
        float base = g*3*beat + 1.4f*beat;
        for (int k = 0; k < 5; k++)
            AddTone(mix, n, base + k*0.07f, 0.5f, lyr[k%5], lyr[k%5], 0.038f, OSC_TRI, 3.5f);
    }
    // --- true seamless loop: long equal-power crossfade + very short click fade ---
    {
        // 1.8 sec ~ 2 beats crossfade - covers pad overlap and harp tail
        int x = (int)(1.80f * AUDIO_SR);
        if (x > n) x = n/2;
        for (int i=0;i<x;i++) {
            float t = (float)i / (float)x; // 0..1
            // equal-power crossfade: tail fades out cos, head fades in sin
            float wTail = cosf(t * PI * 0.5f) * 0.55f;
            float wHead = sinf(t * PI * 0.5f);
            // blend tail energy into head region
            mix[i] = mix[i] * (0.45f + 0.55f * wHead) + mix[n - x + i] * wTail;
        }
        // also smear last harp gliss into first 0.4s so gap 10.75->12.0 disappears
        for (int k=0;k<5;k++) {
            float pos = k*0.07f;
            if (pos < 0.35f) AddTone(mix, n, pos, 0.5f, lyr[k%5], lyr[k%5], 0.018f, OSC_TRI, 3.5f);
        }
        int fade = (int)(0.008f * AUDIO_SR); // 8ms click-free only, not to kill energy
        for (int i=0;i<fade && i<n;i++) mix[i] *= (float)i / (float)fade;
        for (int i=n-fade;i<n;i++) if (i>=0) mix[i] *= (float)(n-1 - i) / (float)fade;
    }
    BuildWavFromMix(mix, n, &g_trackMems[MUSIC_EPILOGUE], &g_trackMemSizes[MUSIC_EPILOGUE]);
}

void InitGameAudio(void)
{
    InitAudioDevice();
    g_ready = IsAudioDeviceReady();
    if (!g_ready) return;
    for (int i = 0; i < SFX_COUNT; i++) g_sfx[i] = (Sound){ 0 };
    for (int i = 0; i < MUSIC_COUNT; i++) { g_tracks[i] = (Music){0}; g_trackMems[i]=NULL; g_trackMemSizes[i]=0; }
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
    BuildMenuMusic();
    BuildGameCalmMusic();
    BuildDuskRoadMusic();
    BuildGameMusic();
    BuildEpilogueMusic();
    for (int i = 0; i < MUSIC_COUNT; i++) {
        if (g_trackMems[i]) {
            g_tracks[i] = LoadMusicStreamFromMemory(".wav", g_trackMems[i], g_trackMemSizes[i]);
            g_tracks[i].looping = true;
            SetMusicVolume(g_tracks[i], g_music_vol);
        }
    }
    g_curTrack = MUSIC_MENU;
    PlayMusicStream(g_tracks[g_curTrack]);
    g_btn_music = LoadMusicStreamFromMemory(".mp3", src_audio_btn_press_MP3, (int)src_audio_btn_press_MP3_len);
    g_btn_music.looping = false;
    g_btn_loaded = (g_btn_music.frameCount > 0);
    ApplyAudioVolumes(g_music_vol, g_sfx_vol);
}

void CloseGameAudio(void)
{
    if (!g_ready) return;
    for (int i = 0; i < MUSIC_COUNT; i++) {
        UnloadMusicStream(g_tracks[i]);
        if (g_trackMems[i]) { free(g_trackMems[i]); g_trackMems[i]=NULL; }
    }
    if (g_btn_loaded) { UnloadMusicStream(g_btn_music); g_btn_loaded = false; }
    for (int i = 0; i < SFX_COUNT; i++) UnloadSound(g_sfx[i]);
    CloseAudioDevice();
    g_ready = false;
}

void UpdateGameAudio(void)
{
    if (!g_ready) return;
    UpdateMusicStream(g_tracks[g_curTrack]);
    if (g_btn_loaded) UpdateMusicStream(g_btn_music);
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
    for (int i = 0; i < MUSIC_COUNT; i++) SetMusicVolume(g_tracks[i], g_music_vol);
    if (g_btn_loaded) SetMusicVolume(g_btn_music, g_sfx_vol);
    for (int i = 0; i < SFX_COUNT; i++) SetSoundVolume(g_sfx[i], g_sfx_vol);
}

void PlayGameSfx(SoundFx fx)
{
    if (!g_ready || fx < 0 || fx >= SFX_COUNT) return;
    PlaySound(g_sfx[fx]);
}

void PlayButtonSfx(void)
{
    if (!g_ready || !g_btn_loaded) return;
    StopMusicStream(g_btn_music);
    PlayMusicStream(g_btn_music);
}

void SetMusicTrack(MusicTrack track)
{
    if (!g_ready || track < 0 || track >= MUSIC_COUNT) return;
    if (track == g_curTrack) return;
    StopMusicStream(g_tracks[g_curTrack]);
    g_curTrack = track;
    PlayMusicStream(g_tracks[g_curTrack]);
    SetMusicVolume(g_tracks[g_curTrack], g_music_vol);
}
