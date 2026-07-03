/* FF4-style side-view battle: enemies left, party right, ATB-lite (wait mode). */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "game.h"
#include "battle.h"
#include "audio.h"

#define ME(t, h, v, p) ((u16)((t) | ((h) << 10) | ((v) << 11) | ((p) << 12)))
#define TXT ((vu16*)SCREENBLOCK(30))

enum { PO_IDLE, PO_SWING1, PO_SWING2, PO_CAST, PO_HURT, PO_KO, PO_VICTORY };

enum {
    SK_MOCKERY, SK_HEALWORD, SK_INSPIRE,
    SK_HIDE, SK_STEAL,
    SK_MARK, SK_ENSNARE,
    SK_FIREBOLT, SK_MMISSILE, SK_BURNHANDS, SK_SLEEP,
    SK_SURGE,
    SK_CURE, SK_GBOLT, SK_BLESS,
};

typedef struct {
    const char* name;
    u8 kind, skill, mp, target;   /* target: 0 enemy, 1 party, 2 all enemies, 3 none */
} Cmd;

#define K_FIGHT 0
#define K_SKILL 1
#define K_MAGIC 2
#define K_ITEM  3
#define K_NERVE 4

static const Cmd kit_bard[] = {
    {"Fight", K_FIGHT, 0, 0, 0}, {"Mock", K_SKILL, SK_MOCKERY, 0, 0},
    {"H.Word", K_SKILL, SK_HEALWORD, 3, 1}, {"Inspir", K_SKILL, SK_INSPIRE, 4, 1},
    {"Item", K_ITEM, 0, 0, 3},
};
static const Cmd kit_rogue[] = {
    {"Fight", K_FIGHT, 0, 0, 0}, {"Hide", K_SKILL, SK_HIDE, 0, 3},
    {"Steal", K_SKILL, SK_STEAL, 0, 0}, {"Item", K_ITEM, 0, 0, 3},
};
static const Cmd kit_ranger[] = {
    {"Fight", K_FIGHT, 0, 0, 0}, {"Mark", K_SKILL, SK_MARK, 0, 0},
    {"Ensnar", K_SKILL, SK_ENSNARE, 4, 0}, {"Item", K_ITEM, 0, 0, 3},
};
static const Cmd magic_wiz[] = {
    {"M.Msl", K_SKILL, SK_MMISSILE, 3, 0}, {"B.Hand", K_SKILL, SK_BURNHANDS, 5, 2},
    {"Sleep", K_SKILL, SK_SLEEP, 4, 0},
};
static const Cmd kit_wizard[] = {
    {"Fight", K_FIGHT, 0, 0, 0}, {"F.Bolt", K_SKILL, SK_FIREBOLT, 0, 0},
    {"Magic", K_MAGIC, 0, 0, 3}, {"Item", K_ITEM, 0, 0, 3},
};
static const Cmd kit_fighter[] = {
    {"Fight", K_FIGHT, 0, 0, 0}, {"Surge", K_SKILL, SK_SURGE, 0, 3},
    {"Item", K_ITEM, 0, 0, 3},
};
static const Cmd magic_cleric[] = {
    {"Cure", K_SKILL, SK_CURE, 3, 1}, {"G.Bolt", K_SKILL, SK_GBOLT, 4, 0},
    {"Bless", K_SKILL, SK_BLESS, 4, 3},
};
static const Cmd kit_cleric[] = {
    {"Fight", K_FIGHT, 0, 0, 0}, {"Magic", K_MAGIC, 0, 0, 3},
    {"Item", K_ITEM, 0, 0, 3},
};
static const Cmd cmd_nerve = {"Nerve!", K_NERVE, 0, 0, 3};

static const Cmd* kits[6] = { kit_bard, kit_rogue, kit_ranger, kit_wizard, kit_fighter, kit_cleric };
static const u8 kitn[6] = { 5, 4, 4, 4, 3, 3 };

/* ------------------------------------------------------------------ units */

#define MAXU 12
typedef struct {
    const char* name;
    s16 hp, hpmax, mp, mpmax;
    u8 atk, def, mag, spd;
    u16 atb;
    u8 side, alive, pi;
    const EnemyDef* ed;
    s16 sx, sy, ox;
    u8 pose, actf;
    s8 atkmod;
    u8 sleep, hidden, blessed, inspired, gbolt, marked, surged;
} BU;

static BU U[MAXU];
static int NU;
static int helm, rounds, timeout_flag, menu_unit = -1;
static int xpsum, coin_stolen;
static u32 bt;

typedef struct { u16 objt; u8 objs, tpf, pal; } PGfx;
static PGfx pg[3];

static void init_pgfx(void) {
#ifdef OBJT_B_HERO
    pg[0] = (PGfx){ OBJT_B_HERO, OBJS_B_HERO, OBJTPF_B_HERO, 0 };
#else
    pg[0] = (PGfx){ OBJT_HERO + 16, 1, 0, 0 };
#endif
#ifdef OBJT_B_LAEZEL
    pg[1] = (PGfx){ OBJT_B_LAEZEL, OBJS_B_LAEZEL, OBJTPF_B_LAEZEL, 1 };
#else
    pg[1] = (PGfx){ OBJT_LAEZEL + 16, 1, 0, 1 };
#endif
#ifdef OBJT_B_SHADOW
    pg[2] = (PGfx){ OBJT_B_SHADOW, OBJS_B_SHADOW, OBJTPF_B_SHADOW, 2 };
#else
    pg[2] = (PGfx){ OBJT_SHADOW + 16, 1, 0, 2 };
#endif
}

