/* 5e party state: R5Creature per member, persistent across encounters.
 * Canonical for Battle 2.0; the FF4 battle keeps using PMember until cutover.
 * Character generation: SRD standard array + class loadouts (our choices). */
#include "gba.h"
#include "game.h"
#include "rules.h"
#include "party5.h"

R5Creature party5[3];
static u8 built[3];

/*                              str dex con int wis cha   ac  weapon */
static const s8 gen_ab[6][6] = {
    [CLS_BARD]    = { 8, 14, 13, 10, 12, 15 },
    [CLS_ROGUE]   = { 8, 15, 13, 14, 10, 12 },
    [CLS_RANGER]  = { 12, 15, 13, 8, 14, 10 },
    [CLS_WIZARD]  = { 8, 13, 14, 15, 12, 10 },
    [CLS_FIGHTER] = { 15, 13, 14, 10, 12, 8 },
    [CLS_CLERIC]  = { 13, 10, 14, 8, 15, 12 },
};
/* leather 11+DEX / none 10+DEX / chain mail 16 / chain shirt+shield 15 */
static u8 class_ac(int cls, int dexmod) {
    switch (cls) {
        case CLS_WIZARD:  return (u8)(10 + dexmod);
        case CLS_FIGHTER: return 16;
        case CLS_CLERIC:  return 15;
        default:          return (u8)(11 + dexmod);
    }
}
static const u8 gen_weapon[6] = {
    [CLS_BARD] = R5W_DAGGER,   /* the rapier is out there, on a duelist */
    [CLS_ROGUE] = R5W_DAGGER,
    [CLS_RANGER] = R5W_LONGBOW, [CLS_WIZARD] = R5W_QUARTERSTAFF,
    [CLS_FIGHTER] = R5W_GREATSWORD, [CLS_CLERIC] = R5W_MACE,
};
static const u8 cast_ab_tab[6] = {
    [CLS_BARD] = R5_CHA, [CLS_ROGUE] = R5_INT, [CLS_RANGER] = R5_WIS,
    [CLS_WIZARD] = R5_INT, [CLS_FIGHTER] = R5_STR, [CLS_CLERIC] = R5_WIS,
};

int party5_weapon(int i) { return G.weapon[i]; }
int party5_default_weapon(int cls) { return gen_weapon[cls]; }
int party5_cast_ab(int cls) { return cast_ab_tab[cls]; }
int party5_spell_dc(const R5Creature* c) {
    return 8 + r5_prof(c) + r5_mod(c->ab[cast_ab_tab[c->cls]]);
}
int party5_spell_atk(const R5Creature* c) {
    return r5_prof(c) + r5_mod(c->ab[cast_ab_tab[c->cls]]);
}

static s16 max_hp(int cls, int level, int conmod) {
    const R5Class* rc = &r5_classes[cls];
    int hp = rc->hit_die + conmod;                    /* level 1: max die */
    for (int l = 2; l <= level; l++)
        hp += rc->hit_die / 2 + 1 + conmod;           /* then average */
    return (s16)hp;
}

/* build/refresh member i from G.pm[i]; preserves damage + spent slots */
void party5_refresh(int i) {
    PMember* p = &G.pm[i];
    R5Creature* c = &party5[i];
    int missing = 0, spent[3] = { 0, 0, 0 };
    u8 used = 0, conds = 0;
    if (built[i]) {
        missing = c->hpmax - c->hp;
        used = c->used;
        conds = (u8)(c->conds & C_UNCONSCIOUS);
        for (int s = 0; s < 3; s++) {
            int oldmax = r5_classes[c->cls].slots[c->level][s];
            spent[s] = oldmax - c->slots[s];
        }
    }
    c->name = p->name;
    c->cls = p->cls;
    c->level = p->level;
    for (int a = 0; a < 6; a++) c->ab[a] = gen_ab[p->cls][a];
    c->hpmax = max_hp(p->cls, p->level, r5_mod(c->ab[R5_CON]));
    c->hp = (s16)(c->hpmax - missing);
    if (c->hp < 0) c->hp = 0;
    c->temp_hp = 0;
    c->ac = class_ac(p->cls, r5_mod(c->ab[R5_DEX]));
    c->save_prof = r5_classes[p->cls].save_prof;
    c->conds = conds;
    c->resist = c->immune = c->vulnerable = 0;
    for (int s = 0; s < 3; s++) {
        int mx = r5_classes[p->cls].slots[p->level][s];
        int v = mx - spent[s];
        c->slots[s] = (u8)(v < 0 ? 0 : v);
    }
    c->used = used;
    c->concentrating = 0;
    built[i] = 1;
}

void party5_refresh_all(void) {
    for (int i = 0; i < G.nparty; i++) party5_refresh(i);
}

void party5_heal_full(void) {
    for (int i = 0; i < G.nparty; i++) {
        R5Creature* c = &party5[i];
        if (!built[i]) continue;
        c->conds &= (u16)~C_UNCONSCIOUS;
        c->hp = c->hpmax;
        c->used = 0;
        for (int s = 0; s < 3; s++)
            c->slots[s] = r5_classes[c->cls].slots[c->level][s];
    }
}
