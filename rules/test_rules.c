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

static void test_weapon_rider(void) {
    R5RNG r; r5_seed(&r, 13);
    R5Creature a = pc(R5C_FIGHTER, 2, 16, 10, 14);
    R5Creature t = monster(1, 30000);          /* always hit */
    int hits = 0, saves = 0, fullhits = 0;
    for (int i = 0; i < 2000; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_EVERBURN], 0);
        if (!at.hit) continue;
        hits++;
        CHECK(at.rider_dmg.n >= 1);            /* 1d4 fire, 2d4 on crit */
        CHECK(at.rider_type == DT_FIRE);
        if (at.crit) CHECK(at.rider_dmg.n == 2);
        if (at.rider_saved) { saves++; CHECK(at.rider_damage == at.rider_dmg.total / 2); }
        else { fullhits++; CHECK(at.rider_damage == at.rider_dmg.total); }
    }
    CHECK(hits > 1500 && saves > 100 && fullhits > 100);
    /* plain weapons have no rider */
    R5Attack at = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_GREATSWORD], 0);
    if (at.hit) CHECK(at.rider_dmg.n == 0);
}

/* ------------------------------------------------- races + backgrounds
 * Expectations restated from SRD 5.1 race/background text (CC-BY-4.0) and
 * character2.md identities -- never read out of the generator. */

static void test_races_table(void) {
    /* race 0 is "none": the pre-pick compatibility sheet, all deltas zero */
    const R5Race* nn = &r5_races[R5RACE_NONE];
    for (int a = 0; a < 6; a++) CHECK(nn->asi[a] == 0);
    CHECK(nn->traits == 0 && nn->resist == 0 && nn->hp_per_level == 0 &&
          nn->skills == 0);

    static const struct {
        int id; int8_t asi[6]; uint8_t traits; uint16_t resist;
        uint8_t hp_lvl; uint32_t skills; uint8_t srd;
    } want[] = {
        /* hill dwarf: CON+2 WIS+1, Resilience, Toughness (+1 hp/level) */
        { R5RACE_HILL_DWARF, { 0, 0, 2, 0, 1, 0 },
          TR_DARKVISION | TR_POISON_RESIL, 1u << DT_POISON, 1, 0, 1 },
        /* high elf: DEX+2 INT+1, Keen Senses, Fey Ancestry */
        { R5RACE_HIGH_ELF, { 0, 2, 0, 1, 0, 0 }, TR_DARKVISION | TR_FEY,
          0, 0, 1u << SK_PERCEPTION, 1 },
        /* lightfoot: DEX+2 CHA+1, Lucky; halflings see no better in the dark */
        { R5RACE_LIGHTFOOT, { 0, 2, 0, 0, 0, 1 }, TR_LUCKY, 0, 0, 0, 1 },
        { R5RACE_HUMAN, { 1, 1, 1, 1, 1, 1 }, 0, 0, 0, 0, 1 },
        /* dragonborn: STR+2 CHA+1; ancestry/breath/resistance are one
         * named socket, so no bits and no resist land today */
        { R5RACE_DRAGONBORN, { 2, 0, 0, 0, 0, 1 }, 0, 0, 0, 0, 1 },
        { R5RACE_ROCK_GNOME, { 0, 0, 1, 2, 0, 0 },
          TR_DARKVISION | TR_CUNNING, 0, 0, 0, 1 },
        /* half-elf: CHA+2 plus the fixed DEX/WIS pin (no floating ASIs) */
        { R5RACE_HALF_ELF, { 0, 1, 0, 0, 1, 2 }, TR_DARKVISION | TR_FEY,
          0, 0, 0, 1 },
        { R5RACE_HALF_ORC, { 2, 0, 1, 0, 0, 0 },
          TR_DARKVISION | TR_RELENTLESS | TR_SAVAGE, 0, 0,
          1u << SK_INTIMIDATION, 1 },
        { R5RACE_TIEFLING, { 0, 0, 0, 1, 0, 2 }, TR_DARKVISION,
          1u << DT_FIRE, 0, 0, 1 },
        /* githyanki: homebrew stat block, never an SRD claim */
        { R5RACE_GITHYANKI, { 2, 0, 0, 1, 0, 0 }, 0, 0, 0, 0, 0 },
    };
    for (unsigned i = 0; i < sizeof want / sizeof *want; i++) {
        const R5Race* rr = &r5_races[want[i].id];
        for (int a = 0; a < 6; a++)
            CHECKI(rr->asi[a] == want[i].asi[a], "%s asi[%d]=%d",
                   rr->name, a, rr->asi[a]);
        CHECKI(rr->traits == want[i].traits, "%s traits %02x", rr->name, rr->traits);
        CHECKI(rr->resist == want[i].resist, "%s resist %04x", rr->name, rr->resist);
        CHECK(rr->hp_per_level == want[i].hp_lvl);
        CHECKI(rr->skills == want[i].skills, "%s skills %x",
               rr->name, (unsigned)rr->skills);
        CHECK(rr->srd == want[i].srd);
    }
    /* the picker grouping covers each playable race exactly once */
    uint32_t seen = 0;
    for (int b = 0; b < R5RACE_BASE_COUNT; b++)
        for (int k = 0; k < r5_race_bases[b].n; k++) {
            int e = r5_race_bases[b].first + k;
            CHECK(!(seen & (1u << e)));
            seen |= 1u << e;
        }
    CHECK(seen == 0x7FEu);                          /* entries 1..10, no NONE */
}