static int pgfx_for(int pi) {
    u8 c = G.pm[pi].cls;
    return c == CLS_FIGHTER ? 1 : c == CLS_CLERIC ? 2 : 0;
}

/* ------------------------------------------------------------------ ui */

static void hp4(char* b, int v) {
    if (v < 0) v = 0;
    b[4] = 0;
    for (int i = 3; i >= 0; i--) {
        if (v || i == 3) { b[i] = (char)('0' + v % 10); v /= 10; }
        else b[i] = ' ';
    }
}

static void draw_status(void) {
    for (int i = 0; i < G.nparty; i++) {
        BU* u = &U[i];
        int y = 16 + i;
        int pal = (i == menu_unit) ? 1 : 0;
        char nm[7];
        int k = 0;
        while (u->name[k] && k < 6) { nm[k] = u->name[k]; k++; }
        while (k < 6) nm[k++] = ' ';
        nm[6] = 0;
        txt_put(17, y, nm, pal);
        char hb[5];
        hp4(hb, u->hp);
        txt_put(23, y, hb, (u->hp * 4 < u->hpmax) ? 1 : 0);
        int v = u->alive ? u->atb * 16 / 1000 : 0;
        if (v > 16) v = 16;
        int gold = (v >= 16);
        int t0 = v > 8 ? 8 : v, t1 = v > 8 ? v - 8 : 0;
        TXT[y * 32 + 27] = ME(TILE_WIN + 16 + (gold ? 9 : t0), 0, 0, 0);
        TXT[y * 32 + 28] = ME(TILE_WIN + 16 + (gold ? 9 : t1), 0, 0, 0);
    }
}

static int en_wide = 1;   /* current enemy-name window width mode */

static void draw_enemy_names(int wide) {
    en_wide = wide;
    int w = wide ? 16 : 8;
    const char* names[4]; int counts[4], nn = 0;
    for (int i = 0; i < NU; i++) {
        if (U[i].side != 1 || !U[i].alive) continue;
        int f = -1;
        for (int j = 0; j < nn; j++) if (names[j] == U[i].name) f = j;
        if (f < 0 && nn < 4) { names[nn] = U[i].name; counts[nn] = 1; nn++; }
        else if (f >= 0) counts[f]++;
    }
    for (int j = 0; j < 3; j++) {
        txt_clear(1, 16 + j, w - 2, 1);
        if (j < nn) {
            char line[14]; int k = 0;
            const char* s = names[j];
            while (*s && k < (wide ? 10 : 5)) line[k++] = *s++;
            if (counts[j] > 1) { line[k++] = ':'; line[k++] = (char)('0' + counts[j]); }
            line[k] = 0;
            txt_put(1, 16 + j, line, 0);
        }
    }
}

static void ui_base(void) {
    win_draw(0, 15, 16, 5);
    win_draw(16, 15, 14, 5);
    draw_enemy_names(1);
    draw_status();
}

static void rounds_show(void) {
    if (!helm) return;
    win_draw(26, 0, 4, 3);
    char b[3] = { ' ', ' ', 0 };
    if (rounds >= 10) { b[0] = (char)('0' + rounds / 10); b[1] = (char)('0' + rounds % 10); }
    else b[1] = (char)('0' + (rounds < 0 ? 0 : rounds));
    txt_put(27, 1, b, rounds <= 3 ? 1 : 0);
}

static void msg_show(const char* s) {
    win_clear(0, 0, 30, 3);
    win_draw(0, 0, 26, 3);
    char buf[25];
    int i = 0;
    while (s[i] && i < 24) { buf[i] = s[i]; i++; }
    buf[i] = 0;
    txt_put(1, 1, buf, 0);
    rounds_show();
}
static void msg_hide(void) {
    win_clear(0, 0, 30, 3);
    rounds_show();
}

/* ------------------------------------------------------------------ popups */

#define OBJ_PU 40
typedef struct { int x, y, val, pal, t; } Popup;
static Popup pu[4];

static void popup_start(int x, int y, int val, int pal) {
    for (int i = 0; i < 4; i++)
        if (!pu[i].t) { pu[i] = (Popup){ x, y, val, pal, 36 }; return; }
}

static void popup_tick(void) {
    for (int i = 0; i < 4; i++) {
        Popup* p = &pu[i];
        if (!p->t) { continue; }
        p->t--;
        int rise = p->t > 24 ? (36 - p->t) : 12;
        char b[5];
        hp4(b, p->val);
        int n = 0, digs[4] = { 0 };
        for (int k = 0; k < 4; k++) if (b[k] != ' ') digs[n++] = b[k] - '0';
        int x0 = p->x - n * 3;
        for (int d = 0; d < 4; d++) {
            if (d < n && p->t)
                obj_set(OBJ_PU + i * 4 + d, x0 + d * 7, p->y - rise, 0,
                        OBJ_TILE_COUNT + digs[d], p->pal, 0);
            else obj_hide(OBJ_PU + i * 4 + d);
        }
        if (!p->t)
            for (int d = 0; d < 4; d++) obj_hide(OBJ_PU + i * 4 + d);
    }
}

