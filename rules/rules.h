/* 5e SRD combat rules core.
 *
 * Pure, freestanding C: no GBA headers, no libc calls, RNG injected.
 * Compiles for the ROM (arm-none-eabi) and the host (`make test-rules`).
 * Every die thrown is recorded so the UI can SHOW the roll, BG3-style.
 *
 * Known deviations from RAW (deliberate, documented):
 *   - No death saving throws: PCs at 0 HP are unconscious until healed
 *     (Revivify scroll / healing word), monsters at 0 HP are dead.
 *   - Positioning/cover/opportunity attacks live in the battle layer.
 */
#ifndef RULES_H
#define RULES_H

#include <stdint.h>

/* ---------------------------------------------------------------- dice */

typedef struct { uint32_t s; } R5RNG;

void     r5_seed(R5RNG*, uint32_t seed);
uint32_t r5_rand(R5RNG*);
int      r5_die(R5RNG*, int sides);            /* 1..sides */

/* a thrown-and-recorded dice expression, e.g. 2d6+3 */
#define R5_MAX_DICE 10
typedef struct {
    uint8_t n, sides;
    int8_t  mod;
    uint8_t rolls[R5_MAX_DICE];
    int16_t total;                             /* sum(rolls) + mod */
} R5Dice;

R5Dice r5_roll(R5RNG*, int n, int sides, int mod);

typedef struct { uint8_t n, sides; int8_t mod; } R5DiceSpec;

/* ---------------------------------------------------------------- flags */

enum { R5_STR, R5_DEX, R5_CON, R5_INT, R5_WIS, R5_CHA };

/* attack/roll option flags */
enum {
    R5F_ADV       = 1 << 0,
    R5F_DIS       = 1 << 1,   /* ADV|DIS cancel to a straight roll */
    R5F_BLESS     = 1 << 2,   /* +1d4 to the attack roll / save    */
    R5F_MARK      = 1 << 3,   /* hunter's mark: +1d6 damage        */
    R5F_VERSATILE = 1 << 4,   /* use two-handed damage die         */
    R5F_SNEAK     = 1 << 5,   /* add sneak attack dice (by level)  */
    R5F_AUTOCRIT  = 1 << 6,   /* melee vs unconscious: hit -> crit */
    R5F_LUCKY     = 1 << 7,   /* halfling: reroll natural 1s once  */
};

/* weapon properties */
enum {
    WP_FINESSE = 1 << 0, WP_LIGHT = 1 << 1, WP_HEAVY = 1 << 2,
    WP_TWO_HANDED = 1 << 3, WP_VERSATILE = 1 << 4, WP_THROWN = 1 << 5,
    WP_AMMUNITION = 1 << 6, WP_LOADING = 1 << 7, WP_REACH = 1 << 8,
    WP_RANGED = 1 << 9,
};

/* damage types */
enum {
    DT_SLASHING, DT_PIERCING, DT_BLUDGEONING, DT_FIRE, DT_COLD, DT_POISON,
    DT_PSYCHIC, DT_RADIANT, DT_NECROTIC, DT_LIGHTNING, DT_THUNDER, DT_ACID,
    DT_FORCE, DT_COUNT,
};

/* the 14 SRD conditions */
enum {
    C_BLINDED = 1 << 0, C_CHARMED = 1 << 1, C_DEAFENED = 1 << 2,
    C_FRIGHTENED = 1 << 3, C_GRAPPLED = 1 << 4, C_INCAPACITATED = 1 << 5,
    C_INVISIBLE = 1 << 6, C_PARALYZED = 1 << 7, C_PETRIFIED = 1 << 8,
    C_POISONED = 1 << 9, C_PRONE = 1 << 10, C_RESTRAINED = 1 << 11,
    C_STUNNED = 1 << 12, C_UNCONSCIOUS = 1 << 13,
    C_RAGING = 1 << 14,              /* engine condition, not SRD */
};

/* paralyzed/petrified/stunned/unconscious imply incapacitated */
uint16_t r5_conds_effective(uint16_t conds);
int      r5_can_act(uint16_t conds);

/* per-rest resource bits */
enum {
    USED_SECOND_WIND = 1 << 0,
    USED_ACTION_SURGE = 1 << 1,
    USED_BARDIC_INSP = 1 << 2,   /* simplified: one use per battle-rest */
};

/* classes (index into r5_classes) */
enum { R5C_BARD, R5C_ROGUE, R5C_RANGER, R5C_WIZARD, R5C_FIGHTER, R5C_CLERIC,
       R5C_BARBARIAN, R5C_DRUID, R5C_MONK, R5C_PALADIN, R5C_SORCERER,
       R5C_WARLOCK, R5C_COUNT, R5C_MONSTER = 255 };

/* ---------------------------------------------------------------- tables */

typedef struct {
    const char* name;
    R5DiceSpec dmg;
    R5DiceSpec versatile;      /* n==0 if not versatile */
    uint16_t props;
    uint8_t dmg_type;
    R5DiceSpec rider_dmg;      /* on-hit rider (Everburn fire, Stinger venom) */
    uint8_t rider_type;
    uint8_t rider_save_ab, rider_dc;   /* dc 0 = no save, full damage */
} R5Weapon;

