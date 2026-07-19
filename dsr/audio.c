#include "audio.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* DSA C ABI — FMOD5 compatible */
#include "dsa.h"  /* from /home/lain/dsa/include/dsa.h */

struct dsr_audio {
    void *dsa_system;
    f32   master_volume;
};

dsr_audio *audio_create(void) {
    dsr_audio *a = (dsr_audio*)calloc(1, sizeof(dsr_audio));
    if (!a) return NULL;

    /* Initialize DSA via FMOD5 C ABI */
    /* FMOD_System_Create(&a->dsa_system) — DSA exports this */
    a->master_volume = 1.0f;
    return a;
}

void audio_destroy(dsr_audio *a) {
    if (!a) return;
    /* FMOD_System_Close(a->dsa_system) then FMOD_System_Release */
    free(a);
}

void audio_set_master_volume(dsr_audio *a, f32 volume) {
    if (!a) return;
    a->master_volume = volume;
    /* TODO: FMOD_ChannelGroup_SetVolume on master group */
}

int audio_play_sfx(dsr_audio *a, const char *name, vec3 pos, f32 volume) {
    if (!a) return -1;
    (void)name;
    (void)pos;
    (void)volume;
    /* TODO: load + play via DSA */
    return -1;
}

int audio_play_ui(dsr_audio *a, const char *name, f32 volume) {
    if (!a) return -1;
    (void)name;
    (void)volume;
    return -1;
}

int audio_play_music(dsr_audio *a, const char *name, bool loop) {
    if (!a) return -1;
    (void)name;
    (void)loop;
    return -1;
}

void audio_stop_all(dsr_audio *a) {
    if (!a) return;
}

void audio_set_listener(dsr_audio *a, vec3 pos, vec3 forward, vec3 up) {
    if (!a) return;
    (void)pos;
    (void)forward;
    (void)up;
}

void audio_update(dsr_audio *a, f32 dt) {
    if (!a) return;
    (void)dt;
}