/* ------------------------------------------------------------------ draw */

static void draw_units(void) {
    for (int i = 0; i < NU; i++) {
        BU* u = &U[i];
        int obj = OBJ_BATTLE + i;
        if (!u->alive && u->side != 0) { obj_hide(obj); continue; }
        if (u->side == 0) {
            PGfx* g = &pg[pgfx_for(u->pi)];
            int pose = u->alive ? u->pose : PO_KO;
            int tile = g->objt + (g->tpf ? pose * g->tpf : 0);
            obj_set(obj, u->sx + u->ox, u->sy, g->objs, tile, g->pal, 2);
        } else {
            const EnemyDef* d = u->ed;
            int f = u->actf ? (d->tpf > 4 || 1) : ((bt >> 5) & 1);
            if (d->tpf == 4) f = u->actf ? 1 : ((bt >> 4) & 1);
            obj_set(obj, u->sx + u->ox, u->sy, d->objs, d->objt + (f ? d->tpf : 0),
                    d->pal, 2);
        }
    }
}

static void pump(int n) {
    while (n--) {
        bt++;
        frame();
        popup_tick();
        draw_units();
        draw_status();
    }
}

static void msg_wait(const char* s) {
    msg_show(s);
    int t = 0;
    for (;;) {
        pump(1);
        if (key_hit() & KEY_A) break;
        if (G_DEMO && ++t >= 30) break;
    }
    msg_hide();
}

/* ------------------------------------------------------------------ setup */

static void backdrop(void) {
    for (int ty = 0; ty < 20; ty++)
        for (int tx = 0; tx < 32; tx++) {
            int mx = tx >> 1, my = ty >> 1;
            int mt;
            if (my < 2) mt = MT_WALL_F;
            else if (my == 2 && (mx == 2 || mx == 3 || mx == 10 || mx == 11)) mt = MT_WINDOW;
            else if (my == 2) mt = MT_WALL_F;
            else mt = ((mx * 5 + my * 3) % 7 == 0) ? MT_FLOOR_V : MT_FLOOR_A;
            const u16* e = &metatile_lut[mt * 4];
            vu16* sb = SCREENBLOCK(24 + (tx >> 5) + ((ty >> 5) << 1));
            sb[(ty & 31) * 32 + (tx & 31)] = e[((ty & 1) << 1) | (tx & 1)];
        }
    REG_BG2HOFS = 0;
    REG_BG2VOFS = 0;
}

static void add_party_unit(int pi, int slot) {
    PMember* p = &G.pm[pi];
    BU* u = &U[NU++];
    *u = (BU){ 0 };
    u->name = p->name;
    u->hp = p->hp; u->hpmax = p->hpmax; u->mp = p->mp; u->mpmax = p->mpmax;
    u->atk = p->atk; u->def = p->def; u->mag = p->mag; u->spd = p->spd;
    u->side = 0; u->alive = p->hp > 0; u->pi = (u8)pi;
    u->sx = 184; u->sy = (s16)(22 + slot * 30);
    u->pose = u->alive ? PO_IDLE : PO_KO;
    u->atb = (u16)rnd_range(400);
}

static BU* add_enemy(const EnemyDef* d, int x, int y, int side) {
    BU* u = &U[NU++];
    *u = (BU){ 0 };
    u->name = d->name;
    u->hp = u->hpmax = d->hp;
    u->atk = d->atk; u->def = d->def; u->mag = d->mag; u->spd = d->spd;
    u->side = (u8)side; u->alive = 1; u->ed = d;
    u->sx = (s16)x; u->sy = (s16)y;
    u->atb = (u16)rnd_range(300);
    return u;
}

/* ------------------------------------------------------------------ helpers */

static int enemies_alive(void) {
    int n = 0;
    for (int i = 0; i < NU; i++) if (U[i].side == 1 && U[i].alive) n++;
    return n;
}
static int party_alive(void) {
    int n = 0;
    for (int i = 0; i < NU; i++) if (U[i].side == 0 && U[i].alive) n++;
    return n;
}
static BU* first_enemy(void) {
    for (int i = 0; i < NU; i++) if (U[i].side == 1 && U[i].alive) return &U[i];
    return 0;
}
static BU* find_flayer(void) {
    for (int i = 0; i < NU; i++)
        if (U[i].side == 2 && U[i].alive && U[i].ed->ai == AI_FLAYER_ALLY) return &U[i];
    return 0;
}
static BU* find_zhalk(void) {
    for (int i = 0; i < NU; i++)
        if (U[i].side == 1 && U[i].alive && U[i].ed->ai == AI_ZHALK) return &U[i];
    return 0;
}
static BU* rand_party(void) {
    BU* c[4]; int n = 0;
    for (int i = 0; i < NU; i++)
        if (U[i].side == 0 && U[i].alive && !U[i].hidden) c[n++] = &U[i];
    if (!n)
        for (int i = 0; i < NU; i++)
            if (U[i].side == 0 && U[i].alive) c[n++] = &U[i];
    return n ? c[rnd_range(n)] : 0;
}
static BU* rand_enemy(void) {
    BU* c[8]; int n = 0;
    for (int i = 0; i < NU; i++)
        if (U[i].side == 1 && U[i].alive) c[n++] = &U[i];
    return n ? c[rnd_range(n)] : 0;
}

