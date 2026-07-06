/* Field pause menu (START): party overview, 5e character sheets, items,
 * DQ-style tactics. The whole party's state on one screen. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "party5.h"
#include "rules.h"

#define ME(t, h, v, p) ((u16)((t) | ((h) << 10) | ((v) << 11) | ((p) << 12)))
#define TXT ((vu16*)SCREENBLOCK(30))
#define POR_VRAM_TILE UI_TILE_COUNT

static const char* const cls_names[CLS_COUNT] = {
    "Bard", "Rogue", "Ranger", "Wizard", "Fighter", "Cleric",
    "Barbarian", "Druid", "Monk", "Paladin", "Sorcerer", "Warlock"
};
static const char* const tac_names[TAC_COUNT] = {
    "Orders", "Wisely", "All Out", "Healer", "No Slots"
};

/* spells shown on the sheet, per class */
static const u8 cls_spells[CLS_COUNT][3] = {
    [CLS_BARD]    = { R5S_VICIOUS_MOCKERY, R5S_HEALING_WORD, 0xFF },
    [CLS_ROGUE]   = { 0xFF, 0xFF, 0xFF },
    [CLS_RANGER]  = { R5S_HUNTERS_MARK, 0xFF, 0xFF },
    [CLS_WIZARD]  = { R5S_FIRE_BOLT, R5S_MAGIC_MISSILE, R5S_SLEEP },
    [CLS_FIGHTER] = { 0xFF, 0xFF, 0xFF },
    [CLS_CLERIC]  = { R5S_SACRED_FLAME, R5S_GUIDING_BOLT, R5S_CURE_WOUNDS },
    [CLS_BARBARIAN]= { 0xFF, 0xFF, 0xFF },
    [CLS_DRUID]   = { R5S_PRODUCE_FLAME, R5S_POISON_SPRAY, R5S_CURE_WOUNDS },
    [CLS_MONK]    = { 0xFF, 0xFF, 0xFF },
    [CLS_PALADIN] = { R5S_CURE_WOUNDS, R5S_BLESS, 0xFF },
    [CLS_SORCERER]= { R5S_FIRE_BOLT, R5S_RAY_OF_FROST, R5S_MAGIC_MISSILE },
    [CLS_WARLOCK] = { R5S_ELDRITCH_BLAST, R5S_BURNING_HANDS, 0xFF },
};

static char* mp_num(char* d, int v) {
    if (v < 0) { *d++ = '-'; v = -v; }
    char t[6]; int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *d++ = t[--n];
    return d;
}
static char* mp_str(char* d, const char* s) { while (*s) *d++ = *s++; return d; }

void sheet_audit(void) {   /* testability: what member_sheet shows per class */
    for (int cls = 0; cls < CLS_COUNT; cls++) {
        char line[120]; char* d = line;
        d = mp_str(d, "sheet cls="); d = mp_num(d, cls); *d++ = ':';
        for (int s = 0; s < 3; s++) {
            u8 sp = cls_spells[cls][s];
            if (sp == 0xFF) continue;
            *d++ = ' ';
            d = mp_str(d, r5_spells[sp].name);
        }
        *d = 0;
        mgba_log(line);
    }
}

void ui_portrait(int por, int cx, int cy) {
#if PORTRAIT_COUNT > 0
    if (por < 0) return;
    extern const u16 portrait_tiles[];
    memcpy16((vu16*)((u32)CHARBLOCK(0) + POR_VRAM_TILE * 32),
             &portrait_tiles[por * 36 * 16], 36 * 16);
    for (int r = 0; r < 6; r++)
        for (int c = 0; c < 6; c++)
            TXT[(cy + r) * 32 + cx + c] = ME(POR_VRAM_TILE + r * 6 + c, 0, 0, 5);
#else
    (void)por; (void)cx; (void)cy;
#endif
}

