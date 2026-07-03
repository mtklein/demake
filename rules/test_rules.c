/* Native test suite for the 5e rules core: `make test-rules`.
 * Deterministic seeds; distribution checks; invariants in the spirit of
 * 5e-quint's safety-invariant list (see tools/srd/INVARIANTS.md). */
#include "rules.h"
#include <stdio.h>

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECKI(c, ...) do { checks++; if (!(c)) { fails++; \
    printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); \
    printf("\n"); } } while (0)

static R5Creature pc(int cls, int level, int str, int dex, int con) {
    R5Creature c = { 0 };
    c.name = "PC";
    c.ab[R5_STR] = (int8_t)str; c.ab[R5_DEX] = (int8_t)dex;
    c.ab[R5_CON] = (int8_t)con;
    c.ab[R5_INT] = 10; c.ab[R5_WIS] = 10; c.ab[R5_CHA] = 10;
    c.hpmax = c.hp = 20;
    c.ac = 14;
    c.level = (uint8_t)level;
    c.cls = (uint8_t)cls;
    c.save_prof = r5_classes[cls].save_prof;
    return c;
}

static R5Creature monster(int ac, int hp) {
    R5Creature c = { 0 };
    c.name = "Mon";
    for (int i = 0; i < 6; i++) c.ab[i] = 10;
    c.hpmax = c.hp = (int16_t)hp;
    c.ac = (uint8_t)ac;
    c.cls = R5C_MONSTER;
    return c;
}

/* ------------------------------------------------------------ dice */

static void test_dice(void) {
    R5RNG r; r5_seed(&r, 1);
    for (int i = 0; i < 5000; i++) {
        int d = r5_die(&r, 20);
        CHECK(d >= 1 && d <= 20);
    }
    for (int i = 0; i < 2000; i++) {
        R5Dice d = r5_roll(&r, 2, 6, 3);
        CHECK(d.n == 2 && d.sides == 6 && d.mod == 3);
        int sum = 0;
        for (int j = 0; j < d.n; j++) {
            CHECK(d.rolls[j] >= 1 && d.rolls[j] <= 6);
            sum += d.rolls[j];
        }
        CHECK(d.total == sum + 3);
        CHECK(d.total >= 5 && d.total <= 15);
    }
    /* uniformity: mean of d20 over 100k within 10.5 +/- 0.15 */
    long sum = 0;
    for (int i = 0; i < 100000; i++) sum += r5_die(&r, 20);
    double mean = sum / 100000.0;
    CHECKI(mean > 10.35 && mean < 10.65, "d20 mean %.3f", mean);
}

static void test_advantage(void) {
    R5RNG r; r5_seed(&r, 2);
    long sn = 0, sa = 0, sd = 0;
    const int N = 40000;
    for (int i = 0; i < N; i++) {
        R5Dice n = r5_d20(&r, 0);
        R5Dice a = r5_d20(&r, R5F_ADV);
        R5Dice d = r5_d20(&r, R5F_DIS);
        R5Dice x = r5_d20(&r, R5F_ADV | R5F_DIS);
        CHECK(n.n == 1);
        CHECK(x.n == 1);                       /* adv+dis cancel: straight roll */
        CHECK(a.n == 2 && d.n == 2);
        int hi = a.rolls[0] > a.rolls[1] ? a.rolls[0] : a.rolls[1];
        int lo = d.rolls[0] < d.rolls[1] ? d.rolls[0] : d.rolls[1];
        CHECK(a.total == hi);                  /* advantage takes the higher */
        CHECK(d.total == lo);                  /* disadvantage the lower */
        sn += n.total; sa += a.total; sd += d.total;
    }
    double mn = sn / (double)N, ma = sa / (double)N, md = sd / (double)N;
    CHECKI(ma > mn && mn > md, "adv %.2f > norm %.2f > dis %.2f", ma, mn, md);
    CHECKI(ma > 13.5 && ma < 14.2, "adv mean %.2f", ma);   /* ~13.82 */
    CHECKI(md > 6.8 && md < 7.5, "dis mean %.2f", md);     /* ~7.18  */
}