static void sync_to_G(void) {
    for (int i = 0; i < NU; i++) {
        if (U[i].side != 0) continue;
        PMember* p = &G.pm[U[i].pi];
        p->hp = U[i].hp < 0 ? 0 : U[i].hp;
        p->mp = U[i].mp;
    }
}

/* ------------------------------------------------------------------ combat */

static void unit_die(BU* t);

static void apply_damage(BU* t, int dmg, int pal) {
    t->sleep = 0;
    t->hp = (s16)(t->hp - dmg);
    popup_start(t->sx + (t->ed ? t->ed->wpx / 2 : 8), t->sy, dmg, pal);
    if (t->side == 0 && t->alive) {
        t->pose = PO_HURT;
    } else if (t->alive) {
        t->ox = (s16)(t->side == 1 ? 4 : -4);
    }
    sfx_play(SFX_HIT);
    sfx_noise(6);
}

static void settle(BU* t) {
    t->ox = 0;
    if (t->side == 0 && t->alive) t->pose = PO_IDLE;
    if (t->hp <= 0 && t->alive) unit_die(t);
}

static int calc_phys(BU* a, BU* t, int mult100) {
    int atk = a->atk + a->atkmod;
    if (atk < 1) atk = 1;
    int dmg = atk * 2 - t->def + rnd_range(atk / 2 + 1);
    if (a->blessed) dmg += 2;
    if (a->inspired) { dmg = dmg * 3 / 2; a->inspired = 0; }
    if (t->gbolt) { dmg = dmg * 3 / 2; t->gbolt = 0; }
    if (t->marked && a->side == 0 && G.pm[a->pi].cls == CLS_RANGER) dmg = dmg * 3 / 2;
    dmg = dmg * mult100 / 100;
    if (a->side == 0 && a->hidden) dmg *= 3;
    else if (rnd_range(16) == 0) dmg *= 2;
    if (dmg < 1) dmg = 1;
    return dmg;
}

static int calc_mag(BU* a, BU* t, int pow) {
    int dmg = pow + a->mag * 2 + rnd_range(a->mag + 1) - t->def / 2;
    if (t->gbolt) { dmg = dmg * 3 / 2; t->gbolt = 0; }
    if (dmg < 1) dmg = 1;
    return dmg;
}

static void unit_die(BU* t) {
    if (t->side == 0) {
        t->alive = 0;
        t->pose = PO_KO;
        return;
    }
    /* enemy/ally collapse: blink out */
    int obj = OBJ_BATTLE + (int)(t - U);
    for (int k = 0; k < 6; k++) {
        t->alive = (u8)(k & 1);
        if (!t->alive) obj_hide(obj);
        pump(4);
    }
    t->alive = 0;
    obj_hide(obj);
    if (t->side == 1) {
        xpsum += t->ed->xp;
        draw_enemy_names(en_wide);   /* match the window currently drawn */
        if (t->ed->ai == AI_ZHALK) {
            msg_wait("Zhalk falls!");
            G.everburn = 1;
            G.flags |= GF_ZHALK_DEAD;
            for (int i = 0; i < NU; i++)
                if (U[i].side == 0 && G.pm[U[i].pi].cls == CLS_FIGHTER) {
                    U[i].atk = (u8)(U[i].atk + 4);
                    G.pm[U[i].pi].atk = (u8)(G.pm[U[i].pi].atk + 4);
                    msg_wait("Everburn Blade claimed!");
                }
        }
    }
}

/* melee strike with step-forward animation */
static void act_melee(BU* a, BU* t, int mult100, const char* label) {
    if (label) msg_show(label);
    int dir = (a->side == 0 || (a->side == 2 && a->sx > t->sx)) ? -1 : 1;
    if (a->side == 1) dir = 1;
    for (int i = 0; i < 4; i++) { a->ox = (s16)(a->ox + dir * 2); pump(1); }
    if (a->side == 0) { a->pose = PO_SWING1; pump(8); a->pose = PO_SWING2; }
    else { a->actf = 1; pump(8); }
    int dmg = calc_phys(a, t, mult100);
    if (a->side == 0 && a->hidden) { a->hidden = 0; msg_show("Sneak attack!"); }
    apply_damage(t, dmg, 10);
    pump(20);
    settle(t);
    for (int i = 0; i < 4; i++) { a->ox = (s16)(a->ox - dir * 2); pump(1); }
    a->actf = 0;
    if (a->side == 0 && a->alive) a->pose = PO_IDLE;
    if (label) msg_hide();
    pump(6);
}

/* magic hit (no step): cast pose + flash */
static void act_magic(BU* a, BU* t, int pow, const char* label, int pal) {
    if (label) msg_show(label);
    if (a->side == 0) { a->pose = PO_CAST; }
    else a->actf = 1;
    REG_BLDCNT = 0x00FF;
    for (int i = 0; i < 3; i++) { REG_BLDY = 4; pump(2); REG_BLDY = 0; pump(2); }
    REG_BLDCNT = 0;
    int dmg = calc_mag(a, t, pow);
    apply_damage(t, dmg, pal);
    pump(20);
    settle(t);
    a->actf = 0;
    if (a->side == 0 && a->alive) a->pose = PO_IDLE;
    if (label) msg_hide();
    pump(6);
}