/* per-level class resource pools, generator-emitted (0 where N/A) */
enum { R5R_RAGE, R5R_KI, R5R_SORC, R5R_LAY, R5R_SHAPE, R5R_PACT, R5R_COUNT };

/* racial trait bits (Character 2.0; set at creation from race data) */
enum {
    TR_LUCKY      = 1 << 0,   /* halfling: reroll natural 1s, keep the new roll */
    TR_RELENTLESS = 1 << 1,   /* half-orc: drop to 1 hp instead of 0, 1/rest */
    TR_SAVAGE     = 1 << 2,   /* half-orc: +1 weapon die on a crit */
    TR_FEY        = 1 << 3,   /* elf-kin: advantage vs charm, sleep-immune */
    TR_CUNNING    = 1 << 4,   /* gnome: adv on INT/WIS/CHA saves vs magic */
    TR_USED_RELENTLESS = 1 << 7,
};

typedef struct {
    uint8_t hit_die;
    uint8_t save_prof;               /* bit per ability */
    uint8_t prof_bonus[4];           /* [level], level 1..3 used */
    uint8_t slots[4][3];             /* [level][slot_level-1]    */
    uint8_t sneak_d6[4];             /* rogue only, else 0       */
    uint8_t rsrc[4][R5R_COUNT];      /* [level][pool]: rage/ki/sorc/lay/shape/pact */
    uint8_t pact_lvl[4];             /* warlock: the slot level of pact slots */
} R5Class;

/* a monster's attack: fixed to-hit, optional save-rider (imp sting) */
typedef struct {
    const char* name;
    int8_t to_hit;
    R5DiceSpec dmg;
    uint8_t dmg_type;
    R5DiceSpec rider_dmg;            /* n==0 if none */
    uint8_t rider_type;
    uint8_t rider_save_ab, rider_dc; /* half on save */
    uint8_t ranged;
} R5MAttack;

typedef struct {
    const char* name;
    uint8_t ac;
    int16_t hp;
    int8_t ab[6];
    uint16_t resist, immune;         /* damage-type bitmasks (1<<DT_x) */
    R5MAttack attacks[2];
    uint8_t n_attacks;
} R5Monster;

/* ---------------------------------------------------------------- creature */

typedef struct {
    const char* name;
    int8_t ab[6];
    int16_t hp, hpmax, temp_hp;
    uint8_t ac, level, cls;
    uint8_t save_prof;               /* PCs: from class; monsters: 0 */
    uint16_t conds;
    uint16_t resist, immune, vulnerable;
    uint8_t slots[3];
    uint8_t used;                    /* USED_* resource bits */
    uint8_t rsrc[R5R_COUNT];         /* live pools (see R5R_) */
    uint8_t traits;                  /* TR_* racial trait bits */
    uint8_t crit_min;                /* d20 >= this crits (20; Champion 19) */
    uint8_t heal_boost;              /* Life: +2+level on healing spells */
    uint8_t concentrating;           /* spell id + 1, or 0 */
} R5Creature;

static inline int r5_mod(int score) { return (score - 10) >> 1; }  /* floors */
int r5_prof(const R5Creature*);      /* proficiency bonus by class/level */

/* ---------------------------------------------------------------- rolls */

/* d20 with advantage state; rolls[] raw (1-2 dice), total = die USED (no mods) */
R5Dice r5_d20(R5RNG*, int flags);

typedef struct {
    R5Dice d20;                      /* raw d20(s), total = chosen die  */
    R5Dice bless;                    /* n==0 unless blessed             */
    int16_t bonus, total, target_ac;
    uint8_t hit, crit, fumble;
    R5Dice dmg;                      /* thrown only on hit              */
    int16_t damage;                  /* post resist/immune/vuln         */
    uint8_t dmg_type;
    R5Dice rider_dmg;                /* monster rider (poison etc.)     */
    R5Dice rider_save;               /* target's save vs the rider      */
    int16_t rider_damage;
    uint8_t rider_type, rider_saved;
} R5Attack;

/* PC weapon attack: STR melee / DEX ranged / best-of for finesse, + prof */
R5Attack r5_weapon_attack(R5RNG*, const R5Creature* a, const R5Creature* t,
                          const R5Weapon* w, int flags);
/* monster attack from its stat block */
R5Attack r5_monster_attack(R5RNG*, const R5Creature* a, const R5MAttack*,
                           const R5Creature* t, int flags);

typedef struct {
    R5Dice d20, bless;
    int16_t total, dc;
    uint8_t success;
} R5Save;

R5Save r5_save(R5RNG*, const R5Creature*, int ability, int dc, int flags);

/* apply damage honoring temp hp / resist / immune / vulnerable;
 * returns actual hp lost; sets C_UNCONSCIOUS on PCs at 0 */
int  r5_apply_damage(R5Creature*, int amount, uint8_t type);
void r5_heal(R5Creature*, int amount);           /* wakes unconscious PCs */
int  r5_conc_dc(int damage);                     /* max(10, dmg/2) */

