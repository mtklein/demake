/* Host-native test suite: REAL game logic (menu.c, data.c, party5.c,
 * encounter.c, game.c + rules/) driven through the fake engine under
 * ASan/UBSan. Run: make -C test/host sim   (optionally: sim ARGS=substr)
 *
 * Expectation tables here are written independently against the design
 * (docs/character2.md + the SRD progression), NOT copied from the code
 * under test -- that is what makes them an oracle. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_engine.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "party5.h"
#include "rules.h"
#include "encounter.h"
#include "assets.h"

void field_menu(void);        /* menu.c  */
int game_class_select(void);  /* game.c  */

#define T_ASSERT(cond, ...) do { if (!(cond)) sim_fail(__VA_ARGS__); } while (0)

/* ---------------------------------------------------------------- helpers */

static void mk_party(int cls, int lvl) {
    G_DEMO_LEVEL = (u8)lvl;      /* same pre-level hook the mGBA harness pokes */
    party_init(cls, "TAV");
}

static const char* const cls_display[CLS_COUNT] = {
    "Bard", "Rogue", "Ranger", "Wizard", "Fighter", "Cleric",
    "Barbarian", "Druid", "Monk", "Paladin", "Sorcerer", "Warlock"
};

/* ------------------------------------------------- (a) battle menu kits
 * The expected root menu per class x level 1..3, from the design:
 *   - only bards mock; Smite is paladin L2+ (first slots); WildShape is
 *     druid L2+; rage is barbarian-only; ki (Pat.Def) arrives monk L2;
 *     Hunter's Mark needs ranger slots (L2+); warlocks cast leveled spells
 *     on pact slots (present from L1); everyone gets Attack/Item/Dodge/
 *     Tactics/End Turn.
 * Codes: fixed actions 0..9, class features 20..25, spells SPELL_CODE(100)
 * + R5S_id -- pinned here so no generated spell id can ever collide with a
 * feature case again (the druid-Smite bug). */
typedef struct { const char* label; int code; } Row;
#define SP(id) (100 + (id))

