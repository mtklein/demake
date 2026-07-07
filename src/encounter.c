/* Battle 2.0: 5e encounters fought in place on the field map (CT-style).
 *
 * Combatants are field sprites; the camera stays where you stood. Initiative
 * is rolled visibly, turns follow the 5e action + bonus-action economy, and
 * every die lands in the message bar. Consumes rules/ for ALL math.
 *
 * v1 simplifications (documented): attackers dash in and return home
 * (engagement is logical, not spatial); downed PCs keep their standing
 * sprite (status shows U); no explicit Move command yet -- Disengage/Dodge
 * carry the positioning weight, and leaving melee to strike someone else
 * provokes an opportunity attack.
 */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "party5.h"
#include "rules.h"
#include "encounter.h"
#include "audio.h"
#include "dice_ui.h"

#define ME(t, h, v, p) ((u16)((t) | ((h) << 10) | ((v) << 11) | ((p) << 12)))
#define TXT ((vu16*)SCREENBLOCK(30))

#define MAXEC 12
typedef struct {
    R5Creature* c;
    R5Creature store;            /* backing for non-party combatants */
    s16 hx, hy;                  /* home position (px) */
    s8 npc;                      /* field npc idx driving the sprite */
    u8 side;                     /* 0 party, 1 enemy, 2 ally */
    u8 mon, pi;                  /* monster id / party index */
    s16 init;
    s8 engaged;                  /* EC idx we're locked in melee with */
    u8 reacted, dodge, hidden;
    u8 gbolt, mock;              /* next-attack riders */
    s8 marked_by;
    u8 smite;                    /* paladin: next hit spends a slot */
    u8 shaped;                   /* druid wild shape: c points at store */
    u16 xp;
} EC;

static EC ec[MAXEC];
static int nec, round_no, rounds_left;
static int warp_npc[2], nwarp;
static R5RNG rng;
static int party_npc[3];         /* temp npcs for the party sprites */

/* ---------------------------------------------------------------- the dark
 * Darkvision doctrine (docs/character2.md): events.c flags DARK rooms here;
 * the equipped Everburn Blade lights the room outright -- the fighter's
 * trophy becomes the party's torch. Everything else (per-actor disadvantage)
 * keys off encounter_dark() at the attack-flag choke point. */
static int enc_dark_room;
void encounter_set_dark(int on) { enc_dark_room = on; }
int encounter_dark(void) {
    if (!enc_dark_room) return 0;
    for (int i = 0; i < G.nparty; i++)
        if (G.weapon[i] == R5W_EVERBURN) return 0;
    return 1;
}

/* ------------------------------------------------------------------ ui */

static void rnd_show(void) {
    win_draw(26, 0, 4, 3);
    int v = rounds_left > 0 ? rounds_left : round_no;
    char b[3] = { ' ', ' ', 0 };
    if (v >= 10) { b[0] = (char)('0' + v / 10); b[1] = (char)('0' + v % 10); }
    else b[1] = (char)('0' + (v < 0 ? 0 : v));
    txt_put(27, 1, b, (rounds_left > 0 && rounds_left <= 3) ? 1 : 0);
}

static void bar(const char* s) {
    win_clear(0, 0, 30, 3);
    win_draw(0, 0, 26, 3);
    char buf[25];
    int i = 0;
    while (s[i] && i < 24) { buf[i] = s[i]; i++; }
    buf[i] = 0;
    txt_put(1, 1, buf, 0);
    rnd_show();
}

static void pump(int n);
static void add_mon(int mon, int npc, int side, u16 xp);

static void bar_wait(const char* s) {
    bar(s);
    int t = 0;
    for (;;) {
        pump(1);
        if (key_hit() & KEY_A) break;
        if (++t >= (G_DEMO ? 34 : 90)) break;   /* auto-advance */
    }
}

static char* put_num(char* d, int v) {
    if (v < 0) { *d++ = '-'; v = -v; }
    char tmp[6]; int n = 0;
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *d++ = tmp[--n];
    return d;
}
static char* put_str(char* d, const char* s) {
    while (*s) *d++ = *s++;
    return d;
}

/* "14+5=19 v13 HIT!" / "3+5=8 v13 miss" / "20! CRIT" */
static void bar_attack(const char* verb, const R5Attack* a) {
    char m[48]; char* d = m;
    d = put_str(d, verb); *d++ = ' ';
    d = put_num(d, a->d20.total);
    if (a->bonus >= 0) *d++ = '+';
    d = put_num(d, a->bonus);
    *d++ = '='; d = put_num(d, a->total);
    d = put_str(d, " v"); d = put_num(d, a->target_ac);
    d = put_str(d, a->crit ? " CRIT!" : a->hit ? " HIT" : " miss");
    *d = 0;
    bar_wait(m);
}

static void bar_damage(const R5Attack* a) {
    char m[48]; char* d = m;
    d = put_str(d, "dmg ");
    d = put_num(d, a->dmg.n); *d++ = 'd'; d = put_num(d, a->dmg.sides);
    if (a->dmg.mod) { *d++ = '+'; d = put_num(d, a->dmg.mod); }
    *d++ = '='; d = put_num(d, a->damage);
    if (a->rider_dmg.n) {
        d = put_str(d, " +"); d = put_num(d, a->rider_damage);
        d = put_str(d, a->rider_saved ? " sv" : "!");
    }
    *d = 0;
    bar_wait(m);
}

static void status_draw(void) {
    win_draw(0, 15, 16, 5);
    for (int i = 0; i < G.nparty; i++) {
        R5Creature* c = &party5[i];
        int y = 16 + i;
        char nm[7]; int k = 0;
        while (c->name[k] && k < 5) { nm[k] = c->name[k]; k++; }
        nm[k] = 0;
        txt_put_n(1, y, nm, (c->conds & C_UNCONSCIOUS) ? 2 : 0, 5);
        char hp[10]; char* d = hp;
        d = put_num(d, c->hp); *d++ = '/'; d = put_num(d, c->hpmax); *d = 0;
        txt_put_n(7, y, hp, c->hp * 4 < c->hpmax ? 1 : 0, 5);
        char pips[4]; int p = 0;
        for (int s = 0; s < c->slots[0] && p < 3; s++) pips[p++] = '*';
        pips[p] = 0;
        txt_put_n(12, y, (c->conds & C_UNCONSCIOUS) ? "KO" : pips, 1, 3);
    }
}

/* damage-type tint for popup numbers and dice sprites */
static const u8 dmg_pal_tab[DT_COUNT] = {
    [DT_SLASHING] = 10, [DT_PIERCING] = 10, [DT_BLUDGEONING] = 10,
    [DT_FIRE] = 11, [DT_COLD] = 15, [DT_POISON] = 12, [DT_PSYCHIC] = 14,
    [DT_RADIANT] = 9, [DT_NECROTIC] = 12, [DT_LIGHTNING] = 15,
    [DT_THUNDER] = 15, [DT_ACID] = 12, [DT_FORCE] = 13,
};
#define DPAL(t) dmg_pal_tab[(t) < DT_COUNT ? (t) : 0]

/* ------------------------------------------------------- the dice tray
 * Every rolled die appears as its polyhedron with the value overdrawn,
 * tinted by damage type, in a row under the message bar. */
#define OBJ_DICE 56
#define OBJ_ZZ 50                /* sleep marker overlays, 3 slots */
#define SPELL_CODE 100          /* menu code base for spells: SPELL_CODE + R5S_id.
                                 * MUST stay above every class-action code (0-25). */
#define OBJ_TETH 24              /* engagement tether dots 24-26, cursor glyphs 27-28 */
static int conscious(const EC* e);
static void shape_revert(EC* e, const char* why);
static void mon_to_creature(int mon, R5Creature* c);
static EC* g_act;    /* whose turn it is (engagement tether) */
static EC* tsel_a;   /* attacker during attack target-select (cursor glyphs) */
static int tray_n;

/* one settled die at the next tray slot -- the shared primitive lays out the
 * face + digits (dice_ui.c); the tray just tracks the running slot/x */
static void tray_push(int sides, int value, int pal) {
    if (tray_n >= 8) return;
    dice_draw_one(OBJ_DICE + tray_n * 3, 6 + tray_n * 20, 26, sides, value, pal);
    tray_n++;
}

static void tray_dice(const R5Dice* d, int pal) {
    for (int i = 0; i < d->n; i++) tray_push(d->sides, d->rolls[i], pal);
}

static void pump1(void) { pump(1); }   /* one-frame step for the shared tumble */

/* the headline "does it land" die: tumble it into the next tray slot(s) --
 * BOTH dice on advantage/disadvantage, spinning together -- then settle. The
 * damage/bless/rider dice that follow appear as they always have (tray_push);
 * only the meaningful roll gets the drama, which bounds the added frames. */
