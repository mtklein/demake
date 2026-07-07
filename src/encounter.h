#ifndef ENCOUNTER_H
#define ENCOUNTER_H
#include "gba.h"
#include "rules.h"

/* Battle 2.0: 5e encounters fought in place on the field map. */

/* Inputs to the root battle-menu build, decoupled from the (file-private)
 * combatant struct so the host harness (test/host) can pin the exact
 * per-class/level kit. pc_turn fills one of these each menu pass. */
typedef struct {
    u8 cls, level;        /* PMember class/level (stable across wild shape) */
    u8 shaped, smited, hidden;
    s8 engaged;           /* >= 0 shows Disengage */
    u8 action, bonus;     /* turn economy already spent */
    u8 nerve;             /* helm-objective row visible */
} PcMenuCtx;
int pc_menu_build(const R5Creature* c, const PcMenuCtx* x,
                  const char** items, u8* code);   /* returns row count */

typedef struct {
    u8 mon;     /* R5M_* stat block */
    s8 npc;     /* field npc index (the visible sprite) */
    u16 xp;
    u8 side;    /* 1 enemy, 2 ally (Us, the helm mind flayer) */
} EncSpawn;

enum { ENC_WIN, ENC_CONNECTED };

/* helm_rounds: 0 = normal fight; >0 = transponder countdown (that many
 * rounds; hero gains the Nerve! action; cambions warp in as it runs out) */
/* surprise: 0 none, 1 enemies skip round 1, 2 party+allies skip round 1 */
int encounter_run(const EncSpawn* es, int n, int helm_rounds, int surprise);

/* set the NEXT battle's theme (one-shot; default SONG_BATTLE) */
void encounter_song(int s);

/* darkvision doctrine (docs/character2.md): the room's DARK flag, set by
 * events.c on room entry. encounter_dark() is the live answer -- the
 * equipped Everburn Blade suppresses DARK entirely. */
void encounter_set_dark(int on);
int  encounter_dark(void);

#endif
