#ifndef ENCOUNTER_H
#define ENCOUNTER_H
#include "gba.h"

/* Battle 2.0: 5e encounters fought in place on the field map. */

typedef struct {
    u8 mon;     /* R5M_* stat block */
    s8 npc;     /* field npc index (the visible enemy sprite) */
    u16 xp;
} EncSpawn;

enum { ENC_WIN, ENC_FLED };
enum { ENCF_ALLY_US = 1 };   /* freed Us fights on side 2 */

int encounter_run(const EncSpawn* es, int n, int flags);

#endif
