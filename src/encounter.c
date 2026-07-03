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

#define ME(t, h, v, p) ((u16)((t) | ((h) << 10) | ((v) << 11) | ((p) << 12)))
#define TXT ((vu16*)SCREENBLOCK(30))

#define MAXEC 8
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
    u16 xp;
} EC;

static EC ec[MAXEC];
static int nec, round_no;
static R5RNG rng;
static int party_npc[3];         /* temp npcs for the party sprites */

/* ------------------------------------------------------------------ ui */

static void rnd_show(void) {
    win_draw(26, 0, 4, 3);
    char b[3] = { ' ', ' ', 0 };
    if (round_no >= 10) { b[0] = (char)('0' + round_no / 10); b[1] = (char)('0' + round_no % 10); }
    else b[1] = (char)('0' + round_no);
    txt_put(27, 1, b, 0);
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
    char m[26]; char* d = m;
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
    char m[26]; char* d = m;
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

static void pump(int n) {
    while (n--) {
        frame();
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
    char m[26]; char* d = m;
    d = put_str(d, r5_spells[spell].name);
    d = put_str(d, why);
    *d = 0;
    bar_wait(m);
}

/* apply damage + all bookkeeping (wake sleepers, concentration checks, death) */
static void deal(EC* t, int amount, u8 type, int scr_pop) {
    int was_sleeping = (t->c->hp > 0) && (t->c->conds & C_UNCONSCIOUS);
    if (t->c->immune & (u16)(1 << type)) {
        bar_wait("IMMUNE!");
        return;
    }
    if (t->c->resist & (u16)(1 << type)) bar("Resisted...");
    int lost = r5_apply_damage(t->c, amount, type);
    if (scr_pop) popup(ec_sx(t) + 8, ec_sy(t), lost, 10);
    if (was_sleeping && t->c->hp > 0) {
        t->c->conds &= (u16)~C_UNCONSCIOUS;
        bar_wait("It jolts awake!");
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
    if (old->side == 0) {
        at = r5_weapon_attack(&rng, old->c, a->c,
                              &r5_weapons[party5_weapon(old->pi)], 0);
        bar_attack("OA:", &at);
    } else {
        const R5MAttack* ma = &r5_monsters[old->mon].attacks[0];
        at = r5_monster_attack(&rng, old->c, ma, a->c, 0);
        bar_attack("OA:", &at);
    }
    if (at.hit) { hit_react(a); deal(a, at.damage, at.dmg_type, 1); bar_damage(&at); }
    a->engaged = -1;
}

static const R5MAttack* ai_ma;   /* AI-chosen monster attack for next strike */

/* full melee/ranged weapon strike */
static void strike(EC* a, EC* t) {
    const R5Weapon* w = a->side == 0 ? &r5_weapons[party5_weapon(a->pi)] : 0;
    const R5MAttack* mona = 0;
    if (a->side != 0) {
        mona = ai_ma ? ai_ma : &r5_monsters[a->mon].attacks[0];
        ai_ma = 0;
    }
    int melee = a->side == 0 ? !(w->props & WP_RANGED) : !mona->ranged;
    if (melee) {
        maybe_opportunity(a, t);
        if (a->c->hp <= 0) return;          /* OA dropped us */
        dash_to(a, t, 18);
    } else ec_face_toward(a, t);

    int f = atk_flags(a, t, melee);
    R5Attack at;
    if (a->side == 0) {
        at = r5_weapon_attack(&rng, a->c, t->c, w, f);
        bar_attack(w->name, &at);
        if (f & R5F_SNEAK && at.hit) bar_wait("Sneak attack!");
    } else {
        at = r5_monster_attack(&rng, a->c, mona, t->c, f);
        bar_attack(mona->name, &at);
    }
    mgba_logf("atk %s->%s d20=%d tot=%d v%d %s dmg=%d rider=%d",
              a->c->name, t->c->name, at.d20.total, at.total, at.target_ac,
              at.crit ? "CRIT" : at.hit ? "hit" : "miss",
              at.damage, at.rider_damage);
    if (at.hit) {
        hit_react(t);
        deal(t, at.damage, at.dmg_type, 1);
        bar_damage(&at);
        if (at.rider_dmg.n && t->c->hp > 0 && at.rider_damage > 0)
            deal(t, at.rider_damage, at.rider_type, 1);
        if (melee && t->c->hp > 0) { a->engaged = (s8)(t - ec); t->engaged = (s8)(a - ec); }
    }
    post_attack(a, t, &at);
    if (melee) dash_home(a);
}

/* ------------------------------------------------------------------ spells */

static void cast_attack_spell(EC* a, EC* t, int sp) {
    const R5Spell* s = &r5_spells[sp];
    R5MAttack ma = { s->name, (s8)party5_spell_atk(a->c), s->dice,
                     s->dmg_type, { 0, 0, 0 }, 0, 0, 0, 1 };
    ec_face_toward(a, t);
    int f = atk_flags(a, t, 0);
    R5Attack at = r5_monster_attack(&rng, a->c, &ma, t->c, f);
    bar_attack(s->name, &at);
    mgba_logf("spell %s d20=%d tot=%d v%d %s dmg=%d", s->name, at.d20.total,
              at.total, at.target_ac, at.hit ? "hit" : "miss", at.damage);
    if (at.hit) {
        hit_react(t);
        deal(t, at.damage, at.dmg_type, 1);
        bar_damage(&at);
        if (sp == R5S_GUIDING_BOLT) { t->gbolt = 1; bar_wait("Light clings to it!"); }
    }
    post_attack(a, t, &at);
}

static void cast_save_spell(EC* a, EC* t, int sp) {
    const R5Spell* s = &r5_spells[sp];
    int dc = party5_spell_dc(a->c);
    ec_face_toward(a, t);
    R5Save sv = r5_save(&rng, t->c, s->save_ab, dc, 0);
    char m[26]; char* d = m;
    d = put_str(d, "save "); d = put_num(d, sv.d20.total);
    d = put_str(d, " v DC "); d = put_num(d, sv.dc);
    d = put_str(d, sv.success ? " ok" : " FAIL");
    *d = 0;
    bar_wait(m);
    mgba_logf("save-spell %s d20=%d tot=%d dc=%d %s", s->name, sv.d20.total,
              sv.total, sv.dc, sv.success ? "save" : "fail");
    int dmg = r5_roll(&rng, s->dice.n, s->dice.sides, s->dice.mod).total;
    if (sv.success) dmg = s->save_half ? dmg / 2 : 0;
    if (dmg > 0) {
        hit_react(t);
        deal(t, dmg, s->dmg_type, 1);
    }
    if (!sv.success && sp == R5S_VICIOUS_MOCKERY) {
        t->mock = 1;
        bar_wait("It reels, stung!");
    }
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
        bar("Magic Missile!");
        hit_react(t);
        deal(t, d.total, DT_FORCE, 1);
        mgba_logf("missile %d -> %s", d.total, t->c->name);
    }
}

static void cast_sleep(EC* a) {
    (void)a;  /* area spell: no single target */
    R5Dice pool = r5_roll(&rng, 5, 8, 0);
    mgba_logf("sleep pool=%d", pool.total);
    char m[26]; char* d = m;
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
        char b[26]; char* e = b;
        e = put_str(e, t->c->name); e = put_str(e, " falls asleep!"); *e = 0;
        bar_wait(b);
    }
}

static void cast_heal(EC* a, EC* t, int sp) {
    const R5Spell* s = &r5_spells[sp];
    int mod = s->add_mod ? r5_mod(a->c->ab[party5_cast_ab(a->c->cls)]) : 0;
    R5Dice d = r5_roll(&rng, s->dice.n, s->dice.sides, s->dice.mod + mod);
    sfx_play(SFX_HEAL);
    int wake = t->c->hp <= 0;
    r5_heal(t->c, d.total);
    popup(ec_sx(t) + 8, ec_sy(t), d.total, 8);
    if (wake) bar_wait("Back on their feet!");
    mgba_logf("heal %s +%d -> %d", t->c->name, d.total, t->c->hp);
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

static void enemy_turn(EC* a) {
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
        pump(1);
        u16 k = key_hit();
        if (k & (KEY_UP | KEY_LEFT)) { sel = (sel + n - 1) % n; sfx_play(SFX_CURSOR); }
        if (k & (KEY_DOWN | KEY_RIGHT)) { sel = (sel + 1) % n; sfx_play(SFX_CURSOR); }
        if (k & KEY_A) { sfx_play(SFX_CONFIRM); obj_hide(OBJ_CURSOR); return t; }
        if (k & KEY_B) { sfx_play(SFX_CANCEL); obj_hide(OBJ_CURSOR); return 0; }
    }
}

/* demo auto-play so headless runs exercise the full economy */
static void demo_pc_turn(EC* a) {
    R5Creature* c = a->c;
    int cls = G.pm[a->pi].cls;
    EC* foe = nearest_foe(a, 1);
    /* bonus action first where it matters */
    if (cls == CLS_FIGHTER && r5_can_second_wind(c) && c->hp * 2 < c->hpmax) {
        R5Dice d = r5_second_wind(&rng, c);
        bar_wait("Second Wind!");
        popup(ec_sx(a) + 8, ec_sy(a), d.total, 8);
    }
    if (cls == CLS_ROGUE && !a->hidden && foe) {
        a->hidden = 1;
        bar_wait("Hides in the chaos!");
    }
    if (cls == CLS_RANGER && foe && !c->concentrating && c->slots[0] &&
        r5_spend_slot(c, 1)) {
        c->concentrating = R5S_HUNTERS_MARK + 1;
        foe->marked_by = (s8)(a - ec);
        bar_wait("Hunter's Mark!");
    }
    /* heal a downed friend before attacking */
    if ((cls == CLS_BARD || cls == CLS_CLERIC)) {
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 0 && ec[i].c->hp <= 0 && c->slots[0] &&
                r5_spend_slot(c, 1)) {
                cast_heal(a, &ec[i], cls == CLS_BARD ? R5S_HEALING_WORD : R5S_CURE_WOUNDS);
                if (cls == CLS_CLERIC) return;   /* cure wounds was the action */
                break;
            }
    }
    if (!foe) return;
    switch (cls) {
        case CLS_WIZARD: {
            int fire_ok = !(foe->c->immune & (1 << DT_FIRE));
            if (side_up(1) >= 3 && c->slots[0] && r5_spend_slot(c, 1)) cast_sleep(a);
            else if (!fire_ok && c->slots[0] && r5_spend_slot(c, 1)) cast_missiles(a);
            else if (fire_ok) cast_attack_spell(a, foe, R5S_FIRE_BOLT);
            else strike(a, foe);
            break;
        }
        case CLS_BARD: cast_save_spell(a, foe, R5S_VICIOUS_MOCKERY); break;
        case CLS_CLERIC:
            if (c->slots[0] && r5_spend_slot(c, 1)) cast_attack_spell(a, foe, R5S_GUIDING_BOLT);
            else strike(a, foe);
            break;
        default: strike(a, foe); break;
    }
    if (cls == CLS_FIGHTER && r5_can_action_surge(c) && foe->c->hp > 0) {
        r5_use_action_surge(c);
        bar_wait("Action Surge!");
        strike(a, foe);
    }
}