static const Row kit_bard[] = {
    { "Attack", 0 }, { "V.Mockery", SP(R5S_VICIOUS_MOCKERY) },
    { "Item", 1 }, { "Dodge", 2 },
    { "Heal.Word", SP(R5S_HEALING_WORD) },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_rogue[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 }, { "Hide", 5 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_ranger1[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_ranger2[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 }, { "Hunt.Mark", 6 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_wizard[] = {
    { "Attack", 0 }, { "Fire Bolt", SP(R5S_FIRE_BOLT) },
    { "M.Missile", SP(R5S_MAGIC_MISSILE) }, { "Sleep", SP(R5S_SLEEP) },
    { "Item", 1 }, { "Dodge", 2 }, { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_fighter[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 }, { "2nd Wind", 4 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_cleric[] = {
    { "Attack", 0 }, { "S.Flame", SP(R5S_SACRED_FLAME) },
    { "Guid.Bolt", SP(R5S_GUIDING_BOLT) }, { "Cure Wnds", SP(R5S_CURE_WOUNDS) },
    { "Bless", SP(R5S_BLESS) },
    { "Item", 1 }, { "Dodge", 2 }, { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_barb[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 }, { "RAGE!", 20 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_druid1[] = {
    { "Attack", 0 }, { "P.Flame", SP(R5S_PRODUCE_FLAME) },
    { "Psn.Spray", SP(R5S_POISON_SPRAY) }, { "Cure Wnds", SP(R5S_CURE_WOUNDS) },
    { "Item", 1 }, { "Dodge", 2 },
    { "Heal.Word", SP(R5S_HEALING_WORD) },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_druid2[] = {
    { "Attack", 0 }, { "WildShape", 25 },
    { "P.Flame", SP(R5S_PRODUCE_FLAME) }, { "Psn.Spray", SP(R5S_POISON_SPRAY) },
    { "Cure Wnds", SP(R5S_CURE_WOUNDS) },
    { "Item", 1 }, { "Dodge", 2 },
    { "Heal.Word", SP(R5S_HEALING_WORD) },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_monk1[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_monk2[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 }, { "Pat.Def", 23 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_pal1[] = {
    { "Attack", 0 }, { "Lay Hands", 22 }, { "Item", 1 }, { "Dodge", 2 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_pal2[] = {
    { "Attack", 0 }, { "Cure Wnds", SP(R5S_CURE_WOUNDS) },
    { "Bless", SP(R5S_BLESS) }, { "Lay Hands", 22 }, { "Smite", 24 },
    { "Item", 1 }, { "Dodge", 2 }, { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_sorc[] = {
    { "Attack", 0 }, { "Fire Bolt", SP(R5S_FIRE_BOLT) },
    { "RayFrost", SP(R5S_RAY_OF_FROST) }, { "M.Missile", SP(R5S_MAGIC_MISSILE) },
    { "Sleep", SP(R5S_SLEEP) },
    { "Item", 1 }, { "Dodge", 2 }, { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_warlock[] = {
    { "Attack", 0 }, { "Eld.Blast", SP(R5S_ELDRITCH_BLAST) },
    { "Burn.Hands", SP(R5S_BURNING_HANDS) },
    { "Item", 1 }, { "Dodge", 2 }, { "Tactics", 9 }, { "End Turn", 7 },
};

#define KIT(k) { k, (int)(sizeof k / sizeof *k) }
static const struct { const Row* rows; int n; } kits[CLS_COUNT][3] = {
    [CLS_BARD]      = { KIT(kit_bard),    KIT(kit_bard),    KIT(kit_bard)    },
    [CLS_ROGUE]     = { KIT(kit_rogue),   KIT(kit_rogue),   KIT(kit_rogue)   },
    [CLS_RANGER]    = { KIT(kit_ranger1), KIT(kit_ranger2), KIT(kit_ranger2) },
    [CLS_WIZARD]    = { KIT(kit_wizard),  KIT(kit_wizard),  KIT(kit_wizard)  },
    [CLS_FIGHTER]   = { KIT(kit_fighter), KIT(kit_fighter), KIT(kit_fighter) },
    [CLS_CLERIC]    = { KIT(kit_cleric),  KIT(kit_cleric),  KIT(kit_cleric)  },
    [CLS_BARBARIAN] = { KIT(kit_barb),    KIT(kit_barb),    KIT(kit_barb)    },
    [CLS_DRUID]     = { KIT(kit_druid1),  KIT(kit_druid2),  KIT(kit_druid2)  },
    [CLS_MONK]      = { KIT(kit_monk1),   KIT(kit_monk2),   KIT(kit_monk2)   },
    [CLS_PALADIN]   = { KIT(kit_pal1),    KIT(kit_pal2),    KIT(kit_pal2)    },
    [CLS_SORCERER]  = { KIT(kit_sorc),    KIT(kit_sorc),    KIT(kit_sorc)    },
    [CLS_WARLOCK]   = { KIT(kit_warlock), KIT(kit_warlock), KIT(kit_warlock) },
};

static int build_root_menu(const char** items, u8* code) {
    PcMenuCtx x = {
        (u8)G.pm[0].cls, G.pm[0].level, 0, 0, 0, -1, 0, 0, 0,
    };
    return pc_menu_build(&party5[0], &x, items, code);
}

static void t_battle_menu_kits(void) {
    for (int cls = 0; cls < CLS_COUNT; cls++)
        for (int lvl = 1; lvl <= 3; lvl++) {
            sim_reset();
            mk_party(cls, lvl);
            T_ASSERT(G.pm[0].level == lvl, "%s: level hook gave %d not %d",
                     cls_display[cls], G.pm[0].level, lvl);
            const char* items[12]; u8 code[12];
            int n = build_root_menu(items, code);
            const Row* want = kits[cls][lvl - 1].rows;
            int wn = kits[cls][lvl - 1].n;
            for (int i = 0; i < n && i < wn; i++)
                T_ASSERT(!strcmp(items[i], want[i].label) && code[i] == want[i].code,
                         "%s L%d row %d: got '%s'(code %d), want '%s'(code %d)",
                         cls_display[cls], lvl, i, items[i], code[i],
                         want[i].label, want[i].code);
            T_ASSERT(n == wn, "%s L%d: %d rows, want %d", cls_display[cls], lvl, n, wn);
            /* no duplicate dispatch codes inside one menu */
            for (int i = 0; i < n; i++)
                for (int j = i + 1; j < n; j++)
                    T_ASSERT(code[i] != code[j],
                             "%s L%d: rows '%s' and '%s' share code %d",
                             cls_display[cls], lvl, items[i], items[j], code[i]);
        }
}

static void t_battle_menu_variants(void) {
    const char* items[12]; u8 code[12];

    /* warlock leveled spells ride pact slots: spend them, Burn.Hands goes */
    sim_reset(); mk_party(CLS_WARLOCK, 1);
    T_ASSERT(party5[0].rsrc[R5R_PACT] == 1, "warlock L1 pact slots %d",
             party5[0].rsrc[R5R_PACT]);
    T_ASSERT(r5_pact_cast(&party5[0]) == 1, "pact cast failed");
    int n = build_root_menu(items, code);
    int has_bh = 0, has_eb = 0;
    for (int i = 0; i < n; i++) {
        if (!strcmp(items[i], "Burn.Hands")) has_bh = 1;
        if (!strcmp(items[i], "Eld.Blast")) has_eb = 1;
    }
    T_ASSERT(!has_bh, "warlock with spent pact still offers Burn.Hands");
    T_ASSERT(has_eb, "warlock lost the Eldritch Blast cantrip");

    /* wild-shaped druid (boar creature: no slots) loses spells + WildShape */
    sim_reset(); mk_party(CLS_DRUID, 2);
    R5Creature beast = party5[0];
    beast.slots[0] = beast.slots[1] = beast.slots[2] = 0;
    PcMenuCtx x = { CLS_DRUID, 2, 1 /*shaped*/, 0, 0, -1, 0, 0, 0 };
    n = pc_menu_build(&beast, &x, items, code);
    static const Row shaped_want[] = {
        { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 },
        { "Tactics", 9 }, { "End Turn", 7 },
    };
    T_ASSERT(n == 5, "shaped druid menu has %d rows", n);
    for (int i = 0; i < n; i++)
        T_ASSERT(!strcmp(items[i], shaped_want[i].label) && code[i] == shaped_want[i].code,
                 "shaped druid row %d: '%s'(%d)", i, items[i], code[i]);

    /* engaged melee shows Disengage; nerve objective adds Nerve! */
    sim_reset(); mk_party(CLS_FIGHTER, 1);
    PcMenuCtx x2 = { CLS_FIGHTER, 1, 0, 0, 0, 3 /*engaged*/, 0, 0, 1 /*nerve*/ };
    n = pc_menu_build(&party5[0], &x2, items, code);
    int has_dis = 0, has_nerve = 0;
    for (int i = 0; i < n; i++) {
        if (!strcmp(items[i], "Disengage")) has_dis = 1;
        if (!strcmp(items[i], "Nerve!")) has_nerve = 1;
    }
    T_ASSERT(has_dis && has_nerve, "engaged+nerve fighter: dis=%d nerve=%d",
             has_dis, has_nerve);

    /* concentrating cleric can't stack Bless */
    sim_reset(); mk_party(CLS_CLERIC, 1);
    party5[0].concentrating = R5S_BLESS + 1;
    n = build_root_menu(items, code);
    for (int i = 0; i < n; i++)
        T_ASSERT(strcmp(items[i], "Bless"),
                 "concentrating cleric still offers Bless");

    /* monk with action spent gets Flurry (ki, L2+) in the bonus section */
    sim_reset(); mk_party(CLS_MONK, 2);
    PcMenuCtx x3 = { CLS_MONK, 2, 0, 0, 0, -1, 1 /*action*/, 0, 0 };
    n = pc_menu_build(&party5[0], &x3, items, code);
    static const Row monk_bonus[] = {
        { "Flurry", 21 }, { "Pat.Def", 23 }, { "Tactics", 9 }, { "End Turn", 7 },
    };
    T_ASSERT(n == 4, "monk post-action menu has %d rows", n);
    for (int i = 0; i < n; i++)
        T_ASSERT(!strcmp(items[i], monk_bonus[i].label) && code[i] == monk_bonus[i].code,
                 "monk post-action row %d: '%s'(%d)", i, items[i], code[i]);

    /* armed smite hides the Smite row until it discharges */
    sim_reset(); mk_party(CLS_PALADIN, 2);
    PcMenuCtx x4 = { CLS_PALADIN, 2, 0, 1 /*smited*/, 0, -1, 0, 0, 0 };
    n = pc_menu_build(&party5[0], &x4, items, code);
    for (int i = 0; i < n; i++)
        T_ASSERT(strcmp(items[i], "Smite"), "armed paladin still offers Smite");
}

/* ------------------------------------------------- (b) member sheets */

/* what each class's sheet must list (menu.c cls_spells, by design) */
static const char* const sheet_spells[CLS_COUNT][3] = {
    [CLS_BARD]      = { "Vicious Mockery", "Healing Word", 0 },
    [CLS_ROGUE]     = { 0, 0, 0 },
    [CLS_RANGER]    = { "Hunter's Mark", 0, 0 },
    [CLS_WIZARD]    = { "Fire Bolt", "Magic Missile", "Sleep" },
    [CLS_FIGHTER]   = { 0, 0, 0 },
    [CLS_CLERIC]    = { "Sacred Flame", "Guiding Bolt", "Cure Wounds" },
    [CLS_BARBARIAN] = { 0, 0, 0 },
    [CLS_DRUID]     = { "Produce Flame", "Poison Spray", "Cure Wounds" },
    [CLS_MONK]      = { 0, 0, 0 },
    [CLS_PALADIN]   = { "Cure Wounds", "Bless", 0 },
    [CLS_SORCERER]  = { "Fire Bolt", "Ray Of Frost", "Magic Missile" },
    [CLS_WARLOCK]   = { "Eldritch Blast", "Burning Hands", 0 },
};

/* class-unique markers: if one shows on another class's sheet, state leaked */
static const struct { const char* text; int cls; } sig_marks[] = {
    { "Vicious Mockery", CLS_BARD },
    { "Hunter's Mark",   CLS_RANGER },
    { "Sleep",           CLS_WIZARD },
    { "Sacred Flame",    CLS_CLERIC },
    { "Produce Flame",   CLS_DRUID },
    { "Ray Of Frost",    CLS_SORCERER },
    { "Eldritch Blast",  CLS_WARLOCK },
    { "Second Wind",     CLS_FIGHTER },
    { "Sneak Attack",    CLS_ROGUE },
};

static int in_own_sheet(int cls, const char* text) {
    for (int s = 0; s < 3; s++)
        if (sheet_spells[cls][s] && strstr(text, sheet_spells[cls][s]))
            return 1;
    return 0;
}

static void t_member_sheet_per_class(void) {
    for (int cls = 0; cls < CLS_COUNT; cls++) {
        sim_reset();
        mk_party(cls, 1);
        script_keys("A A SNAP B B");   /* Status -> member 0 -> capture -> out */
        field_menu();
        T_ASSERT(snap_count() == 1, "%s: no sheet snapshot", cls_display[cls]);
        T_ASSERT(snap_contains(0, cls_display[cls]),
                 "%s: class name missing from sheet", cls_display[cls]);
        T_ASSERT(snap_contains(0, "TAV"), "%s: hero name missing", cls_display[cls]);
        T_ASSERT(snap_contains(0, "XP 0/300"), "%s: xp line missing", cls_display[cls]);
        T_ASSERT(snap_contains(0, "Weapon:"), "%s: weapon line missing", cls_display[cls]);
        for (int s = 0; s < 3; s++)
            if (sheet_spells[cls][s])
                T_ASSERT(snap_contains(0, sheet_spells[cls][s]),
                         "%s: sheet lacks %s", cls_display[cls], sheet_spells[cls][s]);
        for (unsigned m = 0; m < sizeof sig_marks / sizeof *sig_marks; m++) {
            if (sig_marks[m].cls == cls) {
                T_ASSERT(snap_contains(0, sig_marks[m].text),
                         "%s: own mark '%s' missing", cls_display[cls], sig_marks[m].text);
            } else if (!in_own_sheet(cls, sig_marks[m].text)) {
                T_ASSERT(!snap_contains(0, sig_marks[m].text),
                         "%s: LEAK -- '%s' (belongs to %s) on this sheet",
                         cls_display[cls], sig_marks[m].text,
                         cls_display[sig_marks[m].cls]);
            }
        }
    }
}

/* the sheet must survive the longest weapon line in the game (this is the
 * test that catches the member_sheet char[] overflow: "Everburn Blade 2d6
 * 2h +fire" through "Weapon: " needs 36 bytes) */
static void t_member_sheet_everburn(void) {
    sim_reset();
    mk_party(CLS_FIGHTER, 2);
    G.weapon[0] = R5W_EVERBURN;
    script_keys("A A SNAP B B");
    field_menu();
    T_ASSERT(snap_contains(0, "Everburn Blade"), "everburn weapon line missing");
    T_ASSERT(snap_contains(0, "Action Surge"), "fighter L2 sheet lacks Action Surge");
}

/* ------------------------------------------------- (c) prepare screen */

static void t_prepare_screen_cleric(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    /* Prepare -> pick TAV -> toggle 3 (the cap: WIS +2 + level 1), bounce
     * off the cap on a 4th, capture before/after, unwind */
    script_keys("DOWN DOWN A A SNAP A DOWN A DOWN A DOWN A SNAP B B B");
    field_menu();
    T_ASSERT(snap_count() == 2, "prepare: %d snapshots", snap_count());
    T_ASSERT(snap_contains(0, "TAV: 0/3 prepared"), "prepare header (before)");
    /* the 29-char SRD name renders: the historical char[30] smash */
    T_ASSERT(snap_contains(0, "Protection From Evil"), "long spell name missing");
    T_ASSERT(snap_contains(0, "Healing Word"), "cleric list lacks Healing Word");
    T_ASSERT(snap_contains(1, "TAV: 3/3 prepared"), "prepare header (after)");
    T_ASSERT(snap_contains(1, "* Healing Word"), "toggle marker missing");
    T_ASSERT(G.pm[0].prepared == 0x7,
             "prepared mask 0x%x, want 0x7 (cap must hold)", (unsigned)G.pm[0].prepared);
}

/* ------------------------------------------------- (d) menu-crawl fuzz */

static u32 fz_state;
static int fz_events, fz_quota, fz_hold;

static u32 fz_next(void) {   /* xorshift32: no wall clock, no rand() */
    u32 x = fz_state;
    x ^= (x << 13) & 0xFFFFFFFFu;
    x ^= x >> 17;
    x ^= (x << 5) & 0xFFFFFFFFu;
    return fz_state = x;
}

static u16 fuzz_gen(u32 frame) {
    (void)frame;
    if (fz_hold) { fz_hold = 0; return 0; }        /* release edge */
    if (fz_events >= fz_quota) { fz_hold = 1; return KEY_B; }   /* unwind */
    if (fz_next() & 1) return 0;                   /* idle frame */
    static const u16 keys[8] = { KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
                                 KEY_A, KEY_B, KEY_START, KEY_A };
    fz_events++;
    fz_hold = 1;
    return keys[fz_next() & 7];
}

static void t_menu_crawl_fuzz(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 2);
    party_add_laezel();
    party_add_shadowheart();
    G.potions = 3;
    G.revivify = 2;
    G.everburn = 1;
    loot_weapon(R5W_RAPIER);
    loot_weapon(R5W_SHORTBOW);
    loot_weapon(R5W_EVERBURN);
    loot_weapon(R5W_STINGER);
    party5[0].hp = 3;              /* give potions something to heal */
    G.pm[0].hp = 3;

    fz_state = 0xBADC0DEu;
    fz_events = 0;
    fz_quota = 10000;
    fz_hold = 0;
    sim_budget(3000000, 0);        /* a hang is a failure, not a timeout */
    script_gen(fuzz_gen);
    int menus = 0;
    while (fz_events < fz_quota) { field_menu(); menus++; }
    T_ASSERT(sim_violations == 0, "fuzz invariant: %s", sim_violation_msg);
    T_ASSERT(menus > 0, "fuzz never entered the menu");
    mgba_logf("fuzz: %d key events across %d menu sessions, %d frames",
              fz_events, menus, (int)sim_frames());
}

/* ------------------------------------------------- scripted menu flows */

static void t_overview_party_of_three(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    party_add_laezel();
    party_add_shadowheart();
    script_keys("SNAP B");
    field_menu();
    T_ASSERT(snap_contains(0, "PARTY"), "overview header missing");
    T_ASSERT(snap_contains(0, "TAV"), "hero row missing");
    T_ASSERT(snap_contains(0, "LAE'ZEL"), "Lae'zel row missing");
    T_ASSERT(snap_contains(0, "SHADOW."), "Shadowheart row missing");
    /* the class column is 8 cells wide: names render truncated */
    T_ASSERT(snap_contains(0, "Lv1 Cler"), "hero class row missing");
    T_ASSERT(snap_contains(0, "Lv1 Figh"), "Lae'zel class row missing");
    T_ASSERT(snap_contains(0, "Potion x2 Scr x1"), "item counts wrong");
    T_ASSERT(snap_contains(0, "Mace"), "cleric weapon missing");
    T_ASSERT(snap_contains(0, "Greatsword"), "fighter weapon missing");
}

static void t_equip_swap(void) {
    sim_reset();
    mk_party(CLS_FIGHTER, 1);
    loot_weapon(R5W_RAPIER);
    script_keys("DOWN A A A B B");
    field_menu();
    T_ASSERT(G.weapon[0] == R5W_RAPIER, "equip: weapon %d", G.weapon[0]);
    T_ASSERT(G.nwinv == 1 && G.winv[0] == R5W_GREATSWORD,
             "equip: pool not swapped honestly (n=%d w0=%d)", G.nwinv, G.winv[0]);
}

static void t_items_potion(void) {
    sim_reset();
    mk_party(CLS_FIGHTER, 1);
    party5[0].hp = 2;
    G.pm[0].hp = 2;
    script_keys("DOWN DOWN DOWN A A A B B");
    field_menu();
    T_ASSERT(G.potions == 1, "potion not consumed (x%d)", G.potions);
    T_ASSERT(party5[0].hp > 2, "potion healed nothing (hp %d)", party5[0].hp);
}

static void t_tactics_set(void) {
    sim_reset();
    mk_party(CLS_FIGHTER, 1);
    script_keys("DOWN DOWN DOWN DOWN A A DOWN A B B");
    field_menu();
    T_ASSERT(G.tactics[0] == TAC_WISELY, "tactics %d, want Wisely", G.tactics[0]);
}

/* ------------------------------------------------- battles (encounter.c) */

static void t_battle_auto_win(void) {
    sim_reset();
    mk_party(CLS_FIGHTER, 3);
    G_DEMO = 1;                    /* tactic AI drives; fast bars */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    EncSpawn es[2] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
    };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
    T_ASSERT(log_contains("enc result=WIN"), "no WIN line in log");
    T_ASSERT(G.pm[0].xp == 900 + 20, "xp %d, want 920", G.pm[0].xp);
    T_ASSERT(sim_level_up_choices_calls == 1, "level_up_choices calls %d",
             sim_level_up_choices_calls);
    T_ASSERT(sim_last_music == SONG_VICTORY, "music %d, want victory fanfare",
             sim_last_music);
}

/* manual battle: the warlock picks Eld.Blast off the real menu5 and it must
 * dispatch as a SPELL. Under the old colliding codes (10+spell), Eldritch
 * Blast (id 11 -> 21) dispatched as monk Flurry instead -- this test hangs
 * on the old code because the spell never fires. */
static int eb_done, eb_step;
static u32 eb_rng;
static u16 eb_gen(u32 frame) {
    if (!eb_done && log_contains("spell Eldritch Blast")) {
        eb_done = 1;
        G.tactics[0] = TAC_WISELY;      /* the NEXT turn goes to the tactic AI */
    }
    if (eb_done) {
        /* a party wipe restores the pre-battle snapshot -- including
         * tactics -- so keep re-pinning the AI or a retry would hand the
         * menus back to a driver that only knows how to End Turn */
        G.tactics[0] = TAC_WISELY;
        /* still inside the casting turn: a DOWN run pins the cursor to the
         * last row (End Turn) wherever it starts, then A ends the turn.
         * Once the AI owns the member, stray keys only skip message bars. */
        int ph = (int)(eb_step++ % 40);
        if (ph < 24) return (ph & 1) ? 0 : KEY_DOWN;
        if (ph == 24) return KEY_A;
        return 0;
    }
    /* Seeking phase: strict [DOWN, A] pairs with randomized idle gaps.
     * In a fresh root menu the pair lands exactly on row 1 = Eld.Blast; in
     * the follow-up target select it confirms a target; in the post-action
     * [Tactics, End Turn] menu it ends the turn. The RANDOM gap matters: a
     * fixed period phase-locks with the deterministic menu loop into a
     * closed orbit that never casts (observed as a root->Item->Potion
     * livelock). ORDERS is pinned so a stray Tactics pick can't hand the
     * member to the AI before the cast. */
    (void)frame;
    G.tactics[0] = TAC_ORDERS;
    int ph = eb_step++;
    if (ph == 0) return KEY_DOWN;
    if (ph == 2) return KEY_A;
    if (ph >= 4) {
        eb_rng ^= eb_rng << 13; eb_rng ^= eb_rng >> 17; eb_rng ^= eb_rng << 5;
        eb_step = -(int)(eb_rng % 9);   /* 0..8 extra idle frames, then wrap */
    }
    return 0;
}

static void t_battle_manual_eldritch_blast(void) {
    sim_reset();
    mk_party(CLS_WARLOCK, 2);
    party_add_laezel();
    G.tactics[1] = TAC_WISELY;     /* the escort fights herself; TAV is manual */
    G_DEMO = 1;
    G_MANUAL_BAT = 1;
    eb_done = 0; eb_step = 0; eb_rng = 0xEB0117u;
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 6, 0, 0, 0, 0);
    EncSpawn es[2] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
    };
    script_gen(eb_gen);
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    T_ASSERT(log_contains("spell Eldritch Blast"),
             "Eld.Blast menu row never dispatched as a spell cast");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
    T_ASSERT(eb_done, "generator never saw the cast");
}

/* ------------------------------------------------- party art (game.c) */

/* the custom-Tav walker per class: fighter/cleric are OBJT_HERO now --
 * a custom fighter wearing Lae'zel's sheet was the borrowed-faces bug */
static const int custom_walk[CLS_COUNT] = {
    OBJT_HERO, OBJT_HERO, OBJT_HERO, OBJT_HERO, OBJT_HERO, OBJT_HERO,
    OBJT_BARB, OBJT_DRUID, OBJT_MONK, OBJT_PALADIN, OBJT_SORC, OBJT_WARLOCK
};

static void t_member_look_identity(void) {
    sim_reset();
    /* custom Tav: class walker, Tav palette, no identity sheet */
    for (int cls = 0; cls < CLS_COUNT; cls++) {
        MemberLook L = member_look(ORIG_CUSTOM, cls);
        T_ASSERT(L.objt == custom_walk[cls],
                 "custom %s walker tile %d, want %d",
                 cls_display[cls], L.objt, custom_walk[cls]);
        T_ASSERT(L.pal == 0, "custom %s palette %d, want 0 (Tav slot)",
                 cls_display[cls], L.pal);
        T_ASSERT(L.objt != OBJT_LAEZEL && L.objt != OBJT_SHADOW,
                 "custom %s borrows a companion sheet", cls_display[cls]);
    }
    /* identity faces win over class */
    MemberLook lz = member_look(ORIG_LAEZEL, CLS_FIGHTER);
    T_ASSERT(lz.objt == OBJT_LAEZEL && lz.ko == OBJT_LAEZEL_KO && lz.pal == 1,
             "Lae'zel look wrong (objt %d ko %d pal %d)", lz.objt, lz.ko, lz.pal);
    T_ASSERT(lz.por == POR_LAEZEL, "Lae'zel portrait %d", lz.por);
    MemberLook sh = member_look(ORIG_SHADOW, CLS_CLERIC);
    T_ASSERT(sh.objt == OBJT_SHADOW && sh.ko == OBJT_SHADOW_KO && sh.pal == 2,
             "Shadowheart look wrong (objt %d ko %d pal %d)", sh.objt, sh.ko, sh.pal);
    /* origin faces keep the class sprite but take the origin portrait */
    MemberLook ga = member_look(ORIG_GALE, CLS_WIZARD);
    T_ASSERT(ga.objt == OBJT_HERO && ga.por == POR_GALE,
             "Gale look wrong (objt %d por %d)", ga.objt, ga.por);
    /* party_init wires member 0's face from the chosen origin */
    G.origin = ORIG_SHADOW;
    mk_party(CLS_CLERIC, 1);
    T_ASSERT(G.pm[0].face == ORIG_SHADOW, "party_init face %d", G.pm[0].face);
}

static void t_class_select_art(void) {
    for (int cls = 0; cls < CLS_COUNT; cls++) {
        sim_reset();
        char script[256] = "";
        for (int i = 0; i < cls; i++) strcat(script, "DOWN ");
        strcat(script, "A");
        script_keys(script);
        int r = game_class_select();
        T_ASSERT(r == cls, "class select returned %d, want %d", r, cls);
        const SimObj* hero = sim_obj(OBJ_PLAYER);
        T_ASSERT(hero->tile == custom_walk[cls],
                 "%s preview tile %d, want %d", cls_display[cls],
                 hero->tile, custom_walk[cls]);
        T_ASSERT(hero->pal == 0, "%s preview palette slot %d, want 0",
                 cls_display[cls], hero->pal);
        if (cls < 4)   /* bard..wizard load their tav palette into slot 0 */
            T_ASSERT(!memcmp((const void*)PAL_OBJ, pal_tav_classes[cls], 32),
                     "%s: OBJ palette 0 not loaded from tav set %d",
                     cls_display[cls], cls);
    }
}

/* ---------------------------------------------------------------- runner */

typedef struct { const char* name; void (*fn)(void); } Test;

static const Test tests[] = {
    { "battle_menu_kits",          t_battle_menu_kits },
    { "battle_menu_variants",      t_battle_menu_variants },
    { "member_sheet_per_class",    t_member_sheet_per_class },
    { "member_sheet_everburn",     t_member_sheet_everburn },
    { "prepare_screen_cleric",     t_prepare_screen_cleric },
    { "overview_party_of_three",   t_overview_party_of_three },
    { "equip_swap",                t_equip_swap },
    { "items_potion",              t_items_potion },
    { "tactics_set",               t_tactics_set },
    { "battle_auto_win",           t_battle_auto_win },
    { "battle_manual_eldritch_blast", t_battle_manual_eldritch_blast },
    { "member_look_identity",      t_member_look_identity },
    { "class_select_art",          t_class_select_art },
    { "menu_crawl_fuzz",           t_menu_crawl_fuzz },
};

int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : 0;
    int ran = 0, failed = 0;
    for (unsigned i = 0; i < sizeof tests / sizeof *tests; i++) {
        if (filter && !strstr(tests[i].name, filter)) continue;
        ran++;
        int rc = sim_guard(tests[i].fn);
        if (rc == 0 && sim_violations)
            { rc = 1; snprintf(sim_fail_msg, sizeof sim_fail_msg,
                               "engine invariant: %s", sim_violation_msg); }
        if (rc == 1) {
            failed++;
            printf("FAIL %s: %s\n", tests[i].name, sim_fail_msg);
            if (getenv("SIM_VERBOSE")) grid_dump();   /* what was on screen */
        } else {
            printf("ok %s\n", tests[i].name);
        }
        fflush(stdout);
    }
    printf("%d/%d passed\n", ran - failed, ran);
    return failed ? 1 : 0;
}