static void test_mods(void) {
    CHECK(r5_mod(1) == -5);
    CHECK(r5_mod(7) == -2);
    CHECK(r5_mod(8) == -1);
    CHECK(r5_mod(10) == 0);
    CHECK(r5_mod(11) == 0);
    CHECK(r5_mod(15) == 2);
    CHECK(r5_mod(17) == 3);
    CHECK(r5_mod(20) == 5);
}

/* ------------------------------------------------------------ attacks */

static void test_attack_math(void) {
    R5RNG r; r5_seed(&r, 3);
    R5Creature a = pc(R5C_FIGHTER, 1, 16, 10, 14);   /* +3 STR, +2 prof */
    R5Creature t = monster(15, 50);
    int saw_crit = 0, saw_fumble = 0, saw_hit = 0, saw_miss = 0;
    for (int i = 0; i < 4000; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_LONGSWORD], 0);
        CHECK(at.total == at.d20.total + at.bonus);
        CHECK(at.bonus == 5);                          /* STR +3, prof +2 */
        if (at.d20.total == 20) { CHECK(at.crit && at.hit); saw_crit = 1; }
        if (at.d20.total == 1) { CHECK(at.fumble && !at.hit); saw_fumble = 1; }
        if (at.hit && !at.crit) CHECK(at.total >= 15);
        if (!at.hit && !at.fumble) CHECK(at.total < 15);
        if (at.hit) {
            saw_hit = 1;
            /* longsword one-handed: 1d8+3 (or crit 2d8+3) */
            CHECK(at.dmg.sides == 8);
            CHECK(at.dmg.n == (at.crit ? 2 : 1));
            CHECK(at.dmg.total >= at.dmg.n * 1 + 3 &&
                  at.dmg.total <= at.dmg.n * 8 + 3);
            CHECK(at.damage == at.dmg.total);          /* no resist on target */
        } else saw_miss = 1;
    }
    CHECK(saw_crit && saw_fumble && saw_hit && saw_miss);
}

static void test_finesse_and_ranged(void) {
    R5RNG r; r5_seed(&r, 4);
    R5Creature a = pc(R5C_ROGUE, 1, 8, 16, 10);       /* STR -1, DEX +3 */
    R5Creature t = monster(10, 50);
    R5Attack d = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_DAGGER], 0);
    CHECK(d.bonus == 5);                               /* finesse: DEX+prof */
    R5Attack l = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_LONGSWORD], 0);
    CHECK(l.bonus == 1);                               /* STR -1 + prof 2 */
    R5Attack b = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_SHORTBOW], 0);
    CHECK(b.bonus == 5);                               /* ranged: DEX */
}

static void test_versatile(void) {
    R5RNG r; r5_seed(&r, 5);
    R5Creature a = pc(R5C_FIGHTER, 1, 16, 10, 14);
    R5Creature t = monster(1, 50);                     /* always hit */
    for (int i = 0; i < 200; i++) {
        R5Attack one = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_LONGSWORD], 0);
        if (one.hit) CHECK(one.dmg.sides == 8);
        R5Attack two = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_LONGSWORD],
                                        R5F_VERSATILE);
        if (two.hit) CHECK(two.dmg.sides == 10);
    }
}

static void test_crit_doubles_dice_not_mod(void) {
    R5RNG r; r5_seed(&r, 6);
    R5Creature a = pc(R5C_FIGHTER, 1, 16, 10, 14);
    R5Creature t = monster(1, 50);
    int crits = 0;
    for (int i = 0; i < 4000 && crits < 20; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_GREATSWORD], 0);
        if (!at.crit) continue;
        crits++;
        CHECK(at.dmg.n == 4);                          /* 2d6 -> 4d6 */
        CHECK(at.dmg.total >= 4 + 3 && at.dmg.total <= 24 + 3);
    }
    CHECK(crits == 20);
}