static void act_heal(BU* a, BU* t, int amount, const char* label) {
    if (label) msg_show(label);
    if (a && a->side == 0) a->pose = PO_CAST;
    sfx_play(SFX_HEAL);
    t->hp = (s16)(t->hp + amount);
    if (t->hp > t->hpmax) t->hp = t->hpmax;
    popup_start(t->sx + 8, t->sy, amount, 8);
    pump(24);
    if (a && a->side == 0) a->pose = PO_IDLE;
    if (label) msg_hide();
    pump(6);
}

/* ------------------------------------------------------------------ menus */

static int menu_list(const char* const* items, int n, int can_cancel, int x, int w) {
    win_draw(x, 15, w, 5);
    int sel = 0, top = 0;
    for (;;) {
        for (int r = 0; r < 3; r++) {
            txt_clear(x + 2, 16 + r, w - 3, 1);
            if (top + r < n) txt_put(x + 2, 16 + r, items[top + r], 0);
        }
        if (top > 0) txt_put(x + w - 2, 16, "-", 1);
        if (top + 3 < n) txt_put(x + w - 2, 18, "+", 1);
        for (;;) {
            obj_set(OBJ_CURSOR, x * 8 - 6, (16 + sel - top) * 8 - 4, 1, OBJT_HAND, 7, 0);
            pump(1);
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); if (sel < top) top--; break; }
            if (k & KEY_DOWN && sel < n - 1) { sel++; sfx_play(SFX_CURSOR); if (sel > top + 2) top++; break; }
            if (k & KEY_A) { sfx_play(SFX_CONFIRM); obj_hide(OBJ_CURSOR); win_clear(x, 15, w, 5); return sel; }
            if ((k & KEY_B) && can_cancel) { sfx_play(SFX_CANCEL); obj_hide(OBJ_CURSOR); win_clear(x, 15, w, 5); return -1; }
        }
    }
}

static BU* pick_target(int side, int want_dead) {
    BU* c[10]; int n = 0;
    for (int i = 0; i < NU; i++) {
        if ((int)U[i].side != side) continue;
        if (want_dead ? U[i].alive : !U[i].alive) continue;
        c[n++] = &U[i];
    }
    if (!n) return 0;
    int sel = 0;
    for (;;) {
        BU* t = c[sel];
        int w = t->ed ? t->ed->wpx : 16;
        obj_set(OBJ_CURSOR, t->sx + w / 2 - 20, t->sy + 2, 1, OBJT_HAND, 7, 0);
        pump(1);
        u16 k = key_hit();
        if (k & (KEY_UP | KEY_LEFT)) { sel = (sel + n - 1) % n; sfx_play(SFX_CURSOR); }
        if (k & (KEY_DOWN | KEY_RIGHT)) { sel = (sel + 1) % n; sfx_play(SFX_CURSOR); }
        if (k & KEY_A) { sfx_play(SFX_CONFIRM); obj_hide(OBJ_CURSOR); return t; }
        if (k & KEY_B) { sfx_play(SFX_CANCEL); obj_hide(OBJ_CURSOR); return 0; }
    }
}

/* ------------------------------------------------------------------ skills */