static void test_backgrounds_table(void) {
    CHECK(r5_backgrounds[R5BG_NONE].skills == 0);
    static const struct { int id; uint32_t skills; uint8_t srd; } want[] = {
        { R5BG_ACOLYTE,     (1u << SK_INSIGHT) | (1u << SK_RELIGION), 1 },
        { R5BG_CRIMINAL,    (1u << SK_DECEPTION) | (1u << SK_STEALTH), 0 },
        { R5BG_SAGE,        (1u << SK_ARCANA) | (1u << SK_HISTORY), 0 },
        { R5BG_SOLDIER,     (1u << SK_ATHLETICS) | (1u << SK_INTIMIDATION), 0 },
        { R5BG_ENTERTAINER, (1u << SK_ACROBATICS) | (1u << SK_PERFORMANCE), 0 },
        { R5BG_FOLK_HERO,   (1u << SK_ANIMAL_HANDLING) | (1u << SK_SURVIVAL), 0 },
        { R5BG_NOBLE,       (1u << SK_HISTORY) | (1u << SK_PERSUASION), 0 },
        { R5BG_OUTLANDER,   (1u << SK_ATHLETICS) | (1u << SK_SURVIVAL), 0 },
        { R5BG_URCHIN,      (1u << SK_SLEIGHT_OF_HAND) | (1u << SK_STEALTH), 0 },
        { R5BG_HAUNTED_ONE, (1u << SK_INVESTIGATION) | (1u << SK_SURVIVAL), 0 },
    };
    for (unsigned i = 0; i < sizeof want / sizeof *want; i++) {
        const R5Background* b = &r5_backgrounds[want[i].id];
        CHECKI(b->skills == want[i].skills, "%s skills %x",
               b->name, (unsigned)b->skills);
        CHECK(b->srd == want[i].srd);
        int bits = 0;
        for (int s = 0; s < SK_COUNT; s++) bits += (int)((b->skills >> s) & 1);
        CHECK(bits == 2);                    /* a background IS two skills */
        CHECK(b->blurb[0] != 0);             /* ...and one flavor line */
    }
}

/* Dwarven Resilience: advantage on the save against poison riders (imp
 * sting, stinger venom); other damage types get no such grace. */