static void test_hunters_mark_and_sneak(void) {
    R5RNG r; r5_seed(&r, 7);
    R5Creature ra = pc(R5C_RANGER, 1, 10, 16, 12);
    R5Creature ro = pc(R5C_ROGUE, 3, 8, 16, 10);
    R5Creature t = monster(1, 200);
    for (int i = 0; i < 300; i++) {
        R5Attack m = r5_weapon_attack(&r, &ra, &t, &r5_weapons[R5W_LONGBOW], R5F_MARK);
        if (m.hit && !m.crit) CHECK(m.dmg.n == 2);     /* 1d8 + 1d6 mark */
        R5Attack s = r5_weapon_attack(&r, &ro, &t, &r5_weapons[R5W_DAGGER], R5F_SNEAK);
        if (s.hit && !s.crit) CHECK(s.dmg.n == 3);     /* 1d4 + 2d6 sneak @L3 */
    }
}

static void test_monster_attack_rider(void) {
    R5RNG r; r5_seed(&r, 8);
    const R5MAttack* sting = &r5_monsters[R5M_IMP].attacks[0];
    CHECK(sting->to_hit == 5);
    R5Creature t = pc(R5C_WIZARD, 1, 8, 14, 12);
    t.hpmax = t.hp = 1000;
    int hits = 0, saves = 0, fails2 = 0;
    for (int i = 0; i < 2000; i++) {
        R5Attack at = r5_monster_attack(&r, 0, sting, &t, 0);
        if (!at.hit) continue;
        hits++;
        CHECK(at.rider_dmg.n >= 3);                    /* 3d6, 6d6 on crit */
        if (at.rider_saved) {
            saves++;
            CHECK(at.rider_damage == at.rider_dmg.total / 2);
        } else {
            fails2++;
            CHECK(at.rider_damage == at.rider_dmg.total);
        }
        if (at.crit) CHECK(at.rider_dmg.n == 6);
    }
    CHECK(hits > 500 && saves > 50 && fails2 > 50);
}

/* ------------------------------------------------------------ damage */

static void test_resist_immune_vuln(void) {
    R5Creature t = monster(10, 100);
    t.resist = (uint16_t)(1 << DT_FIRE);
    t.immune = (uint16_t)(1 << DT_POISON);
    t.vulnerable = (uint16_t)(1 << DT_RADIANT);
    CHECK(r5_apply_damage(&t, 9, DT_FIRE) == 4);       /* halved, floored */
    CHECK(r5_apply_damage(&t, 50, DT_POISON) == 0);
    CHECK(r5_apply_damage(&t, 10, DT_RADIANT) == 20);
    CHECK(t.hp == 100 - 4 - 20);
}

static void test_temp_hp(void) {
    R5Creature t = monster(10, 50);
    t.temp_hp = 5;
    int lost = r5_apply_damage(&t, 8, DT_SLASHING);
    CHECK(t.temp_hp == 0);
    CHECK(lost == 3 && t.hp == 47);                    /* temp absorbs first */
}

static void test_downed_and_healing(void) {
    R5Creature p = pc(R5C_BARD, 1, 10, 14, 12);
    p.hp = 5;
    p.concentrating = 3;
    r5_apply_damage(&p, 12, DT_SLASHING);
    CHECK(p.hp == 0);
    CHECK(p.conds & C_UNCONSCIOUS);
    CHECK(p.concentrating == 0);                       /* conc breaks at 0 */
    CHECK(!r5_can_act(p.conds));                       /* implied incapacitated */
    r5_heal(&p, 7);
    CHECK(p.hp == 7);
    CHECK(!(p.conds & C_UNCONSCIOUS));                 /* healing wakes */
    CHECK(r5_can_act(p.conds));

    R5Creature m = monster(10, 10);
    r5_apply_damage(&m, 99, DT_FIRE);
    CHECK(m.hp == 0 && !(m.conds & C_UNCONSCIOUS));    /* monsters just die */

    r5_heal(&p, 9999);
    CHECK(p.hp == p.hpmax);                            /* clamped */
}

