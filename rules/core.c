#include "rules.h"

/* ---------------------------------------------------------------- conditions */

uint16_t r5_conds_effective(uint16_t c) {
    if (c & C_UNCONSCIOUS) c |= C_PRONE;      /* an unconscious creature drops */
    if (c & (C_PARALYZED | C_PETRIFIED | C_STUNNED | C_UNCONSCIOUS))
        c |= C_INCAPACITATED;
    return c;
}

int r5_can_act(uint16_t conds) {
    return !(r5_conds_effective(conds) & C_INCAPACITATED);
}

int r5_prof(const R5Creature* c) {
    if (c->cls == R5C_MONSTER) return 2;      /* CR<=7 prologue foes */
    int l = c->level < 1 ? 1 : c->level > 3 ? 3 : c->level;
    return r5_classes[c->cls].prof_bonus[l];
}

/* ---------------------------------------------------------------- damage */

static int scale_damage(const R5Creature* t, int amount, uint8_t type) {
    uint16_t bit = (uint16_t)(1 << type);
    if (t->immune & bit) return 0;
    if (t->resist & bit) return amount / 2;
    if (t->vulnerable & bit) return amount * 2;
    return amount;
}

int r5_apply_damage(R5Creature* t, int amount, uint8_t type) {
    int dmg = scale_damage(t, amount, type);
    if (dmg <= 0) return 0;
    if (t->temp_hp > 0) {
        int absorbed = dmg < t->temp_hp ? dmg : t->temp_hp;
        t->temp_hp = (int16_t)(t->temp_hp - absorbed);
        dmg -= absorbed;
    }
    int lost = dmg < t->hp ? dmg : t->hp;
    t->hp = (int16_t)(t->hp - dmg);
    if (t->hp <= 0 && (t->traits & TR_RELENTLESS) &&
        !(t->traits & TR_USED_RELENTLESS) && t->cls != R5C_MONSTER) {
        t->traits |= TR_USED_RELENTLESS;      /* half-orc: stay at 1 hp */
        t->hp = 1;
        return lost - 1 > 0 ? lost - 1 : 0;
    }
    if (t->hp <= 0) {
        t->hp = 0;
        t->concentrating = 0;
        if (t->cls != R5C_MONSTER)
            t->conds |= C_UNCONSCIOUS;        /* no death saves: downed */
    }
    return lost;
}

void r5_heal(R5Creature* t, int amount) {
    if (amount <= 0) return;
    if (t->hp <= 0) t->conds &= (uint16_t)~C_UNCONSCIOUS;  /* back on your feet */
    t->hp = (int16_t)(t->hp + amount);
    if (t->hp > t->hpmax) t->hp = t->hpmax;
}

int r5_conc_dc(int damage) {
    int half = damage / 2;
    return half > 10 ? half : 10;
}

/* ---------------------------------------------------------------- attacks */

static int ability_for_weapon(const R5Creature* a, const R5Weapon* w) {
    int s = r5_mod(a->ab[R5_STR]), d = r5_mod(a->ab[R5_DEX]);
    if (w->props & WP_RANGED) return d;
    if (w->props & WP_FINESSE) return s > d ? s : d;
    return s;
}

/* shared to-hit resolution; returns 1 on hit and fills d20/bless/bonus/total */
static void resolve_to_hit(R5RNG* r, R5Attack* out, int to_hit_bonus,
                           int flags, uint8_t target_ac) {
    out->d20 = r5_d20(r, flags);
    out->bonus = (int16_t)to_hit_bonus;
    if (flags & R5F_BLESS) {
        out->bless = r5_roll(r, 1, 4, 0);
        out->bonus = (int16_t)(out->bonus + out->bless.total);
    } else {
        out->bless.n = 0;
        out->bless.total = 0;
    }
    out->target_ac = target_ac;
    out->total = (int16_t)(out->d20.total + out->bonus);
    out->crit = (out->d20.total == 20);
    out->fumble = (out->d20.total == 1);
    out->hit = out->crit || (!out->fumble && out->total >= target_ac);
    if (out->hit && (flags & R5F_AUTOCRIT)) out->crit = 1;
}

/* damage roll: crit doubles the DICE, never the modifier */
static R5Dice damage_roll(R5RNG* r, R5DiceSpec spec, int mod, int crit,
                          int extra_d6) {
    int n = spec.n + extra_d6;
    if (crit) n *= 2;
    /* all dice share a size only if extra dice match; mixed sizes (d8 weapon
     * + d6 mark) are rolled as the weapon size for record simplicity when
     * sizes differ we roll separately and merge totals */
    if (extra_d6 == 0 || spec.sides == 6)
        return r5_roll(r, n, spec.sides, spec.mod + mod);
    R5Dice w = r5_roll(r, crit ? spec.n * 2 : spec.n, spec.sides, spec.mod + mod);
    R5Dice x = r5_roll(r, crit ? extra_d6 * 2 : extra_d6, 6, 0);
    /* merge records (w first) */
    for (int i = 0; i < x.n && w.n < R5_MAX_DICE; i++)
        w.rolls[w.n++] = x.rolls[i];
    w.total = (int16_t)(w.total + x.total);
    return w;
}