static void test_poison_resilience(void) {
    R5RNG r; r5_seed(&r, 77);
    const R5MAttack* sting = &r5_monsters[R5M_IMP].attacks[0];
    R5Creature t = pc(R5C_FIGHTER, 1, 16, 10, 14);
    t.hpmax = t.hp = 30000;
    t.traits = TR_POISON_RESIL;
    int hits = 0;
    for (int i = 0; i < 600; i++) {
        R5Attack at = r5_monster_attack(&r, 0, sting, &t, 0);
        if (!at.hit) continue;
        hits++;
        CHECK(at.rider_type == DT_POISON);
        CHECK(at.rider_save.n == 2);         /* advantage: two dice thrown */
    }
    CHECK(hits > 150);
    /* without the trait the same save is a single die */
    t.traits = 0;
    for (int i = 0; i < 200; i++) {
        R5Attack at = r5_monster_attack(&r, 0, sting, &t, 0);
        if (at.hit) CHECK(at.rider_save.n == 1);
    }
    /* the weapon path honors it too (stinger venom)... */
    R5Creature a = pc(R5C_FIGHTER, 2, 16, 10, 14);
    R5Creature vt = monster(1, 30000);
    vt.traits = TR_POISON_RESIL;
    int vhits = 0;
    for (int i = 0; i < 200; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &vt, &r5_weapons[R5W_STINGER], 0);
        if (!at.hit) continue;
        vhits++;
        CHECK(at.rider_type == DT_POISON);
        CHECK(at.rider_save.n == 2);
    }
    CHECK(vhits > 150);
    /* ...but a FIRE rider against the same dwarf stays a straight roll */
    for (int i = 0; i < 200; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &vt, &r5_weapons[R5W_EVERBURN], 0);
        if (at.hit) CHECK(at.rider_save.n == 1);
    }
}