static void test_conditions_and_conc(void) {
    CHECK(r5_conds_effective(C_PARALYZED) & C_INCAPACITATED);
    CHECK(r5_conds_effective(C_STUNNED) & C_INCAPACITATED);
    CHECK(r5_conds_effective(C_UNCONSCIOUS) & C_INCAPACITATED);
    CHECK(r5_conds_effective(C_UNCONSCIOUS) & C_PRONE);   /* falls prone */
    CHECK(r5_conds_effective(C_PETRIFIED) & C_INCAPACITATED);
    CHECK(!(r5_conds_effective(C_PRONE) & C_INCAPACITATED));
    CHECK(r5_conc_dc(22) == 11);
    CHECK(r5_conc_dc(7) == 10);
    CHECK(r5_conc_dc(20) == 10);
    CHECK(r5_conc_dc(21) == 10);
    CHECK(r5_conc_dc(23) == 11);
}

/* ------------------------------------------------------------ features */

static void test_second_wind_and_surge(void) {
    R5RNG r; r5_seed(&r, 9);
    R5Creature f1 = pc(R5C_FIGHTER, 1, 16, 10, 14);
    R5Creature f2 = pc(R5C_FIGHTER, 2, 16, 10, 14);
    R5Creature w = pc(R5C_WIZARD, 3, 8, 14, 12);

    CHECK(r5_can_second_wind(&f1));                    /* Second Wind at L1 */
    CHECK(!r5_can_second_wind(&w));
    f1.hp = 5;
    R5Dice d = r5_second_wind(&r, &f1);
    CHECK(d.total >= 2 && d.total <= 11);              /* 1d10 + level 1 */
    CHECK(f1.hp == 5 + d.total);
    CHECK(!r5_can_second_wind(&f1));                   /* once per rest */
    r5_short_rest(&f1);
    CHECK(r5_can_second_wind(&f1));

    CHECK(!r5_can_action_surge(&f1));                  /* NOT at level 1 */
    CHECK(r5_can_action_surge(&f2));                   /* arrives at L2 */
    r5_use_action_surge(&f2);
    CHECK(!r5_can_action_surge(&f2));
}

static void test_sneak_progression(void) {
    R5Creature r1 = pc(R5C_ROGUE, 1, 8, 16, 10);
    R5Creature r2 = pc(R5C_ROGUE, 2, 8, 16, 10);
    R5Creature r3 = pc(R5C_ROGUE, 3, 8, 16, 10);
    R5Creature f = pc(R5C_FIGHTER, 3, 16, 10, 14);
    CHECK(r5_sneak_dice(&r1) == 1);
    CHECK(r5_sneak_dice(&r2) == 1);
    CHECK(r5_sneak_dice(&r3) == 2);
    CHECK(r5_sneak_dice(&f) == 0);
}

static void test_slots(void) {
    R5Creature w = pc(R5C_WIZARD, 1, 8, 14, 12);
    w.slots[0] = r5_classes[R5C_WIZARD].slots[1][0];
    CHECK(w.slots[0] == 2);                            /* wizard L1: 2 slots */
    CHECK(r5_spend_slot(&w, 1));
    CHECK(r5_spend_slot(&w, 1));
    CHECK(!r5_spend_slot(&w, 1));                      /* empty */
    CHECK(!r5_spend_slot(&w, 2));
    CHECK(r5_classes[R5C_RANGER].slots[1][0] == 0);    /* ranger: none at L1 */
    CHECK(r5_classes[R5C_RANGER].slots[2][0] == 2);    /* two at L2 */
}

static void test_save_proficiency(void) {
    R5RNG r; r5_seed(&r, 10);
    R5Creature f = pc(R5C_FIGHTER, 1, 16, 10, 14);     /* saves: STR, CON */
    long strtot = 0, dextot = 0;
    const int N = 4000;
    for (int i = 0; i < N; i++) {
        strtot += r5_save(&r, &f, R5_STR, 10, 0).total;
        dextot += r5_save(&r, &f, R5_DEX, 10, 0).total;
    }
    /* STR: d20 +3 mod +2 prof; DEX: d20 +0 — means differ by ~5 */
    double diff = (strtot - dextot) / (double)N;
    CHECKI(diff > 4.5 && diff < 5.5, "save prof diff %.2f", diff);
}