static void tray_headline_d20(const R5Dice* d, int pal) {
    if (tray_n >= 8) return;
    dice_roll_headline(OBJ_DICE + tray_n * 3, 6 + tray_n * 20, 20, 26,
                       d->sides, d->rolls, d->n, pal, pump1);
    tray_n = (int)(tray_n + d->n);
}

static void tray_clear(void) {
    for (int i = 0; i < 8 * 3; i++) obj_hide(OBJ_DICE + i);
    tray_n = 0;
}

/* battle-end OBJ + state cleanup (tray_clear runs after EVERY attack --
 * resetting g_act there silently killed the engagement tether) */
static void enc_garnish_clear(void) {
    for (int i = 0; i < nec; i++)
        if (ec[i].shaped) shape_revert(&ec[i], "");
    tray_clear();
    for (int i = 0; i < 3; i++) obj_hide(OBJ_ZZ + i);
    for (int i = 0; i < 5; i++) obj_hide(OBJ_TETH + i);
    g_act = 0; tsel_a = 0;
}

/* everything an attack rolled, laid out: d20(s), bless, damage, rider */
static void tray_attack(const R5Attack* at) {
    tray_clear();
    tray_headline_d20(&at->d20, at->crit ? 9 : 10);
    if (at->bless.n) tray_dice(&at->bless, 9);
    if (at->hit) {
        tray_dice(&at->dmg, DPAL(at->dmg_type));
        if (at->rider_dmg.n) tray_dice(&at->rider_dmg, DPAL(at->rider_type));
    }
}

/* ---------------------------------------------------------------- popups */

#define OBJ_PU 40
static void popup(int scr_x, int scr_y, int val, int pal) {
    char b[6]; char* e = put_num(b, val); *e = 0;
    int n = (int)(e - b);
    for (int t = 0; t < 34; t++) {
        int rise = t > 24 ? 12 : t / 2;
        for (int d = 0; d < 4; d++) {
            if (d < n)
                obj_set(OBJ_PU + d, scr_x - n * 3 + d * 7, scr_y - rise, 0,
                        OBJ_TILE_COUNT + (b[d] - '0'), pal, 0);
            else obj_hide(OBJ_PU + d);
        }
        pump(1);
    }
    for (int d = 0; d < 4; d++) obj_hide(OBJ_PU + d);
}

/* ------------------------------------------------------------------ sprites */

static void ec_place(EC* e, int x, int y) {
    if (e->npc >= 0) { npcs[e->npc].x = (s16)x; npcs[e->npc].y = (s16)y; }
}
static int ec_x(const EC* e) { return e->npc >= 0 ? npcs[e->npc].x : e->hx; }
static int ec_y(const EC* e) { return e->npc >= 0 ? npcs[e->npc].y : e->hy; }
static int ec_sx(const EC* e) { return ec_x(e) - field_cam_x(); }
static int ec_sy(const EC* e) { return ec_y(e) - field_cam_y(); }

static void ec_face_toward(EC* e, const EC* t) {
    if (e->npc < 0) return;
    int dx = ec_x(t) - ec_x(e), dy = ec_y(t) - ec_y(e);
    int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
    npcs[e->npc].face = (u8)(adx > ady ? (dx > 0 ? 3 : 2) : (dy > 0 ? 0 : 1));
}

/* would attacking t sneak for a? mirrors atk_flags' ally-engaged test */
static int sneak_ok(EC* a, EC* t) {
    if (a->hidden) return 1;
    for (int i = 0; i < nec; i++)
        if (&ec[i] != a && ec[i].side == a->side && conscious(&ec[i]) &&
            ec[i].engaged == (s8)(t - ec)) return 1;
    return 0;
}

/* downed PCs lie where they fell; sleepers snore visibly */
static void garnish_draw(void) {
    int zs = 0;
    for (int i = 0; i < nec; i++) {
        EC* e = &ec[i];
        if (e->npc < 0 || (npcs[e->npc].flags & NPC_GONE)) continue;
        int out = (e->c->conds & C_UNCONSCIOUS) != 0;
        if (e->side == 0 && !e->shaped) {
            MemberLook L = member_look(G.pm[e->pi].face, G.pm[e->pi].cls);
            npcs[e->npc].objt = out ? L.ko : L.objt;
            if (out) npcs[e->npc].face = 0;
        } else if (out && e->c->hp > 0 && zs < 3) {
            obj_set(OBJ_ZZ + zs, ec_sx(e) + 6, ec_sy(e) - 12, 1,
                    OBJT_SLEEPZ, 15, 0);
            zs++;
        }
    }
    while (zs < 3) obj_hide(OBJ_ZZ + zs++);
    /* marching-ant tether on the acting combatant's melee lock */
    static u32 gt;
    gt++;
    if (g_act && g_act->engaged >= 0 && conscious(&ec[g_act->engaged])) {
        EC* o = &ec[g_act->engaged];
        static s16 logged = -1;
        s16 pair = (s16)((g_act - ec) * 32 + g_act->engaged);
        if (pair != logged) {
            logged = pair;
            mgba_logf("tether %s<->%s", g_act->c->name, o->c->name);
        }
        int x0 = ec_sx(g_act) + 8, y0 = ec_sy(g_act) + 8;
        int dx = ec_sx(o) + 8 - x0, dy = ec_sy(o) + 8 - y0;
        for (int i = 0; i < 3; i++) {
            int f = i * 8 + (int)((gt >> 2) & 7);           /* 0..23: dots flow */
            obj_set(OBJ_TETH + i, x0 + dx * f / 24 - 4,
                    y0 + dy * f / 24 - 4, 0, OBJT_GARN, 10, 0);
        }
    } else
        for (int i = 0; i < 3; i++) obj_hide(OBJ_TETH + i);
}

static void pump(int n) {
    while (n--) {
        frame();
        garnish_draw();
        field_draw();
        status_draw();
    }
}

/* dash toward the target, strike beat, dash home */
static void dash_to(EC* a, const EC* t, int gap) {
    int tx = ec_x(t) + (ec_x(a) < ec_x(t) ? -gap : gap), ty = ec_y(t);
    int sx = a->hx, sy = a->hy;
    ec_face_toward(a, t);
    for (int i = 1; i <= 10; i++) {
        ec_place(a, sx + (tx - sx) * i / 10, sy + (ty - sy) * i / 10);
        pump(1);
    }
}
static void dash_home(EC* a) {
    int sx = ec_x(a), sy = ec_y(a);
    for (int i = 1; i <= 10; i++) {
        ec_place(a, sx + (a->hx - sx) * i / 10, sy + (a->hy - sy) * i / 10);
        pump(1);
    }
}

/* pan the camera so every combatant sits in the band clear of the text
 * bars (y 24..120 on screen); smooth CT-style glide */
static void frame_camera(void) {
    int x0 = 0x7FFF, y0 = 0x7FFF, x1 = -0x7FFF, y1 = -0x7FFF;
    for (int i = 0; i < nec; i++) {
        if (ec[i].side != 0 && ec[i].c->hp <= 0) continue;
        int x = ec_x(&ec[i]), y = ec_y(&ec[i]);
        if (x < x0) x0 = x;
        if (y < y0) y0 = y;
        if (x + 16 > x1) x1 = x + 16;
        if (y + 16 > y1) y1 = y + 16;
    }
    if (x0 > x1) return;
    int tx = (x0 + x1) / 2 - 120;
    int ty = (y0 + y1) / 2 - 72;          /* center of the 24..120 band */
    int sx = field_cam_x(), sy = field_cam_y();
    for (int i = 1; i <= 20; i++) {
        field_cam_override(1, sx + (tx - sx) * i / 20, sy + (ty - sy) * i / 20);
        pump(1);
    }
}

static void hit_react(EC* t) {
    sfx_play(SFX_HIT);
    sfx_noise(6);
    int x = ec_x(t), y = ec_y(t);
    for (int i = 0; i < 3; i++) {
        ec_place(t, x + 3, y); pump(2);
        ec_place(t, x - 3, y); pump(2);
    }
    ec_place(t, x, y);
}

/* ------------------------------------------------------------------ helpers */

static int conscious(const EC* e) {
    return e->c->hp > 0 && !(r5_conds_effective(e->c->conds) & C_INCAPACITATED);
}
static int side_up(int side) {
    int n = 0;
    for (int i = 0; i < nec; i++)
        if (ec[i].side == side && ec[i].c->hp > 0) n++;
    return n;
}
static int party_conscious(void) {
    int n = 0;
    for (int i = 0; i < nec; i++)
        if (ec[i].side == 0 && conscious(&ec[i])) n++;
    return n;
}

static int bless_up(void) {
    for (int i = 0; i < nec; i++)
        if (ec[i].side == 0 && ec[i].c->concentrating == R5S_BLESS + 1) return 1;
    return 0;
}

