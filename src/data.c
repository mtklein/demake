#include "gba.h"
#include "game.h"
#include "assets.h"
#include "field.h"
#include "rules.h"

Game G;

/*                         hp  mp atk def mag spd */
static const u8 base[CLS_COUNT][6] = {
    [CLS_BARD]    = { 12, 10,  7,  5,  7, 30 },
    [CLS_ROGUE]   = { 13,  0,  8,  5,  3, 34 },
    [CLS_RANGER]  = { 14,  4,  8,  6,  5, 31 },
    [CLS_WIZARD]  = { 10, 14,  5,  4,  9, 28 },
    [CLS_FIGHTER] = { 16,  0,  9,  7,  2, 29 },
    [CLS_CLERIC]  = { 13, 10,  7,  6,  7, 27 },
    [CLS_BARBARIAN] = { 17,  0, 10,  6,  1, 28 },
    [CLS_DRUID]     = { 13,  9,  7,  6,  7, 28 },
    [CLS_MONK]      = { 13,  2,  8,  5,  4, 35 },
    [CLS_PALADIN]   = { 15,  4,  8,  7,  4, 26 },
    [CLS_SORCERER]  = {  9, 15,  4,  4, 10, 29 },
    [CLS_WARLOCK]   = { 11, 12,  6,  5,  8, 29 }
};
static const u8 grow[CLS_COUNT][6] = {   /* per level: hp mp atk def mag spd */
    [CLS_BARD]    = { 5, 4, 1, 1, 2, 2 },
    [CLS_ROGUE]   = { 6, 0, 2, 1, 0, 3 },
    [CLS_RANGER]  = { 6, 2, 2, 1, 1, 2 },
    [CLS_WIZARD]  = { 4, 5, 1, 1, 3, 2 },
    [CLS_FIGHTER] = { 7, 0, 2, 2, 0, 2 },
    [CLS_CLERIC]  = { 5, 4, 1, 1, 2, 2 },
    [CLS_BARBARIAN] = { 8, 0, 2, 1, 0, 2 },
    [CLS_DRUID]     = { 5, 4, 1, 1, 2, 2 },
    [CLS_MONK]      = { 6, 1, 2, 1, 1, 3 },
    [CLS_PALADIN]   = { 7, 2, 2, 2, 1, 1 },
    [CLS_SORCERER]  = { 4, 5, 1, 1, 3, 2 },
    [CLS_WARLOCK]   = { 5, 4, 1, 1, 3, 2 },
};
static const u16 xp_next[4] = { 0, 300, 900, 65535 };   /* 5e: L2 300, L3 900 */

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

void party5_refresh(int i);   /* party5.c: keep the 5e twin in step */

void party_init(int cls, const char* name) {
    G.nparty = 1;
    G.potions = 2;
    G.revivify = 0;
    G.everburn = 0;
    G.flags = 0;
    PMember* p = &G.pm[0];
    strcpy8(p->name, name);
    p->cls = (u8)cls; p->level = 1; p->xp = 0; p->subclass = 255;
    p->face = (u8)G.origin;   /* member 0 art identity = its origin */
    /* origins whose subclass arrives at level 1 get it now (Char 2.0) */
    {
        static const unsigned char sk2[CLS_COUNT][2] = {
            [CLS_BARD]={SK_PERSUASION,SK_PERFORMANCE},[CLS_ROGUE]={SK_STEALTH,SK_SLEIGHT_OF_HAND},
            [CLS_RANGER]={SK_SURVIVAL,SK_PERCEPTION},[CLS_WIZARD]={SK_ARCANA,SK_HISTORY},
            [CLS_FIGHTER]={SK_ATHLETICS,SK_INTIMIDATION},[CLS_CLERIC]={SK_RELIGION,SK_MEDICINE},
            [CLS_BARBARIAN]={SK_ATHLETICS,SK_INTIMIDATION},[CLS_DRUID]={SK_NATURE,SK_ANIMAL_HANDLING},
            [CLS_MONK]={SK_ACROBATICS,SK_STEALTH},[CLS_PALADIN]={SK_RELIGION,SK_PERSUASION},
            [CLS_SORCERER]={SK_ARCANA,SK_DECEPTION},[CLS_WARLOCK]={SK_ARCANA,SK_DECEPTION},
        };
        p->skills = (1u << sk2[cls][0]) | (1u << sk2[cls][1]);
        p->expert = (cls == CLS_ROGUE) ? p->skills : 0;   /* rogue Expertise */
    }
    if (G.origin == ORIG_SHADOW) p->subclass = R5SUB_DOMAIN_OF_MASKS;
    else if (G.origin == ORIG_WYLL) p->subclass = R5SUB_FIEND;
    if (G_DEMO_LEVEL >= 2) { char nm[16]; party_give_xp(
        G_DEMO_LEVEL >= 3 ? 900 : 300, nm); }   /* test hook: pre-level */
    set_stats(p);
    p->hp = p->hpmax; p->mp = p->mpmax;
    {   /* the avatar walks in its identity's silhouette */
        MemberLook L = member_look(p->face, cls);
        field_set_hero(L.objt, L.pal);
    }
    G.tactics[0] = G.tactics[1] = G.tactics[2] = TAC_ORDERS;
    {
        int party5_default_weapon(int cls);
        G.weapon[0] = (u8)party5_default_weapon(cls);
        G.nwinv = 0;
    }
    party5_refresh(0);
}

void loot_weapon(int w) {
    if (G.nwinv < 8) G.winv[G.nwinv++] = (u8)w;
}

void party_add_laezel(void) {
    PMember* p = &G.pm[G.nparty++];
    strcpy8(p->name, "LAE'ZEL");
    p->cls = CLS_FIGHTER; p->level = 1; p->xp = G.pm[0].xp; p->face = ORIG_LAEZEL;
    set_stats(p);
    p->hp = p->hpmax; p->mp = p->mpmax;
    G.flags |= GF_LAEZEL;
    {
        int party5_default_weapon(int cls);
        G.weapon[G.nparty - 1] = (u8)party5_default_weapon(CLS_FIGHTER);
    }
    { char nm[16]; party_give_xp(0, nm); }   /* join at the party's level */
    party5_refresh(G.nparty - 1);
}

void party_add_shadowheart(void) {
    PMember* p = &G.pm[G.nparty++];
    strcpy8(p->name, "SHADOW.");
    p->cls = CLS_CLERIC; p->level = 1; p->xp = G.pm[0].xp; p->face = ORIG_SHADOW;
    set_stats(p);
    p->hp = p->hpmax; p->mp = p->mpmax;
    G.revivify += 1;
    G.flags |= GF_SH_FREED;
    {
        int party5_default_weapon(int cls);
        G.weapon[G.nparty - 1] = (u8)party5_default_weapon(CLS_CLERIC);
    }
    { char nm[16]; party_give_xp(0, nm); }   /* join at the party's level */
    party5_refresh(G.nparty - 1);
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