/* ---- closed-form expectations (for AI utility scoring) ----
 * Integer math: probabilities in percent, damage expectations x100. */
int r5_hit_pct(int bonus, int ac, int advflags);     /* nat 1/20 honored */
int r5_crit_permille(int advflags);                  /* 50 / 98 / 2      */
int r5_ev_attack_x100(int bonus, int ac, int advflags, R5DiceSpec dmg);

/* ---------------------------------------------------------------- features */

int    r5_can_second_wind(const R5Creature*);
R5Dice r5_second_wind(R5RNG*, R5Creature*);      /* 1d10+level, marks used */
int    r5_can_action_surge(const R5Creature*);
void   r5_use_action_surge(R5Creature*);
int    r5_sneak_dice(const R5Creature*);         /* count of d6, 0 if none */
int    r5_spend_slot(R5Creature*, int slot_level);
void   r5_short_rest(R5Creature*);               /* reset per-rest resources */
void r5_refill(R5Creature*);             /* long rest: pools from class table */
int  r5_spend(R5Creature*, int pool, int n);
int  r5_can_rage(const R5Creature*);
void r5_start_rage(R5Creature*);
void r5_end_rage(R5Creature*);
int  r5_rage_bonus(const R5Creature*);   /* +2 STR melee dmg while C_RAGING */
int  r5_martial_die(const R5Creature*);
R5DiceSpec r5_smite_dice(int slot_level);
int  r5_lay_hands(R5Creature* pal, R5Creature* t, int amt);
int  r5_pact_cast(R5Creature*);
void r5_pact_rest(R5Creature*);
int  r5_savage_crit_dice(const R5Creature*);  /* extra crit weapon dice */

/* ---------------------------------------------------------------- data */

extern const R5Weapon r5_weapons[];
enum { R5W_DAGGER, R5W_SHORTSWORD, R5W_LONGSWORD, R5W_GREATSWORD, R5W_RAPIER,
       R5W_MACE, R5W_QUARTERSTAFF, R5W_SHORTBOW, R5W_LONGBOW, R5W_TRIDENT,
       R5W_EVERBURN, R5W_STINGER,   /* homebrew loot (tools/srd/overrides.py) */
       R5W_COUNT };

extern const R5Class r5_classes[R5C_COUNT];

/* subclasses: id table generated into srd_ids.h; passive bits applied at
 * sheet build, triggered features interpreted by the choice/combat stones */
enum { SUBP_CRIT19 = 1 << 0, SUBP_HEAL_DISCIPLE = 1 << 1, SUBP_TOUGH = 1 << 2 };
typedef struct {
    const char* name;      /* display, <=10 chars */
    uint8_t cls, level;    /* class + choice level */
    uint8_t passive;       /* SUBP_* always-on bits */
    uint8_t srd;
    uint32_t prepared;     /* always-prepared spells: bitmask into r5_spells */
} R5Subclass;
extern const R5Subclass r5_subclasses[];
extern const uint8_t r5_subclass_of_class[R5C_COUNT][4];
extern const uint8_t r5_subclass_level[R5C_COUNT];

/* spells (prologue set, generated from SRD data) */
typedef struct {
    const char* name;
    uint8_t level;              /* 0 = cantrip */
    uint8_t bonus_action;
    uint8_t concentration;
    uint8_t attack;             /* resolves as a ranged spell attack */
    uint8_t save_ab;            /* 0xFF = no save */
    uint8_t save_half;          /* save halves (else negates) */
    R5DiceSpec dice;
    uint8_t count;              /* separate hits (magic missile darts) */
    uint8_t dmg_type;
    uint8_t heal;               /* heals instead of damages */
    uint8_t add_mod;            /* add caster ability mod to the dice */
    uint8_t kind;               /* R5K_* shape of the effect */
    uint8_t cond_bit;           /* C_* bit index applied on hit/fail, 0xFF none */
    uint8_t rounds;             /* condition/buff duration in rounds (0 = scene) */
    uint8_t tgt;                /* 1..3 = foes hit; R5T_PARTY / R5T_SELF */
    uint16_t classes;           /* class list bitmask, R5C_ order */
} R5Spell;

enum { R5K_ATTACK, R5K_SAVE, R5K_HEAL, R5K_BUFF, R5K_CONDITION,
       R5K_MULTI, R5K_POOL, R5K_UTILITY };
enum { R5T_PARTY = 250, R5T_SELF = 251 };

extern const R5Spell r5_spells[];
#include "srd_ids.h"            /* generated R5S_* ids, legacy 13 first */

extern const R5Monster r5_monsters[];
/* IMP/BOAR are true SRD stat blocks (test-validated); the rest are our
 * prologue homebrew from tools/srd/overrides.py */
enum { R5M_IMP, R5M_BOAR, R5M_DEVOURER, R5M_CAMBION, R5M_FLAYER, R5M_ZHALK,
       R5M_LESSER_IMP, R5M_LESSER_BOAR, R5M_THRALL, R5M_COUNT };

#endif
