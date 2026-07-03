#ifndef ENCOUNTER_H
#define ENCOUNTER_H
#include "gba.h"

/* Battle 2.0: 5e encounters fought in place on the field map. */

typedef struct {
    u8 mon;     /* R5M_* stat block */
    s8 npc;     /* field npc index (the visible sprite) */
    u16 xp;
    u8 side;    /* 1 enemy, 2 ally (Us, the helm mind flayer) */
} EncSpawn;

enum { ENC_WIN, ENC_CONNECTED };

/* helm_rounds: 0 = normal fight; >0 = transponder countdown (that many
 * rounds; hero gains the Nerve! action; cambions warp in as it runs out) */
int encounter_run(const EncSpawn* es, int n, int helm_rounds);

#endif