/* ---------------------------------------------------- Character 2.0 pools */
static void test_char2_pools(void) {
    R5Creature b = { 0 };
    b.cls = R5C_BARBARIAN; b.level = 3; b.hp = b.hpmax = 30;
    b.ab[R5_STR] = 16; b.ab[R5_DEX] = 10;
    r5_refill(&b);
    CHECK(b.rsrc[R5R_RAGE] == 3);                 /* SRD: 3 rages at level 3 */
    CHECK(r5_can_rage(&b));
    r5_start_rage(&b);
    CHECK(b.conds & C_RAGING);
    CHECK(b.resist & (1u << DT_SLASHING));
    CHECK(!r5_can_rage(&b));                      /* not while raging */
    r5_end_rage(&b);
    CHECK(!(b.resist & (1u << DT_SLASHING)));
    CHECK(b.rsrc[R5R_RAGE] == 2);

    /* rage adds exactly +2 to STR melee damage: EV check via direct rolls */
    R5RNG rng = { 12345 };
    const R5Weapon* gs = &r5_weapons[R5W_GREATSWORD];
    long base = 0, raged = 0; int hits1 = 0, hits2 = 0;
    R5Creature dummy = { 0 }; dummy.ac = 10; dummy.hp = dummy.hpmax = 1000;
    for (int i = 0; i < 20000; i++) {
        R5Attack at = r5_weapon_attack(&rng, &b, &dummy, gs, 0);
        if (at.hit && !at.crit) { base += at.dmg.total; hits1++; }
    }
    b.conds |= C_RAGING;
    for (int i = 0; i < 20000; i++) {
        R5Attack at = r5_weapon_attack(&rng, &b, &dummy, gs, 0);
        if (at.hit && !at.crit) { raged += at.dmg.total; hits2++; }
    }
    b.conds &= (uint16_t)~C_RAGING;
    double d = (double)raged / hits2 - (double)base / hits1;
    CHECK(d > 1.7 && d < 2.3);                    /* +2 flat, sampled */

    R5Creature p = { 0 };
    p.cls = R5C_PALADIN; p.level = 2; p.hp = 4; p.hpmax = 30;
    r5_refill(&p);
    CHECK(p.rsrc[R5R_LAY] == 10);                 /* 5 x level */
    CHECK(r5_lay_hands(&p, &p, 6));
    CHECK(p.hp == 10 && p.rsrc[R5R_LAY] == 4);
    CHECK(!r5_lay_hands(&p, &p, 5));              /* pool short */
    R5DiceSpec sm = r5_smite_dice(2);
    CHECK(sm.n == 3 && sm.sides == 8);            /* 2nd-level smite = 3d8 */

    R5Creature w = { 0 };
    w.cls = R5C_WARLOCK; w.level = 3; w.hp = w.hpmax = 20;
    r5_refill(&w);
    CHECK(w.rsrc[R5R_PACT] == 2);
    CHECK(r5_pact_cast(&w) && r5_pact_cast(&w) && !r5_pact_cast(&w));
    r5_short_rest(&w);                             /* pact returns on short rest */
    CHECK(w.rsrc[R5R_PACT] == 2);

    /* --- racial traits --- */
    R5Creature h = { 0 };                          /* halfling: Lucky */
    h.cls = R5C_ROGUE; h.level = 1; h.ab[R5_DEX] = 16; h.traits = TR_LUCKY;
    R5RNG lr = { 777 };
    int ones = 0, rolls = 60000;
    for (int i = 0; i < rolls; i++) {
        R5Dice d = r5_d20(&lr, R5F_LUCKY);
        if (d.rolls[0] == 1) ones++;
    }
    /* reroll-once: P(final 1) = 1/400; allow wide slack around 150/60000 */
    CHECK(ones > 60 && ones < 300);

    R5Creature ho = { 0 };                         /* half-orc: Relentless */
    ho.cls = R5C_FIGHTER; ho.level = 1; ho.hp = 5; ho.hpmax = 20;
    ho.traits = TR_RELENTLESS;
    r5_apply_damage(&ho, 50, DT_SLASHING);
    CHECK(ho.hp == 1 && (ho.traits & TR_USED_RELENTLESS));
    r5_apply_damage(&ho, 50, DT_SLASHING);
    CHECK(ho.hp == 0);                             /* once per rest */
    ho.traits = TR_RELENTLESS; ho.hp = 5;
    r5_refill(&ho);                                /* rest clears the use */
    CHECK(!(ho.traits & TR_USED_RELENTLESS));

    R5Creature so = { 0 };                         /* half-orc: Savage crits */
    so.cls = R5C_FIGHTER; so.level = 1; so.ab[R5_STR] = 16;
    so.traits = TR_SAVAGE;
    R5Creature tgt = { 0 }; tgt.ac = 0; tgt.hp = tgt.hpmax = 1000;
    R5RNG sr = { 4242 };
    int saw_crit_dice = 0;
    for (int i = 0; i < 400 && !saw_crit_dice; i++) {
        R5Attack at = r5_weapon_attack(&sr, &so, &tgt,
                                       &r5_weapons[R5W_GREATSWORD], R5F_AUTOCRIT);
        if (at.crit) saw_crit_dice = at.dmg.n;     /* greatsword 2d6 -> savage crit 6 dice */
    }
    CHECK(saw_crit_dice == 6);

    /* subclass passive: Champion crits on 19-20 */
    R5Creature ch = { 0 };
    ch.cls = R5C_FIGHTER; ch.level = 3; ch.ab[R5_STR] = 16; ch.crit_min = 19;
    R5RNG cr = { 999 };
    R5Creature big = { 0 }; big.ac = 1; big.hp = big.hpmax = 30000;
    int crits = 0, hits = 0;
    for (int i = 0; i < 40000; i++) {
        R5Attack at = r5_weapon_attack(&cr, &ch, &big, &r5_weapons[R5W_GREATSWORD], 0);
        if (at.hit) hits++;
        if (at.crit) crits++;
    }
    /* ~10% crit (19-20) vs the ~5% baseline; wide slack */
    CHECK(crits * 100 / hits >= 7 && crits * 100 / hits <= 13);
    ch.crit_min = 20;
    crits = hits = 0;
    for (int i = 0; i < 40000; i++) {
        R5Attack at = r5_weapon_attack(&cr, &ch, &big, &r5_weapons[R5W_GREATSWORD], 0);
        if (at.hit) hits++;
        if (at.crit) crits++;
    }
    CHECK(crits * 100 / hits >= 3 && crits * 100 / hits <= 7);

    R5Creature mk = { 0 };
    mk.cls = R5C_MONK; mk.level = 2; r5_refill(&mk);
    CHECK(mk.rsrc[R5R_KI] == 2 && r5_martial_die(&mk) == 4);
}

/* ---------------------------------------------------------- skill checks
 * r5_skill_check is live field code (src/events.c field_check) and was the
 * one untested entry point into the rules engine. */