static void do_skill(BU* a, int sk, BU* t) {
    switch (sk) {
        case SK_MOCKERY:
            act_magic(a, t, 2, "Vicious Mockery!", 10);
            if (t->alive && t->atkmod > -6) t->atkmod = (s8)(t->atkmod - 2);
            break;
        case SK_HEALWORD: act_heal(a, t, 8 + a->mag, "Healing Word!"); break;
        case SK_INSPIRE:
            msg_show("Bardic Inspiration!");
            sfx_play(SFX_HEAL);
            t->inspired = 1;
            pump(24); msg_hide();
            break;
        case SK_HIDE:
            msg_show("Hidden in shadow...");
            a->hidden = 1;
            pump(24); msg_hide();
            break;
        case SK_STEAL: {
            const char* r;
            if (t->ed && t->ed->ai == AI_ZHALK) {
                if (!coin_stolen) { coin_stolen = 1; r = "Stole a Devil's Coin!"; }
                else r = "Nothing left to steal!";
            } else if (rnd_range(2)) {
                G.potions++;
                r = "Stole a Potion!";
            } else r = "Couldn't grab anything!";
            act_melee(a, t, 20, 0);
            msg_wait(r);
            break;
        }
        case SK_MARK:
            msg_show("Hunter's Mark!");
            t->marked = 1;
            pump(24); msg_hide();
            break;
        case SK_ENSNARE:
            act_magic(a, t, 3, "Ensnaring Strike!", 10);
            if (t->alive && rnd_range(10) < 6 && t->hpmax < 60) {
                t->sleep = 1;
                msg_wait("Ensnared!");
            }
            break;
        case SK_FIREBOLT: act_magic(a, t, 4, "Fire Bolt!", 10); break;
        case SK_MMISSILE:
            msg_show("Magic Missile!");
            for (int b = 0; b < 3 && t->alive; b++) {
                int dmg = 2 + a->mag / 2 + rnd_range(3);
                apply_damage(t, dmg, 10);
                pump(14);
                settle(t);
            }
            msg_hide();
            pump(6);
            break;
        case SK_BURNHANDS: {
            msg_show("Burning Hands!");
            a->pose = PO_CAST;
            REG_BLDCNT = 0x00FF;
            for (int i = 0; i < 4; i++) { REG_BLDY = 6; pump(2); REG_BLDY = 0; pump(2); }
            REG_BLDCNT = 0;
            for (int i = 0; i < NU; i++) {
                if (U[i].side != 1 || !U[i].alive) continue;
                apply_damage(&U[i], calc_mag(a, &U[i], 6), 10);
            }
            pump(24);
            for (int i = 0; i < NU; i++)
                if (U[i].side == 1) settle(&U[i]);
            a->pose = PO_IDLE;
            msg_hide();
            pump(6);
            break;
        }
        case SK_SLEEP:
            if (t->hpmax >= 60) {
                act_magic(a, t, 0, "Sleep!", 10);
                msg_wait("It resists!");
            } else {
                msg_show("Sleep!");
                a->pose = PO_CAST;
                t->sleep = 2;
                pump(24);
                a->pose = PO_IDLE;
                msg_wait("Fast asleep!");
            }
            break;
        case SK_CURE: act_heal(a, t, 10 + a->mag, "Cure Wounds!"); break;
        case SK_GBOLT:
            act_magic(a, t, 5, "Guiding Bolt!", 10);
            if (t->alive) t->gbolt = 1;
            break;
        case SK_BLESS:
            msg_show("Bless!");
            a->pose = PO_CAST;
            sfx_play(SFX_HEAL);
            for (int i = 0; i < NU; i++)
                if (U[i].side == 0 && U[i].alive) U[i].blessed = 1;
            pump(24);
            a->pose = PO_IDLE;
            msg_hide();
            break;
        case SK_SURGE:
            if (a->surged) { msg_wait("Already spent!"); return; }
            a->surged = 1;
            msg_wait("Action Surge!");
            {
                BU* t1 = rand_enemy();
                if (t1) act_melee(a, t1, 100, 0);
                t1 = rand_enemy();
                if (t1) act_melee(a, t1, 100, 0);
            }
            break;
    }
}

/* demo auto-play: returns 1 if the transponder was connected */
static int demo_turn(BU* u) {
    menu_unit = (int)(u - U);
    draw_status();
    pump(10);
    /* at the helm, TAV connects the nerves unless we're in kill-all mode */
    if (helm && u->pi == 0 && G_DEMO_BATTLE != 2) {
        BU* z = find_zhalk();
        if (!z || rounds <= 13) {         /* connect after a round or two of drama */
            msg_wait("You seize the nerves...");
            menu_unit = -1;
            return 1;
        }
    }
    BU* t = 0;
    if (G_DEMO_BATTLE == 2) t = find_zhalk();
    if (!t) t = rand_enemy();
    if (t) {
        u8 c = G.pm[u->pi].cls;
        if (c == CLS_WIZARD) do_skill(u, SK_FIREBOLT, t);      /* cantrip, no MP */
        else if (c == CLS_CLERIC && u->mp >= 4 && rnd_range(2)) {
            u->mp = (s16)(u->mp - 4); do_skill(u, SK_GBOLT, t);
        } else act_melee(u, t, 100, 0);
    }
    menu_unit = -1;
    return 0;
}