static void break_conc(EC* e, const char* why) {
    if (!e->c->concentrating) return;
    int spell = e->c->concentrating - 1;
    e->c->concentrating = 0;
    if (spell == R5S_HUNTERS_MARK)
        for (int i = 0; i < nec; i++)
            if (ec[i].marked_by == (s8)(e - ec)) ec[i].marked_by = -1;
    char m[48]; char* d = m;
    d = put_str(d, r5_spells[spell].name);
    d = put_str(d, why);
    *d = 0;
    bar_wait(m);
}

/* apply damage + all bookkeeping (wake sleepers, concentration checks, death) */
/* wild shape ends: the druid steps back out of the beast */
static void shape_revert(EC* e, const char* why) {
    e->shaped = 0;
    e->c = &party5[e->pi];
    if (e->npc >= 0) npcs[e->npc].face = 0;
    mgba_logf("wildshape revert %s", why);
    if (why[0]) bar_wait(why);
}

static void deal(EC* t, int amount, u8 type, int scr_pop) {
    int was_sleeping = (t->c->hp > 0) && (t->c->conds & C_UNCONSCIOUS);
    if (t->c->immune & (u16)(1 << type)) {
        bar_wait("IMMUNE!");
        return;
    }
    if (t->c->resist & (u16)(1 << type)) bar("Resisted...");
    int shown = r5_scale_damage(t->c, amount, type);   /* one scale, for the popup */
    int lost = r5_apply_damage(t->c, amount, type);     /* scales the same raw once */
    if (scr_pop) popup(ec_sx(t) + 8, ec_sy(t), shown, DPAL(type));
    if (was_sleeping && t->c->hp > 0) {
        t->c->conds &= (u16)~C_UNCONSCIOUS;
        bar_wait("It jolts awake!");
    }
    if (t->shaped && t->c->hp <= 0) {
        shape_revert(t, "The shape breaks!");
        return;                       /* excess damage does not carry over */
    }
    if (t->c->hp > 0 && t->c->concentrating && lost > 0) {
        R5Save sv = r5_save(&rng, t->c, R5_CON, r5_conc_dc(lost), 0);
        mgba_logf("conc save d20=%d total=%d dc=%d %s", sv.d20.total, sv.total,
                  sv.dc, sv.success ? "ok" : "broken");
        if (!sv.success) break_conc(t, " slips away!");
    }
    if (t->c->hp <= 0) {
        if (t->side != 0) {
            /* blink out, leave the deck */
            for (int k = 0; k < 6; k++) {
                if (t->npc >= 0) npcs[t->npc].flags ^= NPC_GONE;
                pump(4);
            }
            if (t->npc >= 0) npcs[t->npc].flags |= NPC_GONE;
            if (t->mon == R5M_ZHALK) {
                bar_wait("Zhalk falls!");
                G.everburn = 1;
                G.flags |= GF_ZHALK_DEAD;
                /* the blade goes to the fighter if present, else the pack */
                int fi = -1;
                for (int pi2 = 0; pi2 < G.nparty; pi2++)
                    if (G.pm[pi2].cls == CLS_FIGHTER) fi = pi2;
                if (fi >= 0) {
                    loot_weapon(G.weapon[fi]);
                    G.weapon[fi] = R5W_EVERBURN;
                    bar_wait("Lae'zel takes the blade!");
                } else {
                    loot_weapon(R5W_EVERBURN);
                    bar_wait("Everburn Blade claimed!");
                }
                mgba_log("zhalk dead, everburn claimed");
            }
        } else {
            bar_wait("They crumple to the deck!");
        }
    }
}

/* attack flags from world state */
static int atk_flags(EC* a, EC* t, int melee) {
    int f = 0;
    if (a->side == 0 && bless_up()) f |= R5F_BLESS;
    if (a->hidden) { f |= R5F_ADV; }
    if (t->dodge) f |= R5F_DIS;
    if (t->mock) { f |= R5F_DIS; }
    if (a->gbolt) { f |= R5F_ADV; }
    if (t->c->conds & C_UNCONSCIOUS) {
        f |= R5F_ADV;
        if (melee) f |= R5F_AUTOCRIT;
    }
    if (!melee && a->engaged >= 0 && conscious(&ec[a->engaged]))
        f |= R5F_DIS;                       /* ranged while in melee */
    if (!melee && encounter_dark() && !(a->c->traits & TR_DARKVISION)) {
        f |= R5F_DIS;      /* the dark (doctrine): ranged and spell attacks
                            * at disadvantage without darkvision -- melee
                            * exempt, per actor, both sides */
        mgba_logf("dark dis %s", a->c->name);
    }
    if (t->marked_by == (s8)(a - ec)) f |= R5F_MARK;
    if (a->side == 0 && G.pm[a->pi].cls == CLS_ROGUE) {
        int adv = (f & R5F_ADV) && !(f & R5F_DIS);
        int ally_engaged = 0;
        for (int i = 0; i < nec; i++)
            if (i != (int)(a - ec) && ec[i].side != t->side &&
                conscious(&ec[i]) && ec[i].engaged == (s8)(t - ec))
                ally_engaged = 1;
        if (adv || ally_engaged) f |= R5F_SNEAK;
    }
    return f;
}

static void post_attack(EC* a, EC* t, const R5Attack* at) {
    if (a->hidden) { a->hidden = 0; }
    if (a->gbolt) a->gbolt = 0;
    if (t->mock) t->mock = 0;
    (void)at;
}

/* opportunity attack when a melee combatant turns away from its opponent */
static void maybe_opportunity(EC* a, EC* new_target) {
    if (a->engaged < 0) return;
    EC* old = &ec[a->engaged];
    if (old == new_target || !conscious(old) || old->reacted) return;
    old->reacted = 1;
    bar_wait("Opportunity attack!");
    ec_face_toward(old, a);
    R5Attack at;
    if (old->side == 0 && !old->shaped) {
        at = r5_weapon_attack(&rng, old->c, a->c,
                              &r5_weapons[party5_weapon(old->pi)], 0);
    } else {
        const R5MAttack* ma = &r5_monsters[old->mon].attacks[0];
        at = r5_monster_attack(&rng, old->c, ma, a->c, 0);
    }
    tray_attack(&at);
    bar_attack("OA:", &at);
    if (at.hit) { hit_react(a); deal(a, at.dmg.total, at.dmg_type, 1); bar_damage(&at); }
    a->engaged = -1;
}

static const R5MAttack* ai_ma;   /* AI-chosen monster attack for next strike */

/* full melee/ranged weapon strike */
static void strike(EC* a, EC* t) {
    int as_beast = a->side != 0 || a->shaped;
    const R5Weapon* w = as_beast ? 0 : &r5_weapons[party5_weapon(a->pi)];
    const R5MAttack* mona = 0;
    if (as_beast) {
        mona = a->shaped ? &r5_monsters[a->mon].attacks[0]
                         : (ai_ma ? ai_ma : &r5_monsters[a->mon].attacks[0]);
        ai_ma = 0;
    }
    int melee = as_beast ? !mona->ranged : !(w->props & WP_RANGED);
    if (melee) {
        maybe_opportunity(a, t);
        if (a->c->hp <= 0) return;          /* OA dropped us */
        dash_to(a, t, 18);
    } else ec_face_toward(a, t);

    int f = atk_flags(a, t, melee);
    R5Attack at;
    const char* verb;
    if (!as_beast) {
        at = r5_weapon_attack(&rng, a->c, t->c, w, f);
        verb = w->name;
    } else {
        at = r5_monster_attack(&rng, a->c, mona, t->c, f);
        verb = mona->name;
    }
    tray_attack(&at);
    bar_attack(verb, &at);
    if (a->side == 0 && (f & R5F_SNEAK) && at.hit) bar_wait("Sneak attack!");
    mgba_logf("atk %s->%s d20=%d tot=%d v%d %s dmg=%d rider=%d",
              a->c->name, t->c->name, at.d20.total, at.total, at.target_ac,
              at.crit ? "CRIT" : at.hit ? "hit" : "miss",
              at.damage, at.rider_damage);
    if (at.hit) {
        hit_react(t);
        deal(t, at.dmg.total, at.dmg_type, 1);
        bar_damage(&at);
        if (at.rider_dmg.n && t->c->hp > 0 && at.rider_damage > 0)
            deal(t, at.rider_dmg.total, at.rider_type, 1);
        if (a->smite && a->side == 0 && melee && t->c->hp > 0 &&
            r5_spend_slot(a->c, 1)) {                    /* Divine Smite */
            a->smite = 0;
            R5DiceSpec sd = r5_smite_dice(1);
            R5Dice d = r5_roll(&rng, at.crit ? sd.n * 2 : sd.n, sd.sides, 0);
            tray_dice(&d, DPAL(DT_RADIANT));
            bar_wait("SMITE!");
            deal(t, d.total, DT_RADIANT, 1);
        }
        if (melee && t->c->hp > 0) {
            a->engaged = (s8)(t - ec); t->engaged = (s8)(a - ec);
            mgba_logf("engage %s<->%s", a->c->name, t->c->name);
        }
    }
    post_attack(a, t, &at);
    if (melee) dash_home(a);
    tray_clear();
}