static void test_skill_check_math(void) {
    R5RNG r; r5_seed(&r, 14);
    /* pin the generated SRD table before leaning on it */
    CHECK(r5_skill_ability[SK_ATHLETICS] == R5_STR);
    CHECK(r5_skill_ability[SK_STEALTH] == R5_DEX);
    CHECK(r5_skill_ability[SK_PERCEPTION] == R5_WIS);
    R5Creature c = pc(R5C_FIGHTER, 1, 16, 14, 10);     /* STR +3, DEX +2 */
    c.ab[R5_WIS] = 8;                                  /* WIS -1 */
    for (int i = 0; i < 500; i++) {
        int d20;
        int t = r5_skill_check(&r, &c, SK_ATHLETICS, 0, 0, &d20);
        CHECK(d20 >= 1 && d20 <= 20);                  /* out-param = raw die */
        CHECK(t == d20 + 3);
        t = r5_skill_check(&r, &c, SK_STEALTH, 0, 0, &d20);
        CHECK(t == d20 + 2);
        t = r5_skill_check(&r, &c, SK_PERCEPTION, 0, 0, &d20);
        CHECK(t == d20 - 1);                           /* negative mods too */
    }
    int t = r5_skill_check(&r, &c, SK_ATHLETICS, 0, 0, 0);   /* d20out optional */
    CHECK(t >= 1 + 3 && t <= 20 + 3);
}

static void test_skill_check_prof_expertise(void) {
    R5Creature c = pc(R5C_ROGUE, 3, 10, 16, 10);       /* DEX +3, prof +2 */
    uint32_t bit = 1u << SK_STEALTH;
    for (uint32_t seed = 1; seed <= 200; seed++) {
        R5RNG r; int d0, d1, d2;
        r5_seed(&r, seed);
        int base = r5_skill_check(&r, &c, SK_STEALTH, 0, 0, &d0);
        r5_seed(&r, seed);
        int prof = r5_skill_check(&r, &c, SK_STEALTH, bit, 0, &d1);
        r5_seed(&r, seed);
        int expt = r5_skill_check(&r, &c, SK_STEALTH, bit, bit, &d2);
        CHECK(d1 == d0 && d2 == d0);                   /* same seed, same die */
        CHECK(base == d0 + 3);
        CHECK(prof == base + 2);                       /* +prof */
        CHECK(expt == base + 4);                       /* expertise: +prof again */
    }
}

static void test_skill_check_lucky(void) {
    R5RNG r; r5_seed(&r, 15);
    R5Creature h = pc(R5C_ROGUE, 1, 10, 16, 10);
    h.traits = TR_LUCKY;
    R5Creature n = pc(R5C_ROGUE, 1, 10, 16, 10);
    int lucky_ones = 0, plain_ones = 0, d20;
    const int N = 20000;
    for (int i = 0; i < N; i++) {
        r5_skill_check(&r, &h, SK_STEALTH, 0, 0, &d20);
        if (d20 == 1) lucky_ones++;
        r5_skill_check(&r, &n, SK_STEALTH, 0, 0, &d20);
        if (d20 == 1) plain_ones++;
    }
    /* Lucky rerolls nat 1s once on checks: P(1) = 1/400 vs 1/20 */
    CHECKI(lucky_ones > 10 && lucky_ones < 120, "lucky ones %d", lucky_ones);
    CHECKI(plain_ones > 800 && plain_ones < 1200, "plain ones %d", plain_ones);
}

/* --------------------------------------------------- uncovered rule edges */

static void test_monster_crit_min_and_prof(void) {
    R5RNG r; r5_seed(&r, 16);
    R5Creature m = monster(10, 50);
    CHECK(r5_prof(&m) == 2);                           /* monsters: flat +2 */
    /* a monster attacker with a widened crit range drives the a->crit_min
     * path in resolve_to_hit (previously only NULL attackers were tested) */
    R5MAttack ma = { "Claw", 4, { 1, 6, 2 }, DT_SLASHING,
                     { 0, 0, 0 }, 0, 0, 0, 0 };
    R5Creature t = monster(1, 30000);
    m.crit_min = 19;
    int saw19 = 0, saw20 = 0;
    for (int i = 0; i < 4000; i++) {
        R5Attack at = r5_monster_attack(&r, &m, &ma, &t, 0);
        CHECK(at.crit == (at.d20.total >= 19));        /* crit iff die >= 19 */
        if (at.d20.total == 19) saw19 = 1;
        if (at.d20.total == 20) saw20 = 1;
    }
    CHECK(saw19 && saw20);
}

