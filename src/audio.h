#ifndef AUDIO_H
#define AUDIO_H
#include "gba.h"

#define NOTE_MAX 59
#define EV_SUS   60   /* sustain: no change   */
#define EV_CUT   61   /* silence the channel  */

/* note index helper: octave 2..6, semitone 0..11 */
#define N(oct, semi) (((oct) - 2) * 12 + (semi))

typedef struct {
    const u8* ch[4];   /* sq1, sq2, wave, noise: one byte per row */
    u16 rows;
    u16 loop;          /* row to loop to, 0xFFFF = play once */
    u8  speed;         /* frames per row */
    u16 env1, env2;    /* SOUNDxCNT_H envelope/duty for squares */
    u16 wavevol;       /* SOUND3CNT_H volume bits */
} Song;

enum { SFX_CURSOR, SFX_CONFIRM, SFX_CANCEL, SFX_HIT, SFX_HEAL, SFX_STEP };

void audio_init(void);
void audio_tick(void);
void music_play(const Song* s);
void music_stop(void);
void sfx_play(int id);
void sfx_noise(int frames);

#endif
