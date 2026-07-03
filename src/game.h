#ifndef GAME_H
#define GAME_H
#include "gba.h"

enum { CLS_BARD, CLS_ROGUE, CLS_RANGER, CLS_WIZARD, CLS_FIGHTER, CLS_CLERIC };

typedef struct {
    char name[8];
    u8 cls, level;
    s16 hp, hpmax, mp, mpmax;
    u8 atk, def, mag, spd;
    u16 xp;
} PMember;

enum { TAC_ORDERS, TAC_WISELY, TAC_ALLOUT, TAC_HEALER, TAC_NOSLOTS, TAC_COUNT };

typedef struct {
    PMember pm[3];
    u8 nparty;
    u8 potions, revivify;
    u8 everburn;
    u8 tactics[3];       /* DQ-style per-member battle AI (TAC_*) */
    u16 flags;
} Game;
extern Game G;

enum {
    GF_US_FREED     = 1 << 0,
    GF_US_MUTILATED = 1 << 1,
    GF_LAEZEL       = 1 << 2,
    GF_SH_FREED     = 1 << 3,
    GF_RUNE         = 1 << 4,
    GF_ZHALK_DEAD   = 1 << 5,
    GF_THRALLS_DONE = 1 << 6,
    GF_CHEST_NURSERY= 1 << 7,
    GF_SLATE_READ   = 1 << 8,
    GF_DECK_FOUGHT  = 1 << 9,
    GF_CONSOLE_SEEN = 1 << 10,
    GF_RELIQUARY    = 1 << 11,
};

#define HERO_CLS (G.pm[0].cls)

void party_init(int cls, const char* name);
void party_add_laezel(void);
void party_add_shadowheart(void);
int  party_give_xp(u16 xp, char* levelup_names);  /* returns # of level-ups */
void party_heal_full(void);

#endif
