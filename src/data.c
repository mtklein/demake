#include "gba.h"
#include "game.h"
#include "battle.h"
#include "assets.h"

Game G;

/*                         hp  mp atk def mag spd */
static const u8 base[6][6] = {
    [CLS_BARD]    = { 12, 10,  7,  5,  7, 30 },
    [CLS_ROGUE]   = { 13,  0,  8,  5,  3, 34 },
    [CLS_RANGER]  = { 14,  4,  8,  6,  5, 31 },
    [CLS_WIZARD]  = { 10, 14,  5,  4,  9, 28 },
    [CLS_FIGHTER] = { 16,  0,  9,  7,  2, 29 },
    [CLS_CLERIC]  = { 13, 10,  7,  6,  7, 27 },
};
static const u8 grow[6][6] = {   /* per level: hp mp atk def mag spd */
    [CLS_BARD]    = { 5, 4, 1, 1, 2, 2 },
    [CLS_ROGUE]   = { 6, 0, 2, 1, 0, 3 },
    [CLS_RANGER]  = { 6, 2, 2, 1, 1, 2 },
    [CLS_WIZARD]  = { 4, 5, 1, 1, 3, 2 },
    [CLS_FIGHTER] = { 7, 0, 2, 2, 0, 2 },
    [CLS_CLERIC]  = { 5, 4, 1, 1, 2, 2 },
};
static const u16 xp_next[4] = { 0, 100, 400, 65535 };

static void set_stats(PMember* p) {
    const u8* b = base[p->cls];
    const u8* g = grow[p->cls];
    int l = p->level - 1;
    p->hpmax = (s16)(b[0] + g[0] * l);
    p->mpmax = (s16)(b[1] + g[1] * l);
    p->atk = (u8)(b[2] + g[2] * l);
    p->def = (u8)(b[3] + g[3] * l);
    p->mag = (u8)(b[4] + g[4] * l);
    p->spd = (u8)(b[5] + g[5] * l);
}

static void strcpy8(char* d, const char* s) {
    int i = 0;
    while (s[i] && i < 7) { d[i] = s[i]; i++; }
    d[i] = 0;
}

void party_init(int cls, const char* name) {
    G.nparty = 1;
    G.potions = 2;
    G.revivify = 0;
    G.everburn = 0;
    G.flags = 0;
    PMember* p = &G.pm[0];
    strcpy8(p->name, name);
    p->cls = (u8)cls; p->level = 1; p->xp = 0;
    set_stats(p);
    p->hp = p->hpmax; p->mp = p->mpmax;
}

void party_add_laezel(void) {
    PMember* p = &G.pm[G.nparty++];
    strcpy8(p->name, "LAE'ZEL");
    p->cls = CLS_FIGHTER; p->level = 1; p->xp = G.pm[0].xp;
    set_stats(p);
    p->hp = p->hpmax; p->mp = p->mpmax;
    G.flags |= GF_LAEZEL;
}

void party_add_shadowheart(void) {
    PMember* p = &G.pm[G.nparty++];
    strcpy8(p->name, "SHADOW.");
    p->cls = CLS_CLERIC; p->level = 1; p->xp = G.pm[0].xp;
    set_stats(p);
    p->hp = p->hpmax; p->mp = p->mpmax;
    G.revivify += 1;
    G.flags |= GF_SH_FREED;
}

int party_give_xp(u16 xp, char* names) {
    int ups = 0; names[0] = 0;
    for (int i = 0; i < G.nparty; i++) {
        PMember* p = &G.pm[i];
        p->xp = (u16)(p->xp + xp);
        while (p->level < 3 && p->xp >= xp_next[p->level]) {
            int oldhp = p->hpmax, oldmp = p->mpmax;
            u8 eb = (G.everburn && p->cls == CLS_FIGHTER) ? 4 : 0;
            p->level++;
            set_stats(p);
            p->atk = (u8)(p->atk + eb);
            p->hp = (s16)(p->hp + p->hpmax - oldhp);
            p->mp = (s16)(p->mp + p->mpmax - oldmp);
            if (ups) { char* d = names; while (*d) d++; *d++ = ' '; *d = 0; }
            char* d = names; while (*d) d++;
            const char* s = p->name;
            while (*s) *d++ = *s++;
            *d = 0;
            ups++;
        }
    }
    return ups;
}