/* returns 1 if the transponder was connected */
static int player_turn(BU* u) {
    if (G_DEMO && !G_MANUAL_BAT) return demo_turn(u);
    menu_unit = (int)(u - U);
    draw_enemy_names(0);
    win_draw(0, 15, 8, 5);
    draw_enemy_names(0);

    const Cmd* kit = kits[G.pm[u->pi].cls];
    int nk = kitn[G.pm[u->pi].cls];
    const Cmd* list[8];
    const char* names[8];
    int n = 0;
    for (int i = 0; i < nk; i++) { list[n] = &kit[i]; names[n] = kit[i].name; n++; }
    if (helm && u->pi == 0) { list[n] = &cmd_nerve; names[n] = cmd_nerve.name; n++; }

    int done = 0, connected = 0;
    while (!done) {
        int sel = menu_list(names, n, 0, 8, 8);
        const Cmd* c = list[sel];
        switch (c->kind) {
            case K_FIGHT: {
                BU* t = pick_target(1, 0);
                if (t) { act_melee(u, t, 100, 0); done = 1; }
                break;
            }
            case K_SKILL: {
                if (c->mp > u->mp) { msg_wait("Not enough MP!"); break; }
                BU* t = u;
                if (c->target == 0) { t = pick_target(1, 0); if (!t) break; }
                else if (c->target == 1) { t = pick_target(0, 0); if (!t) break; }
                u->mp = (s16)(u->mp - c->mp);
                do_skill(u, c->skill, t);
                done = 1;
                break;
            }
            case K_MAGIC: {
                const Cmd* sub = (G.pm[u->pi].cls == CLS_WIZARD) ? magic_wiz : magic_cleric;
                int ns = 3;
                const char* snames[4];
                char buf[4][12];
                for (int i = 0; i < ns; i++) {
                    char* d = buf[i];
                    const char* s = sub[i].name;
                    while (*s) *d++ = *s++;
                    *d++ = ' ';
                    *d++ = (char)('0' + sub[i].mp);
                    *d = 0;
                    snames[i] = buf[i];
                }
                int ss = menu_list(snames, ns, 1, 8, 8);
                if (ss < 0) break;
                const Cmd* sc = &sub[ss];
                if (sc->mp > u->mp) { msg_wait("Not enough MP!"); break; }
                BU* t = u;
                if (sc->target == 0) { t = pick_target(1, 0); if (!t) break; }
                else if (sc->target == 1) { t = pick_target(0, 0); if (!t) break; }
                u->mp = (s16)(u->mp - sc->mp);
                do_skill(u, sc->skill, t);
                done = 1;
                break;
            }
            case K_ITEM: {
                char pbuf[12], rbuf[12];
                const char* inames[2];
                pbuf[0] = 0; rbuf[0] = 0;
                char* d = pbuf; const char* s = "Potion x";
                while (*s) *d++ = *s++;
                *d++ = (char)('0' + (G.potions > 9 ? 9 : G.potions)); *d = 0;
                d = rbuf; s = "Reviv. x";
                while (*s) *d++ = *s++;
                *d++ = (char)('0' + G.revivify); *d = 0;
                inames[0] = pbuf; inames[1] = rbuf;
                int is = menu_list(inames, 2, 1, 8, 8);
                if (is < 0) break;
                if (is == 0) {
                    if (!G.potions) { msg_wait("No potions left!"); break; }
                    BU* t = pick_target(0, 0);
                    if (!t) break;
                    G.potions--;
                    act_heal(u, t, 12, "Potion!");
                    done = 1;
                } else {
                    if (!G.revivify) { msg_wait("No scrolls left!"); break; }
                    BU* t = pick_target(0, 1);
                    if (!t) { msg_wait("No one has fallen."); break; }
                    t->alive = 1;
                    t->hp = (s16)(t->hpmax / 2);
                    t->pose = PO_IDLE;
                    G.revivify--;
                    act_heal(u, t, 0, "Revivify!");
                    done = 1;
                }
                break;
            }
            case K_NERVE:
                msg_wait("You seize the nerves...");
                connected = 1;
                done = 1;
                break;
        }
    }
    menu_unit = -1;
    win_draw(0, 15, 16, 5);
    draw_enemy_names(1);
    draw_status();
    return connected;
}

/* ------------------------------------------------------------------ ai */

static void warp_cambion(void) {
    extern const EnemyDef e_cambion;
    if (NU >= MAXU) return;
    BU* u = add_enemy(&e_cambion, 60, 56 + rnd_range(24), 1);
    (void)u;
    REG_BLDCNT = 0x00FF;
    REG_BLDY = 8; pump(3); REG_BLDY = 0; pump(3);
    REG_BLDCNT = 0;
    msg_wait("A cambion warps in!");
    draw_enemy_names(1);
}

static void ai_turn(BU* u) {
    if (u->sleep) { u->sleep--; return; }
    if (helm && u->side == 1 && u == first_enemy()) {
        rounds--;
        rounds_show();
        if (rounds == 9 || rounds == 5) warp_cambion();
        if (rounds <= 0) { timeout_flag = 1; return; }
    }
    switch (u->ed->ai) {
        case AI_IMP: {
            BU* t = rand_party();
            if (!t) return;
            if (rnd_range(10) < 6) act_magic(u, t, 3, "Fiery Bolt!", 10);
            else act_melee(u, t, 100, 0);
            break;
        }
        case AI_BOAR: {
            BU* t = rand_party();
            if (t) act_melee(u, t, 110, "Gore!");
            break;
        }
        case AI_THRALL: {
            BU* t = rand_party();
            if (t) act_melee(u, t, 90, 0);
            break;
        }
        case AI_CAMBION: {
            BU* t = rand_party();
            if (!t) return;
            if (rnd_range(2)) act_melee(u, t, 120, "Trident!");
            else act_magic(u, t, 5, "Fire Ray!", 10);
            break;
        }
        case AI_ZHALK: {
            BU* f = find_flayer();
            if (f) {
                msg_show("Zhalk hews the flayer!");
                u->actf = 1;
                for (int i = 0; i < 4; i++) { u->ox = (s16)(u->ox + 2); pump(1); }
                apply_damage(f, 6 + rnd_range(5), 10);
                pump(16);
                settle(f);
                if (!f->alive) msg_wait("The flayer collapses!");
                for (int i = 0; i < 4; i++) { u->ox = (s16)(u->ox - 2); pump(1); }
                u->actf = 0;
                msg_hide();
            } else {
                BU* t = rand_party();
                if (t) act_melee(u, t, 130, "Everburn Blade!");
            }
            break;
        }
        case AI_FLAYER_ALLY: {
            BU* z = find_zhalk();
            if (z) {
                msg_show("Flayer lashes Zhalk!");
                z->ox = 4;
                apply_damage(z, 7 + rnd_range(5), 10);
                pump(16);
                settle(z);
                msg_hide();
            } else {
                BU* t = rand_enemy();
                if (t) act_magic(u, t, 6, "Mind Blast!", 10);
            }
            break;
        }
        case AI_US_ALLY: {
            BU* t = rand_enemy();
            if (t) act_melee(u, t, 80, 0);
            break;
        }
    }
}