static void test_rage_exclusions(void) {
    R5RNG r; r5_seed(&r, 17);
    /* dmg.mod records ability + rage flat exactly: no sampling needed */
    R5Creature b = pc(R5C_BARBARIAN, 1, 16, 10, 14);   /* STR +3, DEX +0 */
    b.conds |= C_RAGING;
    R5Creature d = pc(R5C_BARBARIAN, 1, 10, 16, 14);   /* STR +0, DEX +3 */
    d.conds |= C_RAGING;
    R5Creature t = monster(1, 30000);
    int seen = 0;
    for (int i = 0; i < 400 && seen != 15; i++) {
        R5Attack gs = r5_weapon_attack(&r, &b, &t, &r5_weapons[R5W_GREATSWORD], 0);
        if (gs.hit) { CHECK(gs.dmg.mod == 3 + 2); seen |= 1; }  /* STR melee: +2 */
        R5Attack lb = r5_weapon_attack(&r, &b, &t, &r5_weapons[R5W_LONGBOW], 0);
        if (lb.hit) { CHECK(lb.dmg.mod == 0); seen |= 2; }      /* ranged: no rage */
        R5Attack ds = r5_weapon_attack(&r, &b, &t, &r5_weapons[R5W_DAGGER], 0);
        if (ds.hit) { CHECK(ds.dmg.mod == 3 + 2); seen |= 4; }  /* finesse, STR wins: +2 */
        R5Attack dd = r5_weapon_attack(&r, &d, &t, &r5_weapons[R5W_DAGGER], 0);
        if (dd.hit) { CHECK(dd.dmg.mod == 3); seen |= 8; }      /* DEX finesse: no rage */
    }
    CHECK(seen == 15);
}

static void test_lucky_attack_and_save(void) {
    R5RNG r; r5_seed(&r, 18);
    /* TR_LUCKY must flow through the full attack and save paths, not just
     * bare r5_d20: final nat-1 rate drops from 1/20 to 1/400 */
    R5Creature h = pc(R5C_FIGHTER, 1, 16, 10, 14);
    h.traits = TR_LUCKY;
    R5Creature t = monster(30, 1000);
    int ones = 0;
    const int N = 20000;
    for (int i = 0; i < N; i++) {
        R5Attack at = r5_weapon_attack(&r, &h, &t, &r5_weapons[R5W_LONGSWORD], 0);
        if (at.d20.rolls[0] == 1) ones++;
    }
    CHECKI(ones > 10 && ones < 120, "lucky attack nat1s %d", ones);
    ones = 0;
    for (int i = 0; i < N; i++) {
        R5Save sv = r5_save(&r, &h, R5_DEX, 15, 0);
        if (sv.d20.rolls[0] == 1) ones++;
    }
    CHECKI(ones > 10 && ones < 120, "lucky save nat1s %d", ones);
}

static void test_apply_damage_edges(void) {
    R5Creature t = monster(10, 50);
    t.temp_hp = 10;
    CHECK(r5_apply_damage(&t, 4, DT_SLASHING) == 0);   /* absorbed entirely */
    CHECK(t.temp_hp == 6 && t.hp == 50);
    /* Relentless never fires for monsters */
    R5Creature m = monster(10, 5);
    m.traits = TR_RELENTLESS;
    r5_apply_damage(&m, 9, DT_SLASHING);
    CHECK(m.hp == 0 && !(m.traits & TR_USED_RELENTLESS));
    /* half-orc PC at 1 hp: Relentless holds the line, reports 0 lost */
    R5Creature p = pc(R5C_FIGHTER, 1, 16, 10, 14);
    p.hp = 1; p.traits = TR_RELENTLESS;
    CHECK(r5_apply_damage(&p, 1, DT_SLASHING) == 0);
    CHECK(p.hp == 1 && (p.traits & TR_USED_RELENTLESS));
}

static void test_extra_d6_merge(void) {
    R5RNG r; r5_seed(&r, 19);
    /* greatsword 2d6 + hunter's mark d6: same die size, one merged roll
     * (the mixed-size path is covered by longbow+mark elsewhere) */
    R5Creature a = pc(R5C_RANGER, 1, 16, 10, 12);
    R5Creature t = monster(1, 30000);
    int plain = 0, crit = 0;
    for (int i = 0; i < 2000 && !(plain && crit); i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_GREATSWORD],
                                       R5F_MARK);
        if (!at.hit) continue;
        CHECK(at.dmg.sides == 6);
        if (at.crit) { crit = 1; CHECK(at.dmg.n == 6); }   /* (2+1) doubled */
        else { plain = 1; CHECK(at.dmg.n == 3); }
        int sum = 0;
        for (int j = 0; j < at.dmg.n; j++) sum += at.dmg.rolls[j];
        CHECK(at.dmg.total == sum + at.dmg.mod);
    }
    CHECK(plain && crit);
}

