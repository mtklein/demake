#ifndef GAME_H
#define GAME_H
#include "gba.h"

enum { CLS_BARD, CLS_ROGUE, CLS_RANGER, CLS_WIZARD, CLS_FIGHTER, CLS_CLERIC,
       CLS_BARBARIAN, CLS_DRUID, CLS_MONK, CLS_PALADIN, CLS_SORCERER,
       CLS_WARLOCK, CLS_COUNT };

typedef struct {
    char name[8];
    u8 cls, level, subclass;
    u32 prepared;    /* prepared-caster spell bitmask into r5_class_spells */
    u32 skills, expert;   /* SK_* proficiency + expertise bitmasks */
    u8 face;              /* art identity: ORIG_* (companions too) */
    u8 race;              /* R5RACE_*; 0 = none, every racial delta zero */
    u8 background;        /* R5BG_*;   0 = none, no extra skills */
    s16 hp, hpmax, mp, mpmax;
    u8 atk, def, mag, spd;
    u16 xp;
} PMember;

enum { TAC_ORDERS, TAC_WISELY, TAC_ALLOUT, TAC_HEALER, TAC_NOSLOTS, TAC_COUNT };

/* the beach arc's roster is five souls; three walk, the rest wait */
enum { RESERVE_MAX = 2 };

typedef struct {
    PMember pm[3];
    u8 nparty;
    u8 potions, revivify;
    u8 everburn;
    u8 tactics[3];       /* DQ-style per-member battle AI (TAC_*) */
    u8 weapon[3];        /* equipped R5W_* per member */
    u8 winv[8], nwinv;   /* unequipped weapons found around the ship */
    u16 flags;
    u8 origin;        /* ORIG_* -- 6 = Dark Urge, 7 = custom Tav */
    /* reserve roster: souls recruited past the walking three. New fields
     * append here -- test/scenario.py pokes pm[0] by absolute address. */
    PMember reserve[RESERVE_MAX];
    u8 rweapon[RESERVE_MAX];      /* their equipped R5W_* */
    u8 rtactic[RESERVE_MAX];      /* their TAC_* preference */
    u8 nreserve;
} Game;
enum { ORIG_ASTARION, ORIG_GALE, ORIG_KARLACH, ORIG_LAEZEL, ORIG_SHADOW,
       ORIG_WYLL, ORIG_DURGE, ORIG_CUSTOM, ORIG_COUNT };
typedef struct { u16 objt, ko; u8 pal; s8 por; } MemberLook;
MemberLook member_look(int face, int cls);   /* single source of party art */
void loot_weapon(int w);          /* add to inventory (menu equips) */
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
    GF_DUELIST      = 1 << 12,
    GF_STINGER      = 1 << 13,
    GF_W_DECK       = 1 << 14,   /* deck straggler slain */
    GF_W_PODS       = 1 << 15,   /* pods prowler slain */
};

#define HERO_CLS (G.pm[0].cls)

int origin_race(int o);          /* canon blood (character2.md identities) */
int origin_background(int o);    /* canon background */

void party_init(int cls, const char* name);
void party_add_laezel(void);
void party_add_shadowheart(void);
void party_add_astarion(void);
void party_add_gale(void);
void party_swap(int i, int r);   /* walking slot i (1..2; Tav holds 0) <-> reserve r */
int  party_give_xp(u16 xp, char* levelup_names);  /* returns # of level-ups */
void party_heal_full(void);      /* walkers and reserve both */

#endif