/* ------------------------------------------------------------------ spells */

static void cast_attack_spell(EC* a, EC* t, int sp) {
    const R5Spell* s = &r5_spells[sp];
    R5MAttack ma = { s->name, (s8)party5_spell_atk(a->c), s->dice,
                     s->dmg_type, { 0, 0, 0 }, 0, 0, 0, 1 };
    ec_face_toward(a, t);
    int f = atk_flags(a, t, 0);
    R5Attack at = r5_monster_attack(&rng, a->c, &ma, t->c, f);
    tray_attack(&at);
    bar_attack(s->name, &at);
    mgba_logf("spell %s d20=%d tot=%d v%d %s dmg=%d", s->name, at.d20.total,
              at.total, at.target_ac, at.hit ? "hit" : "miss", at.damage);
    if (at.hit) {
        hit_react(t);
        deal(t, at.dmg.total, at.dmg_type, 1);
        bar_damage(&at);
        if (sp == R5S_GUIDING_BOLT) { t->gbolt = 1; bar_wait("Light clings to it!"); }
    }
    post_attack(a, t, &at);
    tray_clear();
}

static void cast_save_spell(EC* a, EC* t, int sp) {
    const R5Spell* s = &r5_spells[sp];
    int dc = party5_spell_dc(a->c);
    ec_face_toward(a, t);
    R5Save sv = r5_save(&rng, t->c, s->save_ab, dc, 0);
    tray_clear();
    tray_headline_d20(&sv.d20, 10);
    char m[48]; char* d = m;
    d = put_str(d, "save "); d = put_num(d, sv.d20.total);
    d = put_str(d, " v DC "); d = put_num(d, sv.dc);
    d = put_str(d, sv.success ? " ok" : " FAIL");
    *d = 0;
    bar_wait(m);
    mgba_logf("save-spell %s d20=%d tot=%d dc=%d %s", s->name, sv.d20.total,
              sv.total, sv.dc, sv.success ? "save" : "fail");
    R5Dice dd = r5_roll(&rng, s->dice.n, s->dice.sides, s->dice.mod);
    int dmg = dd.total;
    if (sv.success) dmg = s->save_half ? dmg / 2 : 0;
    if (dmg > 0) {
        tray_dice(&dd, DPAL(s->dmg_type));
        hit_react(t);
        deal(t, dmg, s->dmg_type, 1);
    }
    if (!sv.success && sp == R5S_VICIOUS_MOCKERY) {
        t->mock = 1;
        bar_wait("It reels, stung!");
    }
    tray_clear();
}

static void cast_missiles(EC* a) {
    (void)a;   /* darts pick their own targets */
    for (int dart = 0; dart < r5_spells[R5S_MAGIC_MISSILE].count; dart++) {
        EC* t = 0;                          /* lowest-hp living enemy */
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 1 && ec[i].c->hp > 0 &&
                (!t || ec[i].c->hp < t->c->hp)) t = &ec[i];
        if (!t) return;
        R5Dice d = r5_roll(&rng, 1, 4, 1);
        tray_clear();
        tray_dice(&d, DPAL(DT_FORCE));
        bar("Magic Missile!");
        hit_react(t);
        deal(t, d.total, DT_FORCE, 1);
        mgba_logf("missile %d -> %s", d.total, t->c->name);
    }
    tray_clear();
}

static void cast_sleep(EC* a) {
    (void)a;  /* area spell: no single target */
    R5Dice pool = r5_roll(&rng, 5, 8, 0);
    tray_clear();
    tray_dice(&pool, 14);
    mgba_logf("sleep pool=%d", pool.total);
    char m[48]; char* d = m;
    d = put_str(d, "Sleep! 5d8 = "); d = put_num(d, pool.total); *d = 0;
    bar_wait(m);
    int left = pool.total;
    for (;;) {
        EC* t = 0;                          /* lowest current HP, awake */
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 1 && ec[i].c->hp > 0 &&
                !(ec[i].c->conds & C_UNCONSCIOUS) &&
                (!t || ec[i].c->hp < t->c->hp)) t = &ec[i];
        if (!t || t->c->hp > left) break;
        left -= t->c->hp;
        t->c->conds |= C_UNCONSCIOUS;
        char b[48]; char* e = b;
        e = put_str(e, t->c->name); e = put_str(e, " falls asleep!"); *e = 0;
        bar_wait(b);
    }
    tray_clear();
}

static void cast_heal(EC* a, EC* t, int sp) {
    const R5Spell* s = &r5_spells[sp];
    int mod = s->add_mod ? r5_mod(a->c->ab[party5_cast_ab(a->c->cls)]) : 0;
    R5Dice d = r5_roll(&rng, s->dice.n, s->dice.sides, s->dice.mod + mod);
    if (a->c->heal_boost && s->level > 0) d.total += a->c->heal_boost;  /* Life */
    tray_clear();
    tray_dice(&d, 8);
    sfx_play(SFX_HEAL);
    int wake = t->c->hp <= 0;
    r5_heal(t->c, d.total);
    popup(ec_sx(t) + 8, ec_sy(t), d.total, 8);
    if (wake) bar_wait("Back on their feet!");
    mgba_logf("heal %s +%d -> %d", t->c->name, d.total, t->c->hp);
    tray_clear();
}

/* ------------------------------------------------------------------ turns */


/* expected value of a monster attack vs a target, x100, defense-scaled */
static int ai_ev(const R5MAttack* ma, const EC* t, int flags) {
    int ev = r5_ev_attack_x100(ma->to_hit, t->c->ac, flags, ma->dmg);
    u16 bit = (u16)(1 << ma->dmg_type);
    if (t->c->immune & bit) ev = 0;
    else if (t->c->resist & bit) ev /= 2;
    else if (t->c->vulnerable & bit) ev *= 2;
    if (ma->rider_dmg.n && !(t->c->immune & (u16)(1 << ma->rider_type))) {
        /* rider: hit% x ~75% of dice EV (save-for-half average) */
        int re = 2 * ma->rider_dmg.n * (ma->rider_dmg.sides + 1);   /* x4 */
        ev += r5_hit_pct(ma->to_hit, t->c->ac, flags) * re * 3 / 16;
    }
    return ev;
}

static EC* nearest_foe(EC* a, int want_side) {
    EC* best = 0; int bd = 0x7FFF;
    for (int i = 0; i < nec; i++) {
        EC* t = &ec[i];
        if (t->side != want_side || t->c->hp <= 0) continue;
        if (want_side == 0 && !conscious(t) && party_conscious()) continue;
        if (t->hidden) continue;
        int dx = ec_x(t) - ec_x(a), dy = ec_y(t) - ec_y(a);
        int d = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        if (d < bd) { bd = d; best = t; }
    }
    return best;
}

static void warp_cambion(void) {
    if (nwarp >= 2 || nec >= MAXEC) return;
    int npc = field_add_npc(14, 3, OBJT_ZHALKF, 6, 2, 0);
    if (npc < 0) return;
    warp_npc[nwarp++] = npc;
    add_mon(R5M_CAMBION, npc, 1, 150);
    bar_wait("A cambion warps in!");
    mgba_log("cambion warps in");
    frame_camera();
}

static void enemy_turn(EC* a) {
    /* Zhalk's obsession: the mind flayer, while it stands */
    if (a->mon == R5M_ZHALK) {
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 2 && ec[i].mon == R5M_FLAYER && conscious(&ec[i])) {
                ai_ma = &r5_monsters[R5M_ZHALK].attacks[0];
                strike(a, &ec[i]);
                return;
            }
    }
    const R5Monster* m = &r5_monsters[a->mon];
    EC* best_t = 0;
    const R5MAttack* best_a = 0;
    int best = -1;
    for (int ai2 = 0; ai2 < m->n_attacks; ai2++) {
        const R5MAttack* ma = &m->attacks[ai2];
        for (int i = 0; i < nec; i++) {
            EC* t = &ec[i];
            if (t->side == 1 || t->c->hp <= 0) continue;
            if (t->side == 0 && !conscious(t) && party_conscious()) continue;
            if (t->hidden) continue;
            int f = t->dodge ? R5F_DIS : 0;
            int ev = ai_ev(ma, t, f);
            int score = ev;
            if (t->c->hp * 100 <= ev * 2) score += 150;      /* blood scent */
            score += (int)(rnd_range(30));                   /* temperament */
            if (score > best) { best = score; best_t = t; best_a = ma; }
        }
    }
    if (!best_t) return;
    mgba_logf("ai %s: %s -> %s ev=%d", a->c->name, best_a->name,
              best_t->c->name, best);
    ai_ma = best_a;
    strike(a, best_t);
}