R5Attack r5_weapon_attack(R5RNG* r, const R5Creature* a, const R5Creature* t,
                          const R5Weapon* w, int flags) {
    R5Attack out = { 0 };
    if (a->traits & TR_LUCKY) flags |= R5F_LUCKY;
    int abil = ability_for_weapon(a, w);
    resolve_to_hit(r, &out, abil + r5_prof(a), flags, t->ac);
    out.dmg_type = w->dmg_type;
    if (!out.hit) return out;

    R5DiceSpec spec = w->dmg;
    if ((flags & R5F_VERSATILE) && w->versatile.n) spec = w->versatile;
    if (out.crit && (a->traits & TR_SAVAGE)) spec.n++;   /* Savage Attacks */
    int extra = 0;
    if (flags & R5F_MARK) extra += 1;                       /* hunter's mark */
    if (flags & R5F_SNEAK) extra += r5_sneak_dice(a);
    int flat = 0;                                      /* rage: +2 STR melee */
    if ((a->conds & C_RAGING) && !(w->props & WP_RANGED) &&
        !((w->props & WP_FINESSE) && r5_mod(a->ab[R5_DEX]) > r5_mod(a->ab[R5_STR])))
        flat = 2;
    out.dmg = damage_roll(r, spec, abil + flat, out.crit, extra);
    out.damage = (int16_t)scale_damage(t, out.dmg.total, w->dmg_type);

    if (w->rider_dmg.n) {                                   /* magic weapon rider */
        R5DiceSpec rs = w->rider_dmg;
        if (out.crit) rs.n = (uint8_t)(rs.n * 2);
        out.rider_dmg = r5_roll(r, rs.n, rs.sides, rs.mod);
        int amt = out.rider_dmg.total;
        if (w->rider_dc) {
            R5Save sv = r5_save(r, t, w->rider_save_ab, w->rider_dc, 0);
            out.rider_save = sv.d20;
            out.rider_saved = sv.success;
            if (sv.success) amt /= 2;
        }
        out.rider_damage = (int16_t)scale_damage(t, amt, w->rider_type);
        out.rider_type = w->rider_type;
    }
    return out;
}

R5Attack r5_monster_attack(R5RNG* r, const R5Creature* a, const R5MAttack* ma,
                           const R5Creature* t, int flags) {
    (void)a;
    R5Attack out = { 0 };
    resolve_to_hit(r, &out, ma->to_hit, flags, t->ac);
    out.dmg_type = ma->dmg_type;
    if (!out.hit) return out;

    out.dmg = damage_roll(r, ma->dmg, 0, out.crit, 0);
    out.damage = (int16_t)scale_damage(t, out.dmg.total, ma->dmg_type);

    if (ma->rider_dmg.n) {
        R5DiceSpec rs = ma->rider_dmg;
        if (out.crit) rs.n = (uint8_t)(rs.n * 2);           /* crits double riders too */
        out.rider_dmg = r5_roll(r, rs.n, rs.sides, rs.mod);
        R5Save sv = r5_save(r, t, ma->rider_save_ab, ma->rider_dc, 0);
        out.rider_save = sv.d20;
        out.rider_saved = sv.success;
        int amt = sv.success ? out.rider_dmg.total / 2 : out.rider_dmg.total;
        out.rider_damage = (int16_t)scale_damage(t, amt, ma->rider_type);
        out.rider_type = ma->rider_type;
    }
    return out;
}

/* ------------------------------------------------------- expectations */

static int hit_pct_straight(int bonus, int ac) {
    int need = ac - bonus;               /* die face required */
    int count = need <= 2 ? 19 : need <= 20 ? 21 - need : 1;
    return count * 5;                    /* nat 1 misses, nat 20 hits */
}

int r5_hit_pct(int bonus, int ac, int advflags) {
    int p = hit_pct_straight(bonus, ac);
    int adv = (advflags & R5F_ADV) != 0, dis = (advflags & R5F_DIS) != 0;
    if (adv == dis) return p;
    if (adv) return 100 - (100 - p) * (100 - p) / 100;
    return p * p / 100;
}

int r5_crit_permille(int advflags) {
    int adv = (advflags & R5F_ADV) != 0, dis = (advflags & R5F_DIS) != 0;
    if (adv == dis) return 50;           /* 5%     */
    return adv ? 98 : 2;                 /* 9.75% / 0.25%, rounded */
}

/* expected damage x100 for one swing: P(hit)*E[dice+mod] + P(crit)*E[dice] */
int r5_ev_attack_x100(int bonus, int ac, int advflags, R5DiceSpec dmg) {
    int e4_dice = 2 * dmg.n * (dmg.sides + 1);         /* E[dice] x4  */
    int e4_full = e4_dice + 4 * dmg.mod;               /* + modifier  */
    if (e4_full < 0) e4_full = 0;
    int hit = r5_hit_pct(bonus, ac, advflags);
    int crit = r5_crit_permille(advflags);
    return hit * e4_full / 4 + crit * e4_dice / 40;
}

R5Save r5_save(R5RNG* r, const R5Creature* c, int ability, int dc, int flags) {
    R5Save s = { 0 };
    if (c->traits & TR_LUCKY) flags |= R5F_LUCKY;
    s.d20 = r5_d20(r, flags);
    int bonus = r5_mod(c->ab[ability]);
    if (c->save_prof & (1 << ability)) bonus += r5_prof(c);
    if (flags & R5F_BLESS) {
        s.bless = r5_roll(r, 1, 4, 0);
        bonus += s.bless.total;
    }
    s.dc = (int16_t)dc;
    s.total = (int16_t)(s.d20.total + bonus);
    s.success = s.total >= dc;
    return s;
}