static void test_bless(void) {
    R5RNG r; r5_seed(&r, 11);
    R5Creature a = pc(R5C_CLERIC, 1, 14, 10, 12);
    R5Creature t = monster(12, 50);
    for (int i = 0; i < 500; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_MACE], R5F_BLESS);
        CHECK(at.bless.n == 1);
        CHECK(at.bless.total >= 1 && at.bless.total <= 4);
        CHECK(at.bonus == 2 + 2 + at.bless.total);     /* STR+prof+bless */
        /* cleric WIS save: proficient (+2), WIS 10 (+0), plus bless */
        R5Save sv = r5_save(&r, &a, R5_WIS, 12, R5F_BLESS);
        CHECK(sv.total == sv.d20.total + 2 + sv.bless.total);
        CHECK(sv.bless.total >= 1 && sv.bless.total <= 4);
    }
}

/* closed-form expectations must match Monte Carlo within tolerance */
static void test_expectations(void) {
    R5RNG r; r5_seed(&r, 12);
    static const struct { int bonus, ac, flags; R5DiceSpec d; } cases[] = {
        { 5, 13, 0,        { 1, 4, 3 } },    /* imp sting     */
        { 4, 16, 0,        { 2, 6, 3 } },    /* vs plate-ish  */
        { 2, 11, R5F_ADV,  { 1, 8, 2 } },
        { 7, 15, R5F_DIS,  { 2, 10, 4 } },
        { 3, 25, 0,        { 1, 6, 0 } },    /* nat-20 only   */
        { 9, 5,  0,        { 1, 4, 0 } },    /* nat-1 only miss */
    };
    for (unsigned ci = 0; ci < sizeof cases / sizeof cases[0]; ci++) {
        R5MAttack ma = { "T", (int8_t)cases[ci].bonus, cases[ci].d, DT_SLASHING,
                         { 0, 0, 0 }, 0, 0, 0, 0 };
        R5Creature t = monster(cases[ci].ac, 30000);
        long total = 0, hits = 0;
        const int N = 60000;
        for (int i = 0; i < N; i++) {
            R5Attack at = r5_monster_attack(&r, 0, &ma, &t, cases[ci].flags);
            if (at.hit) { hits++; total += at.damage; }
        }
        int sim_hit = (int)(hits * 100 / N);
        int sim_ev = (int)(total * 100 / N);
        int cf_hit = r5_hit_pct(cases[ci].bonus, cases[ci].ac, cases[ci].flags);
        int cf_ev = r5_ev_attack_x100(cases[ci].bonus, cases[ci].ac,
                                      cases[ci].flags, cases[ci].d);
        int dh = sim_hit - cf_hit; if (dh < 0) dh = -dh;
        int dev = sim_ev - cf_ev; if (dev < 0) dev = -dev;
        CHECKI(dh <= 1, "case %u hit%% sim=%d cf=%d", ci, sim_hit, cf_hit);
        CHECKI(dev <= 25 + cf_ev / 25, "case %u ev sim=%d cf=%d", ci, sim_ev, cf_ev);
    }
}

int main(void) {
    test_expectations();
    test_dice();
    test_advantage();
    test_mods();
    test_attack_math();
    test_finesse_and_ranged();
    test_versatile();
    test_crit_doubles_dice_not_mod();
    test_hunters_mark_and_sneak();
    test_monster_attack_rider();
    test_resist_immune_vuln();
    test_temp_hp();
    test_downed_and_healing();
    test_conditions_and_conc();
    test_second_wind_and_surge();
    test_sneak_progression();
    test_slots();
    test_save_proficiency();
    test_bless();
    printf("%d checks, %d failures\n", checks, fails);
    return fails != 0;
}