static void ally_turn(EC* a) {
    if (a->mon == R5M_FLAYER) {
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 1 && ec[i].mon == R5M_ZHALK && ec[i].c->hp > 0) {
                ai_ma = &r5_monsters[R5M_FLAYER].attacks[0];
                strike(a, &ec[i]);
                return;
            }
    }
    EC* t = nearest_foe(a, 1);
    if (!t) return;
    strike(a, t);
}

/* --- player menus --- */

static int menu5(const char* const* items, int n, int cancel) {
    win_draw(16, 15, 14, 5);
    int sel = 0, top = 0;
    for (;;) {
        for (int r = 0; r < 3; r++) {
            txt_clear(18, 16 + r, 11, 1);
            if (top + r < n) txt_put(18, 16 + r, items[top + r], 0);
        }
        if (top > 0) txt_put(28, 16, "-", 1);
        if (top + 3 < n) txt_put(28, 18, "+", 1);
        for (;;) {
            obj_set(OBJ_CURSOR, 16 * 8 - 6, (16 + sel - top) * 8 - 4, 1, OBJT_HAND, 7, 0);
            pump(1);
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); if (sel < top) top--; break; }
            if (k & KEY_DOWN && sel < n - 1) { sel++; sfx_play(SFX_CURSOR); if (sel > top + 2) top++; break; }
            if (k & KEY_A) { sfx_play(SFX_CONFIRM); obj_hide(OBJ_CURSOR); win_clear(16, 15, 14, 5); return sel; }
            if ((k & KEY_B) && cancel) { sfx_play(SFX_CANCEL); obj_hide(OBJ_CURSOR); win_clear(16, 15, 14, 5); return -1; }
        }
    }
}

static EC* pick5(int side, int allow_downed) {
    EC* cands[MAXEC]; int n = 0;
    for (int i = 0; i < nec; i++) {
        EC* t = &ec[i];
        if ((int)t->side != side) continue;
        if (side == 0 ? !allow_downed && t->c->hp <= 0 : t->c->hp <= 0) continue;
        cands[n++] = t;
    }
    if (!n) return 0;
    int sel = 0;
    for (;;) {
        EC* t = cands[sel];
        obj_set(OBJ_CURSOR, ec_sx(t) - 12, ec_sy(t), 1, OBJT_HAND, 7, 0);
        if (tsel_a) {   /* attack targeting: provoke warning + sneak pip */
            EC* old = tsel_a->engaged >= 0 ? &ec[tsel_a->engaged] : 0;
            if (old && old != t && conscious(old) && !old->reacted)
                obj_set(OBJ_TETH + 3, ec_sx(t) + 6, ec_sy(t) - 16, 1,
                        OBJT_ALERT, 11, 0);                 /* red !: provokes */
            else obj_hide(OBJ_TETH + 3);
            if (G.pm[tsel_a->pi].cls == CLS_ROGUE && sneak_ok(tsel_a, t))
                obj_set(OBJ_TETH + 4, ec_sx(t) - 10, ec_sy(t) - 10, 0,
                        OBJT_GARN + 1, 8, 0);               /* green pip: sneak */
            else obj_hide(OBJ_TETH + 4);
        }
        pump(1);
        u16 k = key_hit();
        if (k & (KEY_UP | KEY_LEFT)) { sel = (sel + n - 1) % n; sfx_play(SFX_CURSOR); }
        if (k & (KEY_DOWN | KEY_RIGHT)) { sel = (sel + 1) % n; sfx_play(SFX_CURSOR); }
        if (k & KEY_A) { sfx_play(SFX_CONFIRM); obj_hide(OBJ_CURSOR);
                         obj_hide(OBJ_TETH + 3); obj_hide(OBJ_TETH + 4); return t; }
        if (k & KEY_B) { sfx_play(SFX_CANCEL); obj_hide(OBJ_CURSOR);
                         obj_hide(OBJ_TETH + 3); obj_hide(OBJ_TETH + 4); return 0; }
    }
}

/* DQ-style tactics: one auto-player, five temperaments. Returns 1 when the
 * hero connects the transponder (helm objective). Demo mode = Wisely. */
static int tactic_turn(EC* a, int tac) {
    R5Creature* c = a->c;
    int cls = G.pm[a->pi].cls;
    int use_slots = (tac != TAC_NOSLOTS);
    EC* foe = nearest_foe(a, 1);

    /* helm objective: the hero goes for the nerves (demo kill-mode
     * excepted). Demo fallback: if Tav lies downed when the countdown
     * runs, a conscious companion seizes them instead -- manual play
     * keeps the rule that Tav must be standing (design, not a bug). */
    if (rounds_left > 0 && G_DEMO && G_DEMO_BATTLE != 2 && round_no >= 2 &&
        (a->pi == 0 || party5[0].hp <= 0)) {
        mgba_logf("nerve seized pi=%d", a->pi);
        bar_wait("You seize the nerves...");
        return 1;
    }
    if (G_DEMO && G_DEMO_BATTLE == 2)
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 1 && ec[i].mon == R5M_ZHALK && ec[i].c->hp > 0)
                foe = &ec[i];

    /* ---- bonus actions ---- */
    if (cls == CLS_FIGHTER && r5_can_second_wind(c) &&
        (tac == TAC_ALLOUT ? c->hp * 4 < c->hpmax : c->hp * 2 < c->hpmax)) {
        R5Dice d = r5_second_wind(&rng, c);
        tray_clear();
        tray_dice(&d, 8);
        bar_wait("Second Wind!");
        popup(ec_sx(a) + 8, ec_sy(a), d.total, 8);
    }
    if (cls == CLS_ROGUE && !a->hidden && foe) {
        a->hidden = 1;
        bar_wait("Hides in the chaos!");
    }
    if (cls == CLS_RANGER && use_slots && tac != TAC_HEALER && foe &&
        !c->concentrating && c->slots[0] && r5_spend_slot(c, 1)) {
        c->concentrating = R5S_HUNTERS_MARK + 1;
        foe->marked_by = (s8)(a - ec);
        bar_wait("Hunter's Mark!");
    }
    if (cls == CLS_BARD && use_slots && tac != TAC_ALLOUT && c->slots[0]) {
        /* healing word (bonus): Healer patches wounds, others save the downed */
        EC* best = 0;
        for (int i = 0; i < nec; i++) {
            if (ec[i].side != 0) continue;
            R5Creature* pc = ec[i].c;
            int hurt = tac == TAC_HEALER ? pc->hp * 4 < pc->hpmax * 3
                                         : pc->hp <= 0;
            if (hurt && (!best || pc->hp < best->c->hp)) best = &ec[i];
        }
        if (best && r5_spend_slot(c, 1))
            cast_heal(a, best, R5S_HEALING_WORD);
    }

    /* ---- action ---- */
    if (cls == CLS_CLERIC && use_slots && tac != TAC_ALLOUT) {
        EC* best = 0;
        for (int i = 0; i < nec; i++) {
            if (ec[i].side != 0) continue;
            R5Creature* pc = ec[i].c;
            int hurt = tac == TAC_HEALER ? pc->hp * 4 < pc->hpmax * 3
                                         : pc->hp <= 0;
            if (hurt && (!best || pc->hp < best->c->hp)) best = &ec[i];
        }
        if (best && r5_spend_slot(c, 1)) {
            cast_heal(a, best, R5S_CURE_WOUNDS);
            return 0;
        }
    }
    if (!foe) return 0;
    switch (cls) {
        case CLS_WIZARD: {
            int fire_ok = !(foe->c->immune & (1 << DT_FIRE));
            int spend = use_slots && c->slots[0];
            if (spend && tac != TAC_HEALER && side_up(1) >= 3 &&
                r5_spend_slot(c, 1)) cast_sleep(a);
            else if (spend && (tac == TAC_ALLOUT || !fire_ok) &&
                     r5_spend_slot(c, 1)) cast_missiles(a);
            else if (fire_ok) cast_attack_spell(a, foe, R5S_FIRE_BOLT);
            else strike(a, foe);
            break;
        }
        case CLS_BARD: cast_save_spell(a, foe, R5S_VICIOUS_MOCKERY); break;
        case CLS_CLERIC:
            if (use_slots && c->slots[0] && r5_spend_slot(c, 1))
                cast_attack_spell(a, foe, R5S_GUIDING_BOLT);
            else strike(a, foe);
            break;
        default: strike(a, foe); break;
    }
    if (cls == CLS_FIGHTER && r5_can_action_surge(c) && foe->c->hp > 0) {
        r5_use_action_surge(c);
        bar_wait("Action Surge!");
        strike(a, foe);
    }
    return 0;
}