/* ------------------------------------------------------------------ main */

static void battle_transition_in(void) {
    /* FF4-style mosaic dissolve on the field view */
    REG_BG2CNT |= BGCNT_MOSAIC;
    REG_BG3CNT |= BGCNT_MOSAIC;
    for (int i = 0; i <= 12; i++) {
        REG_MOSAIC = (u16)(i | (i << 4));
        vsync(); vsync();
    }
    fade_out(8);
    REG_MOSAIC = 0;
    REG_BG2CNT &= (u16)~BGCNT_MOSAIC;
    REG_BG3CNT &= (u16)~BGCNT_MOSAIC;
}

int battle_run(const Formation* f) {
    Game snap = G;
    int result = -1;
    G_FIELD_IDLE = 0;

retry:
    mgba_logf("battle start n=%d flags=%x", f->n, f->flags);
    NU = 0;
    menu_unit = -1;
    helm = (f->flags & BF_HELM) ? 1 : 0;
    rounds = 15;
    timeout_flag = 0;
    xpsum = 0;
    coin_stolen = 0;
    for (int i = 0; i < 4; i++) pu[i].t = 0;
    init_pgfx();

    battle_transition_in();

    /* clear text/window layers, build scene */
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    for (int i = 0; i < 128; i++) obj_hide(i);
    backdrop();
    memcpy16((vu16*)((u32)OBJ_TILES + OBJ_TILE_COUNT * 32),
             &ui_tiles[('0' - 32) * 16], 10 * 16);

    for (int i = 0; i < G.nparty; i++) add_party_unit(i, i);
    if ((f->flags & BF_ALLY_US) && (G.flags & GF_US_FREED) && !(G.flags & GF_US_MUTILATED)) {
        extern const EnemyDef e_us;
        add_enemy(&e_us, 148, 96, 2);
    }
    if (f->flags & BF_ALLY_FLAYER) {
        extern const EnemyDef e_flayer;
        add_enemy(&e_flayer, 62, 40, 2);
    }
    for (int i = 0; i < f->n; i++) add_enemy(f->e[i], f->x[i], f->y[i], 1);

    music(helm ? SONG_BOSS : SONG_BATTLE);
    ui_base();
    rounds_show();
    draw_units();
    fade_in(12);

    int fleeing = 0;
    for (;;) {
        pump(1);

        /* flee: hold L+R */
        if (!(f->flags & BF_NO_FLEE) &&
            (key_state() & KEY_L) && (key_state() & KEY_R)) {
            if (++fleeing > 45) {
                sync_to_G();
                msg_wait("Fled!");
                result = BR_FLED;
                break;
            }
        } else fleeing = 0;

        /* tick ATB */
        BU* ready = 0;
        for (int i = 0; i < NU; i++) {
            BU* u = &U[i];
            if (!u->alive) continue;
            u->atb = (u16)(u->atb + u->spd / 2 + 2);
            if (u->atb >= 1000 && !ready) ready = u;
        }
        if (ready) {
            ready->atb = 0;
            if (ready->side == 0) {
                if (ready->sleep) { ready->sleep--; }
                else if (player_turn(ready)) {
                    sync_to_G();
                    result = BR_CONNECTED;
                    break;
                }
            } else {
                ai_turn(ready);
            }

            if (timeout_flag) {
                msg_wait("Too late! We're falling!");
                fade_out(20);
                G = snap;
                goto retry;
            }
            if (!enemies_alive()) {
                if (helm) {
                    sync_to_G();
                    result = BR_FLAYER_DONE;
                    break;
                }
                /* victory */
                sync_to_G();
                music(SONG_VICTORY);
                for (int i = 0; i < NU; i++)
                    if (U[i].side == 0 && U[i].alive) U[i].pose = PO_VICTORY;
                pump(30);
                {
                    char m[24];
                    char* d = m;
                    const char* s = "Received ";
                    while (*s) *d++ = *s++;
                    int v = xpsum, st = 1000;
                    int lead = 0;
                    for (; st; st /= 10) {
                        int dg = v / st % 10;
                        if (dg || lead || st == 1) { *d++ = (char)('0' + dg); lead = 1; }
                    }
                    s = " XP!";
                    while (*s) *d++ = *s++;
                    *d = 0;
                    msg_wait(m);
                }
                {
                    char names[32];
                    if (party_give_xp((u16)xpsum, names)) {
                        char m[40];
                        char* d = m;
                        const char* s = names;
                        while (*s) *d++ = *s++;
                        s = " grows stronger!";
                        while (*s) *d++ = *s++;
                        *d = 0;
                        msg_wait(m);
                    }
                }
                if (f == &form_deck) {
                    G.potions++;
                    msg_wait("Found a Potion!");
                }
                result = BR_WIN;
                break;
            }
            if (!party_alive()) {
                msg_wait("The party has fallen.");
                fade_out(20);
                G = snap;
                goto retry;
            }
        }
    }

    fade_out(12);
    for (int i = 0; i < 128; i++) obj_hide(i);
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    mgba_logf("battle result=%d", result);
    return result;
}