/* generic vertical hand-cursor pick over n rows at (x, y0, pitch); B = -1 */
static int pick_row(int x, int y0, int pitch, int n) {
    int sel = 0;
    for (;;) {
        obj_set(OBJ_CURSOR, x * 8 - 6, (y0 + sel * pitch) * 8 - 4, 1, OBJT_HAND, 7, 0);
        frame();
        field_draw();
        u16 k = key_hit();
        if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); }
        if (k & KEY_DOWN && sel < n - 1) { sel++; sfx_play(SFX_CURSOR); }
        if (k & KEY_A) { sfx_play(SFX_CONFIRM); obj_hide(OBJ_CURSOR); return sel; }
        if (k & (KEY_B | KEY_START)) { sfx_play(SFX_CANCEL); obj_hide(OBJ_CURSOR); return -1; }
    }
}

static void clear_all(void) {
    win_clear(0, 0, 30, 20);
}

/* ---------------------------------------------------------------- overview */

static void draw_overview(void) {
    clear_all();
    win_draw(0, 0, 30, 3);
    txt_put(2, 1, "PARTY", 1);
    {
        char b[22]; char* d = b;
        d = mp_str(d, "Potion x"); d = mp_num(d, G.potions);
        d = mp_str(d, " Scr x"); d = mp_num(d, G.revivify);
        *d = 0;
        txt_put_n(11, 1, b, 0, 18);
    }
    win_draw(0, 3, 19, 17);
    for (int i = 0; i < G.nparty; i++) {
        R5Creature* c = &party5[i];
        int y = 4 + i * 5;
        char b[24]; char* d;
        txt_put_n(2, y, c->name, 1, 7);
        d = b; d = mp_str(d, "Lv"); d = mp_num(d, c->level);
        *d++ = ' '; d = mp_str(d, cls_names[c->cls]); *d = 0;
        txt_put_n(10, y, b, 0, 8);
        d = b; d = mp_str(d, "HP "); d = mp_num(d, c->hp);
        *d++ = '/'; d = mp_num(d, c->hpmax);
        d = mp_str(d, "  AC "); d = mp_num(d, c->ac); *d = 0;
        txt_put_n(2, y + 1, b, (c->conds & C_UNCONSCIOUS) ? 2 : 0, 16);
        d = b;
        for (int s = 0; s < 3; s++) *d++ = (char)(s < c->slots[0] ? '*' : '-');
        *d++ = ' '; *d++ = ' ';
        d = mp_str(d, tac_names[G.tactics[i]]); *d = 0;
        txt_put_n(2, y + 2, b, 2, 14);
        txt_put_n(2, y + 3, r5_weapons[G.weapon[i]].name, 2, 15);
    }
    win_draw(19, 3, 11, 17);
    txt_put(22, 5, "Status", 0);
    txt_put(22, 7, "Equip", 0);
    txt_put(22, 9, "Prepare", 0);
    txt_put(22, 11, "Items", 0);
    txt_put(22, 13, "Tactics", 0);
    txt_put(22, 15, "Close", 0);
}

/* "Rapier 1d8 fin" style line */
static void weapon_line(char* d, int w) {
    const R5Weapon* rw = &r5_weapons[w];
    d = mp_str(d, rw->name);
    *d++ = ' ';
    d = mp_num(d, rw->dmg.n); *d++ = 'd'; d = mp_num(d, rw->dmg.sides);
    if (rw->props & WP_FINESSE) d = mp_str(d, " fin");
    if (rw->props & WP_RANGED) d = mp_str(d, " rng");
    if (rw->props & WP_TWO_HANDED) d = mp_str(d, " 2h");
    if (rw->rider_dmg.n) {
        d = mp_str(d, rw->rider_type == DT_FIRE ? " +fire" : " +venom");
    }
    *d = 0;
}

static int prepared_caster(int cls) {
    return cls == CLS_CLERIC || cls == CLS_WIZARD ||
           cls == CLS_DRUID  || cls == CLS_PALADIN;
}

/* prepare-count = spellcasting mod + level (min 1), 5e-standard */
static int prepare_count(const R5Creature* c) {
    int mod = r5_mod(c->ab[party5_cast_ab(c->cls)]);
    int n = mod + c->level;
    return n < 1 ? 1 : n;
}

