#ifndef DSR_AUDIO_H
#define DSR_AUDIO_H

#include "core.h"

/* Audio system — wraps DSA (Data-oriented Streaming Audio)
 * DSA at /home/lain/dsa/
 */

typedef struct dsr_audio dsr_audio;

/* Create audio system */
dsr_audio *audio_create(void);

/* Destroy audio system */
void audio_destroy(dsr_audio *a);

/* Set master volume (0.0 - 1.0) */
void audio_set_master_volume(dsr_audio *a, f32 volume);

/* Play a sound effect (world position) */
int audio_play_sfx(dsr_audio *a, const char *name, vec3 pos, f32 volume);

/* Play a sound effect (2D UI) */
int audio_play_ui(dsr_audio *a, const char *name, f32 volume);

/* Play music */
int audio_play_music(dsr_audio *a, const char *name, bool loop);

/* Stop all sounds */
void audio_stop_all(dsr_audio *a);

/* Update listener position */
void audio_set_listener(dsr_audio *a, vec3 pos, vec3 forward, vec3 up);

/* Tick audio (call per frame) */
void audio_update(dsr_audio *a, f32 dt);

#endif /* DSR_AUDIO_H */
