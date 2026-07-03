#ifndef BATTLE_H
#define BATTLE_H
#include "gba.h"

typedef struct {
    const char* name;
    s16 hp;
    u8 atk, def, mag, spd;
    u16 xp;
    u8 ai;
    u16 objt;          /* first tile of idle frame */
    u8 objs;           /* obj_set shapesize        */
    u8 pal;
    u8 tpf;            /* tiles per frame          */
    u8 wpx, hpx;       /* pixel size for popups    */
} EnemyDef;

enum { AI_IMP, AI_BOAR, AI_THRALL, AI_CAMBION, AI_ZHALK, AI_FLAYER_ALLY, AI_US_ALLY };

typedef struct {
    const EnemyDef* e[6];
    u8 x[6], y[6];
    u8 n;
    u8 flags;
} Formation;

enum { BF_HELM = 1, BF_ALLY_US = 2, BF_ALLY_FLAYER = 4, BF_NO_FLEE = 8 };
enum { BR_WIN, BR_FLED, BR_CONNECTED, BR_FLAYER_DONE };

extern const Formation form_deck, form_thralls, form_helm;

int battle_run(const Formation* f);

#endif
