#include "sfx.h"

#ifndef TESTING_HEADLESS
#include <raylib.h>
#include <math.h>
#include <stdint.h>

typedef enum {
    SFX_HIT_L = 0,
    SFX_HIT_M,
    SFX_HIT_H,
    SFX_LAUNCH,
    SFX_THROW,
    SFX_WALL,
    SFX_BLOCK,
    SFX_COUNT
} SfxId;

typedef struct {
    int initialized;
    int audio_owned;
    Sound sounds[SFX_COUNT];
    uint32_t rng;
} SfxState;

static SfxState g_sfx = {0};

static uint32_t sfx_rand_u32(void) {
    g_sfx.rng = g_sfx.rng * 1664525u + 1013904223u;
    return g_sfx.rng;
}

static float sfx_rand_unit(void) {
    return (float)(sfx_rand_u32() & 0x00ffffffu) / 16777215.0f;
}

static float sfx_pitch_jitter(float span) {
    float t = sfx_rand_unit() * 2.0f - 1.0f;
    return 1.0f + t * span;
}

static Sound build_impact_sound(float freq, float duration_ms, float noise_mix, float decay) {
    const int sample_rate = 44100;
    int frames = (int)((duration_ms / 1000.0f) * (float)sample_rate);
    if (frames < 1) frames = 1;

    short *pcm = (short *)MemAlloc((unsigned int)frames * sizeof(short));
    for (int i = 0; i < frames; i++) {
        float t = (float)i / (float)sample_rate;
        float env = expf(-decay * t);
        float base = sinf(2.0f * PI * freq * t);
        float noise = (sfx_rand_unit() * 2.0f - 1.0f);
        float mixed = (base * (1.0f - noise_mix) + noise * noise_mix) * env;
        int v = (int)(mixed * 28000.0f);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        pcm[i] = (short)v;
    }

    {
        Wave wave = {0};
        Sound sound;
        wave.frameCount = (unsigned int)frames;
        wave.sampleRate = sample_rate;
        wave.sampleSize = 16;
        wave.channels = 1;
        wave.data = pcm;
        sound = LoadSoundFromWave(wave);
        MemFree(pcm);
        return sound;
    }
}

void sfx_init(void) {
    if (g_sfx.initialized) return;

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        if (IsAudioDeviceReady()) g_sfx.audio_owned = 1;
    }
    if (!IsAudioDeviceReady()) return;

    g_sfx.rng = 0xC0DEFACEu;
    g_sfx.sounds[SFX_HIT_L] = build_impact_sound(980.0f, 36.0f, 0.35f, 26.0f);
    g_sfx.sounds[SFX_HIT_M] = build_impact_sound(740.0f, 44.0f, 0.30f, 23.0f);
    g_sfx.sounds[SFX_HIT_H] = build_impact_sound(510.0f, 56.0f, 0.25f, 20.0f);
    g_sfx.sounds[SFX_LAUNCH] = build_impact_sound(620.0f, 62.0f, 0.20f, 17.0f);
    g_sfx.sounds[SFX_THROW] = build_impact_sound(420.0f, 76.0f, 0.32f, 14.0f);
    g_sfx.sounds[SFX_WALL] = build_impact_sound(360.0f, 84.0f, 0.38f, 12.0f);
    g_sfx.sounds[SFX_BLOCK] = build_impact_sound(1160.0f, 34.0f, 0.48f, 24.0f);

    SetSoundVolume(g_sfx.sounds[SFX_HIT_L], 0.33f);
    SetSoundVolume(g_sfx.sounds[SFX_HIT_M], 0.40f);
    SetSoundVolume(g_sfx.sounds[SFX_HIT_H], 0.50f);
    SetSoundVolume(g_sfx.sounds[SFX_LAUNCH], 0.55f);
    SetSoundVolume(g_sfx.sounds[SFX_THROW], 0.60f);
    SetSoundVolume(g_sfx.sounds[SFX_WALL], 0.65f);
    SetSoundVolume(g_sfx.sounds[SFX_BLOCK], 0.28f);

    g_sfx.initialized = 1;
}

void sfx_shutdown(void) {
    if (!g_sfx.initialized) return;
    for (int i = 0; i < SFX_COUNT; i++) {
        UnloadSound(g_sfx.sounds[i]);
    }
    if (g_sfx.audio_owned && IsAudioDeviceReady()) {
        CloseAudioDevice();
    }
    g_sfx.initialized = 0;
    g_sfx.audio_owned = 0;
}

void sfx_play_impact(int damage, int blocked, int is_throw, int is_launcher, int is_wall_bounce) {
    SfxId id = SFX_HIT_L;
    float pitch;

    if (!g_sfx.initialized) return;
    if (blocked) id = SFX_BLOCK;
    else if (is_throw) id = SFX_THROW;
    else if (is_wall_bounce) id = SFX_WALL;
    else if (is_launcher) id = SFX_LAUNCH;
    else if (damage >= 1800) id = SFX_HIT_H;
    else if (damage >= 1300) id = SFX_HIT_M;
    else id = SFX_HIT_L;

    pitch = blocked ? sfx_pitch_jitter(0.02f) : sfx_pitch_jitter(0.035f);
    SetSoundPitch(g_sfx.sounds[id], pitch);
    PlaySound(g_sfx.sounds[id]);
}

#else

void sfx_init(void) {}
void sfx_shutdown(void) {}
void sfx_play_impact(int damage, int blocked, int is_throw, int is_launcher, int is_wall_bounce) {
    (void)damage;
    (void)blocked;
    (void)is_throw;
    (void)is_launcher;
    (void)is_wall_bounce;
}

#endif