static void test_feature_denials(void) {
    R5Creature f = pc(R5C_FIGHTER, 2, 16, 10, 14);
    f.hp = 0;
    CHECK(!r5_can_second_wind(&f));                    /* downed fighter */
    R5Creature w = pc(R5C_WIZARD, 2, 8, 14, 12);
    CHECK(!r5_can_action_surge(&w));                   /* wrong class */
    CHECK(!r5_can_rage(&w));
    R5Creature b = pc(R5C_BARBARIAN, 1, 16, 10, 14);   /* never refilled */
    CHECK(b.rsrc[R5R_RAGE] == 0 && !r5_can_rage(&b));  /* empty pool */
    CHECK(r5_martial_die(&b) == 0);                    /* not a monk */
}

static void test_guard_rails(void) {
    R5RNG r;
    r5_seed(&r, 0);                                    /* 0 falls back, not stuck */
    CHECK(r.s != 0);
    int die = r5_die(&r, 6);
    CHECK(die >= 1 && die <= 6);
    R5Dice big = r5_roll(&r, 99, 6, 0);                /* dice-count clamp */
    CHECK(big.n == R5_MAX_DICE);
    CHECK(big.total >= R5_MAX_DICE && big.total <= R5_MAX_DICE * 6);

    R5Creature f = pc(R5C_FIGHTER, 0, 16, 10, 14);     /* level clamps */
    CHECK(r5_prof(&f) == 2);
    f.level = 9;
    CHECK(r5_prof(&f) == 2);
    R5Creature ro = pc(R5C_ROGUE, 0, 8, 16, 10);
    CHECK(r5_sneak_dice(&ro) == 1);
    ro.level = 9;
    CHECK(r5_sneak_dice(&ro) == 2);

    R5Creature wz = pc(R5C_WIZARD, 1, 8, 14, 12);      /* slot bounds */
    wz.slots[0] = 2;
    CHECK(!r5_spend_slot(&wz, 0) && !r5_spend_slot(&wz, 4));
    CHECK(wz.slots[0] == 2);

    R5Creature m = monster(10, 10);                    /* refill bad-class guard */
    m.rsrc[R5R_RAGE] = 7;
    m.traits = TR_USED_RELENTLESS;
    r5_refill(&m);
    CHECK(m.rsrc[R5R_RAGE] == 7);                      /* class table not consulted */
    CHECK(!(m.traits & TR_USED_RELENTLESS));           /* but the rest-flag clears */

    R5Creature p = pc(R5C_BARD, 1, 10, 14, 12);        /* heal(<=0) is a no-op */
    r5_apply_damage(&p, 99, DT_SLASHING);
    CHECK(p.hp == 0 && (p.conds & C_UNCONSCIOUS));
    r5_heal(&p, 0);
    r5_heal(&p, -5);
    CHECK(p.hp == 0 && (p.conds & C_UNCONSCIOUS));     /* no phantom wake */

    R5Creature pal = pc(R5C_PALADIN, 2, 16, 10, 14);   /* lay_hands(<=0) */
    r5_refill(&pal);
    uint8_t pool = pal.rsrc[R5R_LAY];
    CHECK(!r5_lay_hands(&pal, &p, 0) && !r5_lay_hands(&pal, &p, -2));
    CHECK(pal.rsrc[R5R_LAY] == pool);

    /* EV clamp: a huge negative mod can't drive expected damage below 0;
     * only the crit term (dice alone) survives: 50 * 10 / 40 = 12 */
    R5DiceSpec bad = { 1, 4, -10 };
    CHECK(r5_ev_attack_x100(0, 10, 0, bad) == 12);
}