/* default battle kits: every caster class casts out of the box. Leveled
 * rows gate on slots (pact for warlocks), concentration rows on focus.
 * Superseded per-member once known/prepared choices land (Character 2.0). */
static const struct { u8 cls, spell; const char* label; } spell_kit[] = {
    { CLS_BARD,     R5S_VICIOUS_MOCKERY, "V.Mockery" },
    { CLS_BARD,     R5S_HEALING_WORD,    "Heal.Word" },
    { CLS_WIZARD,   R5S_FIRE_BOLT,       "Fire Bolt" },
    { CLS_WIZARD,   R5S_MAGIC_MISSILE,   "M.Missile" },
    { CLS_WIZARD,   R5S_SLEEP,           "Sleep"     },
    { CLS_CLERIC,   R5S_SACRED_FLAME,    "S.Flame"   },
    { CLS_CLERIC,   R5S_GUIDING_BOLT,    "Guid.Bolt" },
    { CLS_CLERIC,   R5S_CURE_WOUNDS,     "Cure Wnds" },
    { CLS_CLERIC,   R5S_BLESS,           "Bless"     },
    { CLS_DRUID,    R5S_PRODUCE_FLAME,   "P.Flame"   },
    { CLS_DRUID,    R5S_POISON_SPRAY,    "Psn.Spray" },
    { CLS_DRUID,    R5S_CURE_WOUNDS,     "Cure Wnds" },
    { CLS_DRUID,    R5S_HEALING_WORD,    "Heal.Word" },
    { CLS_SORCERER, R5S_FIRE_BOLT,       "Fire Bolt" },
    { CLS_SORCERER, R5S_RAY_OF_FROST,    "RayFrost"  },
    { CLS_SORCERER, R5S_MAGIC_MISSILE,   "M.Missile" },
    { CLS_SORCERER, R5S_SLEEP,           "Sleep"     },
    { CLS_WARLOCK,  R5S_ELDRITCH_BLAST,  "Eld.Blast" },
    { CLS_WARLOCK,  R5S_BURNING_HANDS,   "Burn.Hands" },
    { CLS_PALADIN,  R5S_CURE_WOUNDS,     "Cure Wnds" },
    { CLS_PALADIN,  R5S_BLESS,           "Bless"     },
};

/* Root battle-menu build, extracted verbatim from pc_turn so the host
 * harness (test/host) can pin the exact kit per class/level. Codes: fixed
 * actions 0..9, class features 20..26, spells SPELL_CODE + R5S_id. */
int pc_menu_build(const R5Creature* c, const PcMenuCtx* x,
                  const char** items, u8* code) {
    int cls = x->cls, n = 0;
    if (!x->action) {
        items[n] = "Attack"; code[n++] = 0;
        if (cls == CLS_DRUID && !x->shaped && x->level >= 2 &&
            c->rsrc[R5R_SHAPE]) { items[n] = "WildShape"; code[n++] = 25; }
        for (unsigned ki = 0; x->shaped == 0 &&
             ki < sizeof spell_kit / sizeof *spell_kit; ki++) {
            if (spell_kit[ki].cls != cls) continue;
            const R5Spell* s = &r5_spells[spell_kit[ki].spell];
            if (s->bonus_action) continue;              /* bonus section */
            if (s->level > 0) {
                int have = cls == CLS_WARLOCK ? c->rsrc[R5R_PACT]
                                              : c->slots[s->level - 1];
                if (!have) continue;
            }
            if (s->concentration && c->concentrating) continue;
            items[n] = spell_kit[ki].label; code[n++] = (u8)(SPELL_CODE + spell_kit[ki].spell);
        }
        if (cls == CLS_PALADIN && c->rsrc[R5R_LAY]) { items[n] = "Lay Hands"; code[n++] = 22; }
        if (cls == CLS_PALADIN && c->slots[0] && !x->smited) { items[n] = "Smite"; code[n++] = 24; }
        items[n] = "Item"; code[n++] = 1;
        items[n] = "Dodge"; code[n++] = 2;
        if (x->engaged >= 0) { items[n] = "Disengage"; code[n++] = 3; }
    }
    if (!x->bonus) {
        if (cls == CLS_FIGHTER && r5_can_second_wind(c)) { items[n] = "2nd Wind"; code[n++] = 4; }
        if (r5_can_rage(c)) { items[n] = "RAGE!"; code[n++] = 20; }
        if (cls == CLS_MONK && x->action && c->rsrc[R5R_KI]) { items[n] = "Flurry"; code[n++] = 21; }
        if (cls == CLS_MONK && c->rsrc[R5R_KI]) { items[n] = "Pat.Def"; code[n++] = 23; }
        if (cls == CLS_ROGUE && !x->hidden) { items[n] = "Hide"; code[n++] = 5; }
        for (unsigned ki = 0; ki < sizeof spell_kit / sizeof *spell_kit; ki++) {
            if (spell_kit[ki].cls != cls) continue;
            const R5Spell* s = &r5_spells[spell_kit[ki].spell];
            if (!s->bonus_action) continue;
            if (s->level > 0 && !(cls == CLS_WARLOCK ? c->rsrc[R5R_PACT]
                                                     : c->slots[s->level - 1])) continue;
            items[n] = spell_kit[ki].label; code[n++] = (u8)(SPELL_CODE + spell_kit[ki].spell);
        }
        if (cls == CLS_RANGER && c->slots[0] && !c->concentrating) { items[n] = "Hunt.Mark"; code[n++] = 6; }
    }
    /* Action Surge is neither the action nor the bonus action (free, once
     * per rest), so it lives outside both sections -- same gate the tactic
     * AI checks before its extra strike. */
    if (cls == CLS_FIGHTER && r5_can_action_surge(c)) { items[n] = "ActSurge"; code[n++] = 26; }
    if (x->nerve) {
        items[n] = "Nerve!"; code[n++] = 8;
    }
    items[n] = "Tactics"; code[n++] = 9;
    items[n] = "End Turn"; code[n++] = 7;
    return n;
}

/* Ability audit (testability): logs the REAL root battle menu -- items and
 * dispatch codes straight out of pc_menu_build -- for every class x level,
 * each fed a canonical full-resource turn-start member (party5_refresh +
 * r5_refill, not engaged/shaped/concentrating). No hand-kept mirror of the
 * gates: the old mirror listed ActSurge while the menu itself lacked the
 * row, the exact drift this dump exists to catch. */
void ability_audit(void) {
    PMember pmsave = G.pm[0];
    R5Creature csave = party5[0];
    for (int cls = 0; cls < CLS_COUNT; cls++) {
        for (int lvl = 1; lvl <= 3; lvl++) {
            G.pm[0].cls = (u8)cls;
            G.pm[0].level = (u8)lvl;
            G.pm[0].subclass = 255;              /* no subclass passives */
            party5_refresh(0);
            R5Creature* c = &party5[0];
            c->hp = c->hpmax;                    /* canonical turn 1: */
            c->used = 0;                         /* nothing spent yet */
            c->concentrating = 0;
            for (int s = 0; s < 3; s++)
                c->slots[s] = r5_classes[cls].slots[lvl][s];
            r5_refill(c);
            const char* items[12]; u8 code[12];
            PcMenuCtx mx = { (u8)cls, (u8)lvl, 0, 0, 0, -1, 0, 0, 0 };
            int n = pc_menu_build(c, &mx, items, code);
            char line[200]; char* d = line;
            d = put_str(d, "audit cls="); d = put_num(d, cls);
            d = put_str(d, " L"); d = put_num(d, lvl); d = put_str(d, ":");
            for (int i = 0; i < n; i++) {
                *d++ = ' ';
                d = put_str(d, items[i]);
                *d++ = '='; d = put_num(d, code[i]);
            }
            *d = 0;
            mgba_log(line);
        }
    }
    G.pm[0] = pmsave;            /* exact restore: sheet and 5e twin both */
    party5[0] = csave;
}