static void prepare_screen(void) {
    mgba_log("prepare-screen");
    for (;;) {
        clear_all();
        win_draw(2, 2, 26, 4);
        txt_put(9, 3, "-- PREPARE --", 1);
        int cast = 0, rows[4], nc = 0;
        for (int i = 0; i < G.nparty; i++)
            if (prepared_caster(G.pm[i].cls)) { rows[nc] = i; txt_put_n(4, 5, "", 0, 1); nc++; }
        for (int k = 0; k < nc; k++)
            txt_put_n(5, 4 + k, party5[rows[k]].name, 0, 10);
        if (!nc) { txt_put(5, 4, "No prepared casters.", 2);
                   for (;;){ frame(); field_draw(); if (key_hit()&(KEY_A|KEY_B)) return; } }
        int pk = pick_row(4, 4, 1, nc);
        if (pk < 0) return;
        int mi = rows[pk];
        R5Creature* c = &party5[mi];
        int cap = prepare_count(c);
        const u8* list = r5_class_spells[c->cls];
        int shown[16], sn = 0;
        for (int s = 0; s < 16 && list[s] != 255; s++)
            if (r5_spells[list[s]].level >= 1) shown[sn++] = s;   /* cantrips always known */
        int sel = 0;
        for (;;) {
            cast = 0;
            for (int r = 0; r < sn; r++)
                if (G.pm[mi].prepared & (1u << shown[r])) cast++;
            clear_all();
            win_draw(1, 1, 28, 18);
            char b[48]; char* d = b;   /* wide: some SRD spell names are 29 chars */
            d = mp_str(d, party5[mi].name); d = mp_str(d, ": "); d = mp_num(d, cast);
            d = mp_str(d, "/"); d = mp_num(d, cap); d = mp_str(d, " prepared"); *d = 0;
            txt_put_n(3, 2, b, 1, 26);
            for (int r = 0; r < sn && r < 14; r++) {
                const R5Spell* rs = &r5_spells[list[shown[r]]];
                int on = (G.pm[mi].prepared >> shown[r]) & 1;
                d = b; *d++ = on ? '*' : ' '; *d++ = ' ';
                for (const char* p = rs->name; *p && d < b + 40; p++) *d++ = *p;
                *d++ = ' '; *d++ = (char)('0' + rs->level); *d = 0;
                txt_put_n(4, 4 + r, b, on ? 1 : 0, 22);
            }
            obj_set(OBJ_CURSOR, 3 * 8 - 6, (4 + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
            frame(); field_draw();
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) sel--;
            if (k & KEY_DOWN && sel < sn - 1) sel++;
            if (k & KEY_A) {
                u32 bit = 1u << shown[sel];
                if (G.pm[mi].prepared & bit) { G.pm[mi].prepared &= ~bit; sfx_play(SFX_CANCEL); }
                else if (cast < cap) { G.pm[mi].prepared |= bit; sfx_play(SFX_CONFIRM); }
                else sfx_play(SFX_CANCEL);
            }
            if (k & KEY_B) { obj_hide(OBJ_CURSOR); break; }
        }
    }
}

static void equip_screen(void) {
    for (;;) {
        clear_all();
        win_draw(1, 2, 28, 3 + G.nparty * 2);
        txt_put(11, 3, "-- EQUIP --", 1);
        char b[32];
        for (int i = 0; i < G.nparty; i++) {
            txt_put_n(4, 5 + i * 2, party5[i].name, 0, 7);
            weapon_line(b, G.weapon[i]);
            txt_put_n(12, 5 + i * 2, b, 2, 16);
        }
        int mi = pick_row(3, 5, 2, G.nparty);
        if (mi < 0) return;
        if (!G.nwinv) { sfx_play(SFX_CANCEL); continue; }

        int wy = 6 + G.nparty * 2;
        win_draw(1, wy, 28, G.nwinv + 3);
        txt_put(4, wy + 1, "Carried:", 2);
        for (int j = 0; j < G.nwinv; j++) {
            weapon_line(b, G.winv[j]);
            txt_put_n(6, wy + 2 + j, b, 0, 22);
        }
        int wj = pick_row(5, wy + 2, 1, G.nwinv);
        win_clear(1, wy, 28, G.nwinv + 3);
        if (wj < 0) continue;
        u8 old = G.weapon[mi];
        G.weapon[mi] = G.winv[wj];
        G.winv[wj] = old;                 /* swap keeps the pool honest */
        sfx_play(SFX_CONFIRM);
    }
}

/* ---------------------------------------------------------------- sheet */

static void member_sheet(int i) {
    R5Creature* c = &party5[i];
    clear_all();
    win_draw(0, 0, 30, 20);
    ui_portrait(member_look(G.pm[i].face, c->cls).por, 1, 1);

    char b[28]; char* d;
    txt_put_n(9, 1, c->name, 1, 8);
    d = b; d = mp_str(d, cls_names[c->cls]);
    d = mp_str(d, "  Lv"); d = mp_num(d, c->level); *d = 0;
    txt_put_n(9, 2, b, 0, 12);
    d = b; d = mp_str(d, "XP "); d = mp_num(d, G.pm[i].xp);
    d = mp_str(d, c->level < 3 ? "/" : "");
    if (c->level < 3) d = mp_num(d, c->level == 1 ? 300 : 900);
    *d = 0;
    txt_put_n(9, 3, b, 2, 12);
    d = b; d = mp_str(d, "HP "); d = mp_num(d, c->hp);
    *d++ = '/'; d = mp_num(d, c->hpmax); *d = 0;
    txt_put_n(9, 4, b, (c->conds & C_UNCONSCIOUS) ? 2 : 0, 10);
    d = b; d = mp_str(d, "AC "); d = mp_num(d, c->ac);
    d = mp_str(d, "  Prof +"); d = mp_num(d, r5_prof(c)); *d = 0;
    txt_put_n(9, 5, b, 0, 14);
    txt_put_n(22, 1, tac_names[G.tactics[i]], 2, 8);

    static const char* const abn[6] = { "STR", "DEX", "CON", "INT", "WIS", "CHA" };
    for (int a = 0; a < 6; a++) {
        int x = 2 + (a / 3) * 14, y = 8 + (a % 3);
        d = b; d = mp_str(d, abn[a]);
        *d++ = (char)((c->save_prof >> a) & 1 ? '*' : ' ');
        d = mp_num(d, c->ab[a]);
        *d++ = ' ';
        int m = r5_mod(c->ab[a]);
        *d++ = (char)(m >= 0 ? '+' : '-');
        d = mp_num(d, m < 0 ? -m : m); *d = 0;
        txt_put_n(x, y, b, 0, 11);
    }
    txt_put(2, 11, "* save proficiency", 2);

    {
        char wl[32];
        weapon_line(wl, G.weapon[i]);
        d = b; d = mp_str(d, "Weapon: "); d = mp_str(d, wl); *d = 0;
        txt_put_n(2, 13, b, 0, 27);
    }

    int row = 15;
    for (int s = 0; s < 3 && row < 19; s++) {
        u8 sp = cls_spells[c->cls][s];
        if (sp == 0xFF) continue;
        const R5Spell* rs = &r5_spells[sp];
        d = b; d = mp_str(d, rs->name);
        d = mp_str(d, rs->level ? "  (slot)" : "  (at-will)"); *d = 0;
        txt_put_n(3, row++, b, 0, 26);
    }
    if (c->cls == CLS_FIGHTER) {
        txt_put_n(3, row++, (c->used & USED_SECOND_WIND)
                  ? "Second Wind (spent)" : "Second Wind", 0, 24);
        if (c->level >= 2)
            txt_put_n(3, row++, (c->used & USED_ACTION_SURGE)
                      ? "Action Surge (spent)" : "Action Surge", 0, 24);
    }
    if (c->cls == CLS_ROGUE) {
        d = b; d = mp_str(d, "Sneak Attack ");
        d = mp_num(d, r5_sneak_dice(c)); d = mp_str(d, "d6"); *d = 0;
        txt_put_n(3, row++, b, 0, 24);
        if (c->level >= 2) txt_put_n(3, row++, "Cunning Action", 0, 24);
    }
    if (r5_classes[c->cls].slots[c->level][0]) {
        d = b; d = mp_str(d, "Slots ");
        for (int s = 0; s < r5_classes[c->cls].slots[c->level][0]; s++)
            *d++ = (char)(s < c->slots[0] ? '*' : '-');
        *d = 0;
        txt_put_n(20, 3, b, 1, 9);
    }

    for (;;) {
        frame();
        field_draw();
        if (key_hit() & (KEY_B | KEY_A | KEY_START)) break;
    }
    sfx_play(SFX_CANCEL);
}

/* ---------------------------------------------------------------- items */

static R5RNG mrng;

static void items_screen(void) {
    for (;;) {
        clear_all();
        win_draw(4, 3, 22, 9);
        txt_put(7, 4, "-- ITEMS --", 1);
        char b[24]; char* d;
        d = b; d = mp_str(d, "Potion    x"); d = mp_num(d, G.potions); *d = 0;
        txt_put_n(8, 6, b, G.potions ? 0 : 2, 14);
        d = b; d = mp_str(d, "Revivify  x"); d = mp_num(d, G.revivify); *d = 0;
        txt_put_n(8, 8, b, G.revivify ? 0 : 2, 14);
        if (G.everburn) txt_put(8, 10, "Everburn Blade", 1);
        else if (G.flags & GF_RUNE) txt_put(8, 10, "Eldritch Rune", 1);

        int sel = pick_row(7, 6, 2, 2);
        if (sel < 0) return;
        int is_pot = (sel == 0);
        if (is_pot && !G.potions) { sfx_play(SFX_CANCEL); continue; }
        if (!is_pot && !G.revivify) { sfx_play(SFX_CANCEL); continue; }

        /* pick a member */
        win_draw(8, 12, 14, G.nparty + 2);
        for (int i = 0; i < G.nparty; i++)
            txt_put_n(11, 13 + i, party5[i].name, 0, 8);
        int m = pick_row(10, 13, 1, G.nparty);
        win_clear(8, 12, 14, G.nparty + 2);
        if (m < 0) continue;
        R5Creature* c = &party5[m];
        if (is_pot) {
            if (c->hp >= c->hpmax) { sfx_play(SFX_CANCEL); continue; }
            G.potions--;
            R5Dice h = r5_roll(&mrng, 2, 4, 2);
            r5_heal(c, h.total);
            sfx_play(SFX_HEAL);
        } else {
            if (c->hp > 0) { sfx_play(SFX_CANCEL); continue; }
            G.revivify--;
            r5_heal(c, 1);
            sfx_play(SFX_HEAL);
        }
    }
}

/* ---------------------------------------------------------------- tactics */

static void tactics_screen(void) {
    for (;;) {
        clear_all();
        win_draw(3, 3, 24, 3 + G.nparty * 2);
        txt_put(6, 4, "-- TACTICS --", 1);
        for (int i = 0; i < G.nparty; i++) {
            txt_put_n(6, 6 + i * 2, party5[i].name, 0, 8);
            txt_put_n(16, 6 + i * 2, tac_names[G.tactics[i]], 2, 9);
        }
        int m = pick_row(5, 6, 2, G.nparty);
        if (m < 0) return;
        win_draw(14, 8, 13, TAC_COUNT + 2);
        for (int t = 0; t < TAC_COUNT; t++)
            txt_put_n(17, 9 + t, tac_names[t], 0, 9);
        int t = pick_row(16, 9, 1, TAC_COUNT);
        if (t >= 0) G.tactics[m] = (u8)t;
    }
}

/* ---------------------------------------------------------------- main */

void field_menu(void) {
    G_FIELD_IDLE = 0;
    r5_seed(&mrng, rnd());
    sfx_play(SFX_CONFIRM);
    for (;;) {
        draw_overview();
        int sel = pick_row(21, 5, 2, 6);
        if (sel < 0 || sel == 5) break;
        if (sel == 0) {
            win_draw(19, 16, 11, G.nparty + 2);
            for (int i = 0; i < G.nparty; i++)
                txt_put_n(22, 17 + i, party5[i].name, 0, 7);
            int m = pick_row(21, 17, 1, G.nparty);
            if (m >= 0) member_sheet(m);
        } else if (sel == 1) equip_screen();
        else if (sel == 2) prepare_screen();
        else if (sel == 3) items_screen();
        else tactics_screen();
    }
    clear_all();
}
