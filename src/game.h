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
    s8 ab6[6];            /* assigned base scores, pre-ASI; all-zero = class preset */
    s16 hp, hpmax, mp, mpmax;
    u8 atk, def, mag, spd;
    u16 xp;
} PMember;

enum { TAC_ORDERS, TAC_WISELY, TAC_ALLOUT, TAC_HEALER, TAC_NOSLOTS, TAC_COUNT };

/* the beach arc's roster is six souls (Wyll joins at the grove gates);
 * three walk, the rest wait */
enum { RESERVE_MAX = 3 };

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
    u32 bflags;                   /* BF_* beach-arc story bits (GF_ is full;
                                   * widened in place -- it is the LAST field,
                                   * so the poked addresses before it hold) */
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

/* beach-arc story bits (G.bflags); GF_* above is the ship's word and it is
 * full -- the crash starts a fresh one */
enum {
    BF_SH_RECOVERED = 1 << 0,    /* Shadowheart found ashore and rejoined */
    BF_LZ_RECOVERED = 1 << 1,    /* Lae'zel freed from the scavenger cage */
    BF_SCAVS_GONE   = 1 << 2,    /* the tiefling scavengers bolted */
    BF_FLAYER_DONE  = 1 << 3,    /* the dying mind flayer beat resolved */
    BF_FLAYER_SLAIN = 1 << 4,    /* ...by your hand (else left to the tide) */
    BF_DEV_CRASH    = 1 << 5,    /* crash-site devourer slain */
    BF_DEV_DUNE     = 1 << 6,    /* dune-path devourer slain */
    BF_DEV_DUNE2    = 1 << 7,    /* second dune-path devourer slain */
    BF_CHEST_BEACH  = 1 << 8,    /* crash-site chest looted */
    BF_CHEST_DUNE   = 1 << 9,    /* dune cache looted */
    BF_AST_RECRUITED= 1 << 10,   /* Astarion's knife beat resolved: he walks */
    BF_GALE_RECRUITED=1 << 11,   /* Gale pulled through the portal sigil */
    BF_BOAR_DRAINED = 1 << 12,   /* origin Astarion fed on his staked kill */
    /* stone 4: the chapel on the bluff and the crypt beneath */
    BF_LOOTERS_GONE = 1 << 13,   /* the band before the tomb door resolved */
    BF_TOMB_OPEN    = 1 << 14,   /* the sealed door ground aside */
    BF_CRYPT_BONES  = 1 << 15,   /* the ossuary ambush put back down */
    BF_WITHERS_AWAKE= 1 << 16,   /* the sarcophagus opened; he keeps office */
    BF_CHEST_CHAPEL = 1 << 17,   /* chapel yard chest looted */
    BF_CHEST_CRYPT  = 1 << 18,   /* crypt grave-gifts looted */
    /* stone 5: the camp night */
    BF_CAMP_SCENE   = 1 << 19,   /* the Under Selune scene played; never again */
    /* stone 6: the grove gates */
    BF_WARRYN_SEEN  = 1 << 20,   /* stood close enough; the tadpole knows its kin */
    BF_GATES_WON    = 1 << 21,   /* the assault broken; the door still shut */
    BF_CHEST_GATES  = 1 << 22,   /* the Hellrider cache shared out */
};

#define HERO_CLS (G.pm[0].cls)

int origin_race(int o);          /* canon blood (character2.md identities) */
int origin_background(int o);    /* canon background */
int origin_subclass(int o);      /* canon subclass; 255 = the player's pick */

void game_creation(int cls, int origin);   /* pickers + name + party build */
int  game_race_pick(int origin);           /* -> R5RACE_* entry */
int  game_bg_pick(int origin);             /* -> R5BG_*, -1 = back */
int  game_stats_assign(int cls, int race, s8 out[6]);  /* 1 done, 0 = back */
void game_story_karaoke(int song);  /* synced lyrics over the caller's scene;
        * returns when the song has played through or START skips (the camp
        * night; the jukebox keeps its own chrome-and-exit loop) */

void party_init(int cls, const char* name);
void party_set_identity(int race, int bg, const s8* ab6);  /* hero, post-init */
void party_add_laezel(void);
void party_add_shadowheart(void);
void party_add_astarion(void);
void party_add_gale(void);
void party_add_wyll(void);       /* the gates victory: the sixth soul */
void party_swap(int i, int r);   /* walking slot i (1..2; Tav holds 0) <-> reserve r */
int  party_give_xp(u16 xp, char* levelup_names);  /* returns # of level-ups;
        * names gets one ' '-joined name per level-up gained THIS award:
        * worst case 3 members x 2 ups x 8 bytes -- callers pass char[48] */
int  party_canon_subclass(PMember* p);  /* companions auto-take canon at the reveal */
void party_heal_full(void);      /* walkers and reserve both */
void party_scatter(void);        /* the crash: Tav alone, whole (narrative long rest) */

#endif