void party_heal_full(void) {
    for (int i = 0; i < G.nparty; i++) {
        G.pm[i].hp = G.pm[i].hpmax;
        G.pm[i].mp = G.pm[i].mpmax;
    }
    /* keep the 5e party in step (Battle 2.0) */
    void party5_heal_full(void);
    party5_heal_full();
}

/* ------------------------------------------------ enemies ------ */

/* Each enemy uses its own battle sprite once the art exists; otherwise it
 * falls back to the imp sprite (the #ifdef picks whichever define is present). */
const EnemyDef e_imp = {
    "Imp", 6, 5, 2, 5, 26, 40, AI_IMP,
    OBJT_B_IMP, OBJS_B_IMP, OBJP_B_IMP, OBJTPF_B_IMP, 16, 16,
};
const EnemyDef e_boar = {
    "Hellsboar", 9, 7, 3, 2, 22, 45, AI_BOAR,
#ifdef OBJT_B_BOAR
    OBJT_B_BOAR, OBJS_B_BOAR, OBJP_B_BOAR, OBJTPF_B_BOAR, 32, 16,
#else
    OBJT_B_IMP, OBJS_B_IMP, OBJP_B_IMP, OBJTPF_B_IMP, 16, 16,
#endif
};
const EnemyDef e_thrall = {
    "Thrall", 8, 6, 2, 2, 24, 60, AI_THRALL,
#ifdef OBJT_B_THRALL
    OBJT_B_THRALL, OBJS_B_THRALL, OBJP_B_THRALL, OBJTPF_B_THRALL, 16, 32,
#else
    OBJT_B_IMP, OBJS_B_IMP, OBJP_B_IMP, OBJTPF_B_IMP, 16, 16,
#endif
};
const EnemyDef e_cambion = {
    "Cambion", 40, 10, 6, 6, 30, 150, AI_CAMBION,
#ifdef OBJT_B_CAMBION
    OBJT_B_CAMBION, OBJS_B_CAMBION, OBJP_B_CAMBION, OBJTPF_B_CAMBION, 32, 32,
#else
    OBJT_B_IMP, OBJS_B_IMP, OBJP_B_IMP, OBJTPF_B_IMP, 16, 16,
#endif
};
const EnemyDef e_zhalk = {
    "Cmdr Zhalk", 150, 13, 7, 6, 32, 300, AI_ZHALK,
#ifdef OBJT_B_ZHALK
    OBJT_B_ZHALK, OBJS_B_ZHALK, OBJP_B_ZHALK, OBJTPF_B_ZHALK, 32, 64,
#else
    OBJT_B_IMP, OBJS_B_IMP, OBJP_B_IMP, OBJTPF_B_IMP, 16, 16,
#endif
};
const EnemyDef e_flayer = {
    "Mindflayer", 85, 11, 5, 10, 33, 0, AI_FLAYER_ALLY,
#ifdef OBJT_B_FLAYER
    OBJT_B_FLAYER, OBJS_B_FLAYER, OBJP_B_FLAYER, OBJTPF_B_FLAYER, 32, 32,
#else
    OBJT_FLAYERF, 1, 5, 2, 16, 16,
#endif
};
const EnemyDef e_us = {
    "Us", 21, 7, 3, 3, 28, 0, AI_US_ALLY,
#ifdef OBJT_B_DEVOURER
    OBJT_B_DEVOURER, OBJS_B_DEVOURER, 3, OBJTPF_B_DEVOURER, 32, 32,
#else
    OBJT_US, 1, 3, 4, 16, 16,
#endif
};

const Formation form_deck = {
    { &e_imp, &e_imp, &e_imp }, { 30, 14, 34 }, { 52, 76, 100 }, 3, BF_ALLY_US,
};
const Formation form_thralls = {
    { &e_thrall, &e_thrall }, { 28, 20 }, { 60, 92 }, 2, 0,
};
const Formation form_helm = {
    { &e_zhalk, &e_imp, &e_imp, &e_boar }, { 16, 56, 60, 52 }, { 36, 44, 116, 78 }, 4,
    BF_HELM | BF_ALLY_US | BF_ALLY_FLAYER | BF_NO_FLEE,
};