static void pc_turn(EC* a) {
    if (G_DEMO && !G_MANUAL_BAT) { demo_pc_turn(a); return; }

    R5Creature* c = a->c;
    int cls = G.pm[a->pi].cls;
    int action = 0, bonus = 0;
    for (;;) {
        const char* items[8]; u8 code[8]; int n = 0;
        if (!action) {
            items[n] = "Attack"; code[n++] = 0;
            if (cls == CLS_BARD) { items[n] = "V.Mockery"; code[n++] = 10 + R5S_VICIOUS_MOCKERY; }
            if (cls == CLS_WIZARD) {
                items[n] = "Fire Bolt"; code[n++] = 10 + R5S_FIRE_BOLT;
                if (c->slots[0]) { items[n] = "M.Missile"; code[n++] = 10 + R5S_MAGIC_MISSILE; }
                if (c->slots[0]) { items[n] = "Sleep"; code[n++] = 10 + R5S_SLEEP; }
            }
            if (cls == CLS_CLERIC && c->slots[0]) {
                items[n] = "Guid.Bolt"; code[n++] = 10 + R5S_GUIDING_BOLT;
                items[n] = "Cure Wnds"; code[n++] = 10 + R5S_CURE_WOUNDS;
                if (!c->concentrating) { items[n] = "Bless"; code[n++] = 10 + R5S_BLESS; }
            }
            items[n] = "Item"; code[n++] = 1;
            items[n] = "Dodge"; code[n++] = 2;
            if (a->engaged >= 0) { items[n] = "Disengage"; code[n++] = 3; }
        }
        if (!bonus) {
            if (cls == CLS_FIGHTER && r5_can_second_wind(c)) { items[n] = "2nd Wind"; code[n++] = 4; }
            if (cls == CLS_ROGUE && !a->hidden) { items[n] = "Hide"; code[n++] = 5; }
            if (cls == CLS_BARD && c->slots[0]) { items[n] = "Heal.Word"; code[n++] = 10 + R5S_HEALING_WORD; }
            if (cls == CLS_RANGER && c->slots[0] && !c->concentrating) { items[n] = "Hunt.Mark"; code[n++] = 6; }
        }
        items[n] = "End Turn"; code[n++] = 7;

        int sel = menu5(items, n, 0);
        u8 cd = code[sel];
        if (cd == 7) break;
        switch (cd) {
            case 0: {
                EC* t = pick5(1, 0);
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
            case 3:
                a->engaged = -1;
                bar_wait("Disengages cleanly.");
                action = 1;
                break;
            case 4: {
                R5Dice d = r5_second_wind(&rng, c);
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
            default: {
                int sp = cd - 10;
                const R5Spell* s = &r5_spells[sp];
                if (s->level > 0 && !r5_spend_slot(c, s->level)) { bar_wait("No slots!"); break; }
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

static void add_mon(int mon, int npc, int side, u16 xp) {
    EC* e = &ec[nec++];
    const R5Monster* m = &r5_monsters[mon];
    R5Creature* c = &e->store;
    c->name = m->name;
    for (int i = 0; i < 6; i++) c->ab[i] = m->ab[i];
    c->hp = c->hpmax = m->hp;
    c->temp_hp = 0;
    c->ac = m->ac;
    c->level = 1; c->cls = R5C_MONSTER;
    c->save_prof = 0;
    c->conds = 0;
    c->resist = m->resist; c->immune = m->immune; c->vulnerable = 0;
    c->slots[0] = c->slots[1] = c->slots[2] = 0;
    c->used = 0; c->concentrating = 0;
    e->c = c;
    e->npc = (s8)npc;
    e->hx = npc >= 0 ? npcs[npc].x : 0;
    e->hy = npc >= 0 ? npcs[npc].y : 0;
    e->side = (u8)side; e->mon = (u8)mon;
    e->engaged = -1; e->marked_by = -1;
    e->reacted = e->dodge = e->hidden = e->gbolt = e->mock = 0;
    e->xp = xp;
}

int encounter_run(const EncSpawn* es, int n, int flags) {
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
    nec = 0;
    round_no = 1;
    music(SONG_BATTLE);

    /* stage popup digits after the obj tiles */
    memcpy16((vu16*)((u32)OBJ_TILES + OBJ_TILE_COUNT * 32),
             &ui_tiles[('0' - 32) * 16], 10 * 16);

    /* party materializes beside Tav */
    int px = field_player_x(), py = field_player_y();
    field_hide_player(1);
    static const s8 form[3][2] = { { 0, 0 }, { -20, 16 }, { 20, 16 } };
    static const u16 pobj[6] = { OBJT_HERO, OBJT_HERO, OBJT_HERO, OBJT_HERO,
                                 OBJT_LAEZEL, OBJT_SHADOW };
    static const u8 ppal[6] = { 0, 0, 0, 0, 1, 2 };
    for (int i = 0; i < G.nparty; i++) {
        int cls = G.pm[i].cls;
        party_npc[i] = field_add_npc(0, 0, pobj[cls], ppal[cls], 1, 0);
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

    for (int i = 0; i < n; i++) {
        npcs[es[i].npc].flags &= (u8)~NPC_GONE;
        npcs[es[i].npc].x = enemy_home[i][0];
        npcs[es[i].npc].y = enemy_home[i][1];
        add_mon(es[i].mon, es[i].npc, 1, es[i].xp);
    }
    (void)flags;

    /* --- initiative, rolled in the open --- */
    bar_wait("Roll initiative!");
    for (int i = 0; i < nec; i++) {
        R5Dice d = r5_d20(&rng, 0);
        ec[i].init = (s16)(d.total + r5_mod(ec[i].c->ab[R5_DEX]));
        mgba_logf("init %s d20=%d dex=%d -> %d", ec[i].c->name, d.total,
                  r5_mod(ec[i].c->ab[R5_DEX]), ec[i].init);
        popup(ec_sx(&ec[i]) + 8, ec_sy(&ec[i]) - 4, ec[i].init, 9);
    }
    int order[MAXEC];
    for (int i = 0; i < nec; i++) order[i] = i;
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
            if (a->c->hp <= 0 && a->side != 0) continue;
            if (a->c->conds & C_UNCONSCIOUS) {
                if (a->side == 1 && a->c->hp > 0) bar("Fast asleep...");
                continue;
            }
            a->dodge = 0;                   /* dodge lasts until your turn */
            a->reacted = 0;
            {
                char m[26]; char* d = m;
                d = put_str(d, a->c->name);
                d = put_str(d, "'s turn");
                *d = 0;
                bar(m);
                pump(G_DEMO ? 10 : 20);
            }
            if (a->side == 0) pc_turn(a);
            else if (a->side == 1) enemy_turn(a);
            else ally_turn(a);

            if (!side_up(1)) goto victory;
            if (!party_conscious()) goto wipe;
        }
        round_no++;
        rnd_show();
    }

victory:
    {
        int xp = 0;
        for (int i = 0; i < nec; i++)
            if (ec[i].side == 1) xp += ec[i].xp;
        music(SONG_VICTORY);
        bar_wait("Victory!");
        char m[26]; char* d = m;
        d = put_str(d, "Received "); d = put_num(d, xp); d = put_str(d, " XP!");
        *d = 0;
        bar_wait(m);
        char names[32];
        if (party_give_xp((u16)xp, names)) {
            party5_refresh_all();
            bar_wait("A new level!");
        }
        mgba_logf("enc result=WIN xp=%d", xp);
    }
    /* party folds back into Tav */
    for (int i = 0; i < G.nparty; i++) field_remove_npc(party_npc[i]);
    field_hide_player(0);
    win_clear(0, 0, 30, 3);
    win_clear(26, 0, 4, 3);
    win_clear(0, 15, 30, 5);
    G_FIELD_IDLE = 0;
    return ENC_WIN;

wipe:
    bar_wait("The party has fallen.");
    fade_out(20);
    G = gsnap;
    for (int i = 0; i < 3; i++) party5[i] = p5snap[i];
    for (int i = 0; i < G.nparty; i++) field_remove_npc(party_npc[i]);
    win_clear(0, 0, 30, 3);
    win_clear(26, 0, 4, 3);
    win_clear(0, 15, 30, 5);
    field_draw();
    fade_in(16);
    goto retry;
}