static int pc_turn(EC* a) {
    int tac = (G_DEMO && !G_MANUAL_BAT) ? TAC_WISELY : G.tactics[a->pi];
    if (tac != TAC_ORDERS) return tactic_turn(a, tac);

    R5Creature* c = a->c;
    int cls = G.pm[a->pi].cls;
    int action = 0, bonus = 0;
    for (;;) {
        const char* items[12]; u8 code[12];
        PcMenuCtx mx = {
            (u8)cls, G.pm[a->pi].level, a->shaped, a->smite, a->hidden,
            a->engaged, (u8)action, (u8)bonus,
            (u8)(rounds_left > 0 && a->pi == 0 && round_no >= 2),
        };
        int n = pc_menu_build(c, &mx, items, code);

        int sel = menu5(items, n, 0);
        u8 cd = code[sel];
        crumb(CR_MENU, cd);
        if (cd == 7) break;
        switch (cd) {
            case 0: {
                tsel_a = a;
                EC* t = pick5(1, 0);
                tsel_a = 0;
                if (!t) break;
                strike(a, t);
                action = 1;
                break;
            }
            case 1: {
                static const char* const it[] = { "Potion", "Revivify" };
                int s = menu5(it, 2, 1);
                if (s < 0) break;
                if (s == 0) {
                    if (!G.potions) { bar_wait("No potions!"); break; }
                    EC* t = pick5(0, 0);
                    if (!t) break;
                    G.potions--;
                    R5Dice d = r5_roll(&rng, 2, 4, 2);   /* potion of healing */
                    sfx_play(SFX_HEAL);
                    r5_heal(t->c, d.total);
                    popup(ec_sx(t) + 8, ec_sy(t), d.total, 8);
                } else {
                    if (!G.revivify) { bar_wait("No scrolls!"); break; }
                    EC* t = pick5(0, 1);
                    if (!t || t->c->hp > 0) { bar_wait("No one is down."); break; }
                    G.revivify--;
                    r5_heal(t->c, 1);
                    bar_wait("Revivify!");
                }
                action = 1;
                break;
            }
            case 2: a->dodge = 1; bar_wait("Dodging!"); action = 1; break;
            case 20:
                r5_start_rage(c);
                sfx_noise(16);
                bar_wait("RAGE! Blades will not bite.");
                bonus = 1;
                break;
            case 21: {                                   /* Flurry of Blows */
                EC* t = pick5(1, 0);
                if (!t) break;
                r5_spend(c, R5R_KI, 1);
                static const R5Weapon fists = { "Flurry", { 1, 4, 0 }, { 0, 0, 0 },
                                                WP_FINESSE, DT_BLUDGEONING,
                                                { 0, 0, 0 }, 0, 0, 0 };
                for (int fb = 0; fb < 2 && t->c->hp > 0; fb++) {
                    R5Attack at = r5_weapon_attack(&rng, c, t->c, &fists, atk_flags(a, t, 1));
                    tray_attack(&at); bar_attack("Fists", &at);
                    if (at.hit) { hit_react(t); deal(t, at.dmg.total, at.dmg_type, 1); bar_damage(&at); }
                    post_attack(a, t, &at);
                }
                tray_clear();
                bonus = 1;
                break;
            }
            case 22: {                                   /* Lay on Hands */
                EC* t = pick5(0, 1);
                if (!t) break;
                int amt = c->rsrc[R5R_LAY] < 5 ? c->rsrc[R5R_LAY] : 5;
                r5_lay_hands(c, t->c, amt);
                sfx_play(SFX_HEAL);
                popup(ec_sx(t) + 8, ec_sy(t), amt, 8);
                bar_wait("Healing hands.");
                action = 1;
                break;
            }
            case 23:
                r5_spend(c, R5R_KI, 1);
                a->dodge = 1;
                bar_wait("Patient defense.");
                bonus = 1;
                break;
            case 24:
                a->smite = 1;
                bar_wait("Radiance coils in the blade.");
                break;
            case 25: {                               /* Wild Shape: the Aeon moment */
                r5_spend(c, R5R_SHAPE, 1);
                mon_to_creature(R5M_BOAR, &a->store);
                a->mon = R5M_BOAR;
                a->shaped = 1;
                a->c = &a->store;
                c = a->c;
                if (a->npc >= 0) npcs[a->npc].objt = OBJT_BOARW;   /* the beast */
                sfx_noise(20);
                mgba_logf("wildshape boar pi=%d", a->pi);
                bar_wait("The beast takes you: BOAR!");
                action = 1;
                break;
            }
            case 26:                       /* Action Surge: an extra action,
                                            * same helpers the tactic AI uses */
                r5_use_action_surge(c);
                bar_wait("Action Surge!");
                action = 0;
                break;
            case 3:
                a->engaged = -1;
                bar_wait("Disengages cleanly.");
                action = 1;
                break;
            case 4: {
                R5Dice d = r5_second_wind(&rng, c);
                tray_clear();
                tray_dice(&d, 8);
                bar_wait("Second Wind!");
                popup(ec_sx(a) + 8, ec_sy(a), d.total, 8);
                bonus = 1;
                break;
            }
            case 5: a->hidden = 1; bar_wait("Hidden!"); bonus = 1; break;
            case 6: {
                EC* t = pick5(1, 0);
                if (!t) break;
                if (r5_spend_slot(c, 1)) {
                    c->concentrating = R5S_HUNTERS_MARK + 1;
                    t->marked_by = (s8)(a - ec);
                    bar_wait("Hunter's Mark!");
                }
                bonus = 1;
                break;
            }
            case 8:
                bar_wait("You seize the nerves...");
                return 1;
            case 9: {
                const char* mnames[3];
                for (int i = 0; i < G.nparty; i++) mnames[i] = G.pm[i].name;
                int mi = menu5(mnames, G.nparty, 1);
                if (mi < 0) break;
                static const char* const tnames[TAC_COUNT] = {
                    "Orders", "Wisely", "All Out", "Healer", "No Slots"
                };
                int tv = menu5(tnames, TAC_COUNT, 1);
                if (tv < 0) break;
                G.tactics[mi] = (u8)tv;
                bar_wait("Tactics set.");
                break;
            }
            default: {
                int sp = cd - SPELL_CODE;
                ASSERT(cd >= SPELL_CODE && sp < R5S_COUNT);
                crumb(CR_CAST, sp);
                const R5Spell* s = &r5_spells[sp];
                int paid = s->level == 0 ? 1
                         : cls == CLS_WARLOCK ? r5_pact_cast(c)
                                              : r5_spend_slot(c, s->level);
                if (!paid) { bar_wait("No slots!"); break; }
                if (sp == R5S_MAGIC_MISSILE) cast_missiles(a);
                else if (sp == R5S_SLEEP) cast_sleep(a);
                else if (sp == R5S_BLESS) { c->concentrating = R5S_BLESS + 1; bar_wait("Bless!"); }
                else if (s->heal) {
                    EC* t = pick5(0, 1);
                    if (!t) break;
                    cast_heal(a, t, sp);
                } else if (s->attack) {
                    EC* t = pick5(1, 0);
                    if (!t) break;
                    cast_attack_spell(a, t, sp);
                } else {
                    EC* t = pick5(1, 0);
                    if (!t) break;
                    cast_save_spell(a, t, sp);
                }
                if (s->bonus_action) bonus = 1; else action = 1;
                break;
            }
        }
        if (action && bonus) break;
        if (side_up(1) == 0) break;
    }
    return 0;
}

/* ------------------------------------------------------------------ setup */

static void add_pc(int pi, int x, int y, int npc) {
    EC* e = &ec[nec++];
    e->c = &party5[pi];
    e->hx = (s16)x; e->hy = (s16)y;
    e->npc = (s8)npc;
    e->side = 0; e->pi = (u8)pi;
    e->engaged = -1; e->marked_by = -1;
    e->reacted = e->dodge = e->hidden = e->gbolt = e->mock = 0;
    e->xp = 0;
    ec_place(e, x, y);
}

static void mon_to_creature(int mon, R5Creature* c) {
    const R5Monster* m = &r5_monsters[mon];
    c->name = m->name;
    for (int i = 0; i < 6; i++) c->ab[i] = m->ab[i];
    c->hp = c->hpmax = m->hp;
    c->temp_hp = 0;
    c->ac = m->ac;
    c->level = 1; c->cls = R5C_MONSTER;
    c->save_prof = 0;
    c->conds = 0;
    c->resist = m->resist; c->immune = m->immune; c->vulnerable = m->vulnerable;
    c->slots[0] = c->slots[1] = c->slots[2] = 0;
    c->used = 0; c->concentrating = 0;
    c->traits = m->traits;           /* darkvision rides the stat block */
    for (int i = 0; i < R5R_COUNT; i++) c->rsrc[i] = 0;
}

static void add_mon(int mon, int npc, int side, u16 xp) {
    EC* e = &ec[nec++];
    mon_to_creature(mon, &e->store);
    e->c = &e->store;
    e->npc = (s8)npc;
    e->hx = npc >= 0 ? npcs[npc].x : 0;
    e->hy = npc >= 0 ? npcs[npc].y : 0;
    e->side = (u8)side; e->mon = (u8)mon;
    e->engaged = -1; e->marked_by = -1;
    e->reacted = e->dodge = e->hidden = e->gbolt = e->mock = 0;
    e->xp = xp;
}