static void test_weapon_spec_edges(void) {
    R5RNG r; r5_seed(&r, 20);
    R5Creature a = pc(R5C_FIGHTER, 1, 10, 10, 14);     /* +0 mods: clean bounds */
    R5Creature t = monster(1, 30000);
    /* versatile flag on a weapon with no versatile spec falls back */
    int seen = 0;
    for (int i = 0; i < 100 && !seen; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &r5_weapons[R5W_GREATSWORD],
                                       R5F_VERSATILE);
        if (!at.hit) continue;
        seen = 1;
        CHECK(at.dmg.sides == 6);                      /* still 2d6 */
        CHECK(at.dmg.n == (at.crit ? 4 : 2));
    }
    CHECK(seen);
    /* rider with dc 0: no save is rolled, full rider damage lands */
    R5Weapon venom = { "Venom", { 1, 6, 0 }, { 0, 0, 0 }, 0, DT_SLASHING,
                       { 1, 4, 0 }, DT_POISON, 0, 0 };
    seen = 0;
    for (int i = 0; i < 100 && !seen; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &venom, 0);
        if (!at.hit) continue;
        seen = 1;
        CHECK(at.rider_dmg.n >= 1);
        CHECK(at.rider_save.n == 0 && !at.rider_saved);
        CHECK(at.rider_damage == at.rider_dmg.total);
    }
    CHECK(seen);
    /* oversized crit merge: the roll RECORD caps at R5_MAX_DICE, the total
     * still includes every die (5d8 crit = 10 rolls + 2d6 mark off-record) */
    R5Weapon huge = { "Maul+", { 5, 8, 0 }, { 0, 0, 0 }, 0, DT_BLUDGEONING,
                      { 0, 0, 0 }, 0, 0, 0 };
    seen = 0;
    for (int i = 0; i < 100 && !seen; i++) {
        R5Attack at = r5_weapon_attack(&r, &a, &t, &huge,
                                       R5F_MARK | R5F_AUTOCRIT);
        if (!at.hit) continue;
        seen = 1;
        CHECK(at.crit);
        CHECK(at.dmg.n == R5_MAX_DICE);
        CHECK(at.dmg.total >= 10 + 2 && at.dmg.total <= 80 + 12);
    }
    CHECK(seen);
}

/* ------------------------------------------------------------ registry
 * Honest counting: the headline number is TEST FUNCTIONS. Sampling loops
 * execute thousands of assertions per test; that figure is reported in
 * parentheses only and is never what "N tests" means. */

typedef struct { const char* name; void (*fn)(void); } R5Test;
#define T(f) { #f, f }
static const R5Test tests[] = {
    T(test_weapon_rider),
    T(test_races_table),
    T(test_backgrounds_table),
    T(test_poison_resilience),
    T(test_expectations),
    T(test_dice),
    T(test_advantage),
    T(test_mods),
    T(test_attack_math),
    T(test_finesse_and_ranged),
    T(test_versatile),
    T(test_crit_doubles_dice_not_mod),
    T(test_hunters_mark_and_sneak),
    T(test_monster_attack_rider),
    T(test_resist_immune_vuln),
    T(test_temp_hp),
    T(test_downed_and_healing),
    T(test_conditions_and_conc),
    T(test_second_wind_and_surge),
    T(test_sneak_progression),
    T(test_slots),
    T(test_save_proficiency),
    T(test_bless),
    T(test_char2_pools),
    T(test_skill_check_math),
    T(test_skill_check_prof_expertise),
    T(test_skill_check_lucky),
    T(test_monster_crit_min_and_prof),
    T(test_rage_exclusions),
    T(test_lucky_attack_and_save),
    T(test_apply_damage_edges),
    T(test_extra_d6_merge),
    T(test_feature_denials),
    T(test_guard_rails),
    T(test_weapon_spec_edges),
};
#undef T

int main(void) {
    const int ntests = (int)(sizeof tests / sizeof tests[0]);
    int failed_tests = 0;
    for (int i = 0; i < ntests; i++) {
        int before = fails;
        tests[i].fn();
        if (fails > before) {
            failed_tests++;
            printf("FAILED %s (%d failing checks)\n",
                   tests[i].name, fails - before);
        } else {
            printf("ok %s\n", tests[i].name);
        }
    }
    printf("%d tests, %d failures "
           "(%d assertion executions — sampling loops inflate this figure)\n",
           ntests, failed_tests, checks);
    return fails != 0;
}