/* One battle's theme, set by events before encounter_run; survives the
 * wipe-retry (the rematch keeps its anthem), resets when the fight ends.
 * Stored +1 with 0 = unset so it stays zero-initialized .bss -- an
 * initializer here lands in .data and shifts G, whose address the
 * scenario fleet pokes (the stone-2 lesson, relearned in this file). */
static int enc_song;
void encounter_song(int s) { enc_song = s + 1; }

int encounter_run(const EncSpawn* es, int n, int helm_rounds, int surprise) {
    Game gsnap = G;
    R5Creature p5snap[3];
    for (int i = 0; i < 3; i++) p5snap[i] = party5[i];
    s16 enemy_home[MAXEC][2];
    for (int i = 0; i < n; i++) {
        enemy_home[i][0] = npcs[es[i].npc].x;
        enemy_home[i][1] = npcs[es[i].npc].y;
    }
    G_FIELD_IDLE = 0;
    r5_seed(&rng, rnd());

retry:
    for (int i = 0; i < nwarp; i++) field_remove_npc(warp_npc[i]);
    nwarp = 0;
    nec = 0;
    round_no = 1;
    rounds_left = helm_rounds;
    if (enc_song) mgba_logf("enc song=%d", enc_song - 1);
    music(enc_song ? enc_song - 1 : SONG_BATTLE);

    /* translucent combat UI: BG0|BG1 alpha-blend over field + sprites
     * (re-armed each retry; the wipe fade clears BLDCNT) */
    REG_BLDCNT = 0x3C43;
    REG_BLDALPHA = (u16)(13 | (7 << 8));

    /* stage popup/die-value digits after the obj tiles (shared with the field) */
    dice_stage_digits();

    /* restore spawn sprites first (retry may have marked them GONE) */
    for (int i = 0; i < n; i++) {
        npcs[es[i].npc].flags &= (u8)~NPC_GONE;
        npcs[es[i].npc].x = enemy_home[i][0];
        npcs[es[i].npc].y = enemy_home[i][1];
    }

    party5_refresh_all();     /* sync 5e sheets to PMember as combat begins */

    /* party materializes beside Tav */
    int px = field_player_x(), py = field_player_y();
    field_hide_player(1);
    static const s8 form[3][2] = { { 0, 0 }, { -20, 16 }, { 20, 16 } };
    for (int i = 0; i < G.nparty; i++) {
        MemberLook L = member_look(G.pm[i].face, G.pm[i].cls);
        party_npc[i] = field_add_npc(0, 0, L.objt, L.pal, 1, 0);
        npcs[party_npc[i]].x = (s16)px;
        npcs[party_npc[i]].y = (s16)py;
        add_pc(i, px + form[i][0], py + form[i][1], party_npc[i]);
    }
    /* walk-in */
    for (int t = 1; t <= 16; t++) {
        for (int i = 0; i < G.nparty; i++) {
            npcs[party_npc[i]].x = (s16)(px + form[i][0] * t / 16);
            npcs[party_npc[i]].y = (s16)(py + form[i][1] * t / 16);
        }
        pump(1);
    }
    for (int i = 0; i < G.nparty; i++) {
        ec[i].hx = npcs[party_npc[i]].x;
        ec[i].hy = npcs[party_npc[i]].y;
    }

    for (int i = 0; i < n; i++)
        add_mon(es[i].mon, es[i].npc, es[i].side, es[i].xp);
    frame_camera();

    crumb(CR_ENC, n);

    /* --- initiative, rolled in the open --- */
    bar_wait("Roll initiative!");
    for (int i = 0; i < nec; i++) {
        R5Dice d = r5_d20(&rng, 0);
        ec[i].init = (s16)(d.total + r5_mod(ec[i].c->ab[R5_DEX]));
        mgba_logf("init %s d20=%d dex=%d -> %d", ec[i].c->name, d.total,
                  r5_mod(ec[i].c->ab[R5_DEX]), ec[i].init);
        popup(ec_sx(&ec[i]) + 8, ec_sy(&ec[i]) - 4, ec[i].init, 9);
    }
    /* identity-fill ALL slots: monsters that warp in later (nec grows) act
     * at the end of the round via their own slot -- an uninitialized read
     * here once sent the cambion's turn through a garbage EC pointer. */
    int order[MAXEC];
    for (int i = 0; i < MAXEC; i++) order[i] = i;
    for (int i = 0; i < nec; i++)          /* stable-ish insertion sort */
        for (int j = i + 1; j < nec; j++)
            if (ec[order[j]].init > ec[order[i]].init) {
                int t = order[i]; order[i] = order[j]; order[j] = t;
            }

    status_draw();
    rnd_show();

    /* --- rounds --- */
    for (;;) {
        for (int oi = 0; oi < nec; oi++) {
            EC* a = &ec[order[oi]];
            g_act = a;
            crumb(CR_TURN, (oi << 8) | (u8)(a - ec));
            if (a->c->hp <= 0) continue;   /* any side: the fallen lie where
                                            * they fell, conditions or not */
            if (a->c->conds & C_UNCONSCIOUS) {
                if (a->side == 1) bar("Fast asleep...");
                continue;
            }
            if (round_no == 1 && surprise &&
                ((surprise == 1 && a->side == 1) ||
                 (surprise == 2 && a->side != 1))) {
                char sm[48]; char* sd = sm;
                sd = put_str(sd, a->c->name);
                sd = put_str(sd, " is surprised!");
                *sd = 0;
                bar(sm);
                pump(20);
                continue;                       /* 5e surprise: no round-1 turn */
            }
            a->dodge = 0;                   /* dodge lasts until your turn */
            a->reacted = 0;
            tray_clear();
            mgba_logf("turn r%d %s side%d hp=%d", round_no, a->c->name,
                      a->side, a->c->hp);
            {
                char m[48]; char* d = m;
                d = put_str(d, a->c->name);
                d = put_str(d, "'s turn");
                *d = 0;
                bar(m);
                pump(G_DEMO ? 10 : 20);
            }
            if (a->side == 0) { if (pc_turn(a)) goto connected; }
            else if (a->side == 1) enemy_turn(a);
            else ally_turn(a);

            if (!side_up(1)) goto victory;
            if (!party_conscious()) goto wipe;
        }
        round_no++;
        if (rounds_left > 0) {
            rounds_left--;
            rnd_show();
            if (rounds_left == 9 || rounds_left == 5) warp_cambion();
            if (rounds_left == 0) {
                bar_wait("Too late! We're falling!");
                goto wipe;
            }
        } else rnd_show();
    }

victory:
    {
        int xp = 0;
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 1) xp += ec[i].xp;
        music(SONG_VICTORY);
        bar_wait("Victory!");
        char m[48]; char* d = m;
        d = put_str(d, "Received "); d = put_num(d, xp); d = put_str(d, " XP!");
        *d = 0;
        bar_wait(m);
        char names[48];                /* one name per level-up this award */
        if (party_give_xp((u16)xp, names)) {
            party5_refresh_all();
            bar_wait("A new level!");
        }
        void level_up_choices(void);
        level_up_choices();            /* idempotent: fires pending subclass picks */
        mgba_logf("enc result=WIN xp=%d", xp);
    crumb(CR_RESULT, 1);
    }
    /* party folds back into Tav */
    for (int i = 0; i < G.nparty; i++) field_remove_npc(party_npc[i]);
    field_hide_player(0);
    field_cam_override(0, 0, 0);
    enc_garnish_clear();
    win_clear(0, 0, 30, 3);
    win_clear(26, 0, 4, 3);
    win_clear(0, 15, 30, 5);
    REG_BLDCNT = 0;
    G_FIELD_IDLE = 0;
    enc_song = 0;
    return ENC_WIN;

connected:
    mgba_log("enc result=CONNECTED");
    for (int i = 0; i < G.nparty; i++) field_remove_npc(party_npc[i]);
    field_hide_player(0);
    field_cam_override(0, 0, 0);
    enc_garnish_clear();
    win_clear(0, 0, 30, 3);
    win_clear(26, 0, 4, 3);
    win_clear(0, 15, 30, 5);
    REG_BLDCNT = 0;
    G_FIELD_IDLE = 0;
    enc_song = 0;
    return ENC_CONNECTED;

wipe:
    mgba_log("enc wipe -> retry");
    bar_wait("The party has fallen.");
    fade_out(20);
    G = gsnap;
    for (int i = 0; i < 3; i++) party5[i] = p5snap[i];
    for (int i = 0; i < G.nparty; i++) field_remove_npc(party_npc[i]);
    field_cam_override(0, 0, 0);
    enc_garnish_clear();
    win_clear(0, 0, 30, 3);
    win_clear(26, 0, 4, 3);
    win_clear(0, 15, 30, 5);
    field_draw();
    fade_in(16);
    goto retry;
}
