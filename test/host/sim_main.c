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
void ability_audit(void);     /* encounter.c: per-class menu dump */
void sheet_audit(void);       /* menu.c: per-class sheet-spell dump */
void game_title(void);        /* game.c title/jukebox/karaoke flows */
void game_crawl(void);
int  game_origin_choose(int cls);
void game_name_entry(char* out);
int  origin_class(int o);
int  origin_portrait(int o);
const char* origin_name(int o);

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
 *     Action Surge is fighter L2+ (once per rest, free -- neither the
 *     action nor the bonus action); Hunter's Mark needs ranger slots
 *     (L2+); warlocks cast leveled spells on pact slots (present from
 *     L1); everyone gets Attack/Item/Dodge/Tactics/End Turn.
 * Codes: fixed actions 0..9, class features 20..26, spells SPELL_CODE(100)
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
static const Row kit_fighter1[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 }, { "2nd Wind", 4 },
    { "Tactics", 9 }, { "End Turn", 7 },
};
static const Row kit_fighter2[] = {
    { "Attack", 0 }, { "Item", 1 }, { "Dodge", 2 }, { "2nd Wind", 4 },
    { "ActSurge", 26 },
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
    [CLS_FIGHTER]   = { KIT(kit_fighter1), KIT(kit_fighter2), KIT(kit_fighter2) },
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

    /* a fighter who already surged this rest gets no second ActSurge */
    sim_reset(); mk_party(CLS_FIGHTER, 2);
    r5_use_action_surge(&party5[0]);
    n = build_root_menu(items, code);
    for (int i = 0; i < n; i++)
        T_ASSERT(strcmp(items[i], "ActSurge"),
                 "spent fighter still offers ActSurge");

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
    party_add_astarion();          /* a live reserve: the Party screen gets */
    party_add_gale();              /* fuzzed along with everything else    */
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

/* ------------------------------------------------- roster + reserve swap
 * Oracles are the SRD sheets, independent of party5.c: rogue d8 CON 13
 * (L1 9 hp, L2 15), wizard d6 CON 14 (L1 8 hp, L2 14, slots 2/3),
 * fighter d10 CON 14 (L1 12), cleric d8 CON 14 (L1 10, 2 slots). */

static void mk_roster5(int cls, int lvl) {
    mk_party(cls, lvl);
    party_add_laezel();
    party_add_shadowheart();
    party_add_astarion();
    party_add_gale();
}

static void t_roster_recruit_overflow(void) {
    sim_reset();
    mk_party(CLS_BARD, 2);             /* an L2 party: recruits must catch up */
    party_add_laezel();
    party_add_shadowheart();
    T_ASSERT(G.nparty == 3 && G.nreserve == 0,
             "walking fill: nparty %d nreserve %d", G.nparty, G.nreserve);
    party_add_astarion();
    T_ASSERT(G.nparty == 3, "a 4th soul grew the walking party to %d", G.nparty);
    T_ASSERT(G.nreserve == 1, "Astarion not benched (nreserve %d)", G.nreserve);
    party_add_gale();
    T_ASSERT(G.nreserve == 2, "Gale not benched (nreserve %d)", G.nreserve);
    T_ASSERT(!strcmp(G.reserve[0].name, "ASTAR.") && G.reserve[0].cls == CLS_ROGUE
             && G.reserve[0].face == ORIG_ASTARION,
             "reserve 0 is %s cls %d face %d", G.reserve[0].name,
             G.reserve[0].cls, G.reserve[0].face);
    T_ASSERT(!strcmp(G.reserve[1].name, "GALE") && G.reserve[1].cls == CLS_WIZARD
             && G.reserve[1].face == ORIG_GALE,
             "reserve 1 is %s cls %d face %d", G.reserve[1].name,
             G.reserve[1].cls, G.reserve[1].face);
    T_ASSERT(G.reserve[0].level == 2 && G.reserve[1].level == 2,
             "recruits joined at Lv %d/%d, want the party's 2",
             G.reserve[0].level, G.reserve[1].level);
    T_ASSERT(bench5[0].hp == 15 && bench5[0].hpmax == 15,
             "benched rogue L2 hp %d/%d, want 15/15", bench5[0].hp, bench5[0].hpmax);
    T_ASSERT(bench5[1].hp == 14 && bench5[1].hpmax == 14,
             "benched wizard L2 hp %d/%d, want 14/14", bench5[1].hp, bench5[1].hpmax);
    T_ASSERT(bench5[1].slots[0] == 3, "benched wizard L2 slots %d, want 3",
             bench5[1].slots[0]);
    T_ASSERT(G.rweapon[0] == R5W_DAGGER && G.rweapon[1] == R5W_QUARTERSTAFF,
             "bench weapons %d/%d", G.rweapon[0], G.rweapon[1]);
    party_add_gale();                  /* a 6th soul has nowhere to land */
    T_ASSERT(G.nparty == 3 && G.nreserve == 2,
             "roster overflowed its cap (nparty %d nreserve %d)",
             G.nparty, G.nreserve);
}

static void t_party_swap_menu(void) {
    sim_reset();
    mk_roster5(CLS_BARD, 1);
    party5[1].hp -= 3;                 /* Lae'zel carries a wound to the bench */
    /* Party is row 5 of 7; inside, UP must NOT walk the cursor onto Tav */
    script_keys("DOWN DOWN DOWN DOWN DOWN A UP A A B B");
    field_menu();
    T_ASSERT(!strcmp(G.pm[0].name, "TAV"), "Tav left slot 0 (%s)", G.pm[0].name);
    T_ASSERT(!strcmp(G.pm[1].name, "ASTAR.") && G.pm[1].cls == CLS_ROGUE,
             "slot 1 holds %s cls %d", G.pm[1].name, G.pm[1].cls);
    T_ASSERT(!strcmp(G.reserve[0].name, "LAE'ZEL") && G.reserve[0].cls == CLS_FIGHTER,
             "reserve 0 holds %s cls %d", G.reserve[0].name, G.reserve[0].cls);
    /* the newcomer's creature is HIS sheet: full rogue, not her wounds */
    T_ASSERT(!strcmp(party5[1].name, "ASTAR.") && party5[1].cls == CLS_ROGUE,
             "party5[1] is %s cls %d", party5[1].name, party5[1].cls);
    T_ASSERT(party5[1].hp == 9 && party5[1].hpmax == 9,
             "swapped-in rogue hp %d/%d, want 9/9", party5[1].hp, party5[1].hpmax);
    /* canon blood: Astarion is a high elf (+2 DEX +1 INT on the rogue spread) */
    T_ASSERT(party5[1].ac == 14, "rogue AC %d, want 14 (leather + DEX 17)",
             party5[1].ac);
    T_ASSERT(party5[1].ab[R5_DEX] == 17 && party5[1].ab[R5_INT] == 15,
             "rogue spread DEX %d INT %d, want 17/15 (high elf ASIs)",
             party5[1].ab[R5_DEX], party5[1].ab[R5_INT]);
    /* her wound went with her */
    T_ASSERT(bench5[0].cls == CLS_FIGHTER, "bench5[0] cls %d", bench5[0].cls);
    T_ASSERT(bench5[0].hpmax == 12 && bench5[0].hp == 9,
             "benched fighter hp %d/%d, want the wound kept (9/12)",
             bench5[0].hp, bench5[0].hpmax);
    /* equipment travels with its owner */
    T_ASSERT(G.weapon[1] == R5W_DAGGER, "slot-1 weapon %d, want his dagger",
             G.weapon[1]);
    T_ASSERT(G.rweapon[0] == R5W_GREATSWORD, "bench weapon %d, want her greatsword",
             G.rweapon[0]);
}

static void t_bench_round_trip(void) {
    sim_reset();
    mk_roster5(CLS_BARD, 1);
    /* wound Lae'zel and spend her Second Wind; down Shadowheart and spend
     * one of her two cleric slots */
    party5[1].hp -= 2;
    party5[1].used |= USED_SECOND_WIND;
    T_ASSERT(r5_spend_slot(&party5[2], 1), "cleric slot spend failed");
    party5[2].hp = 0;
    party5[2].conds |= C_UNCONSCIOUS;
    int lz_max = party5[1].hpmax, sh_max = party5[2].hpmax;
    /* one Party session: bench both walkers, then bring both home */
    script_keys("DOWN DOWN DOWN DOWN DOWN A "
                "A A "                /* slot 1 <-> Astarion       */
                "DOWN A DOWN A "      /* slot 2 <-> Gale           */
                "A A "                /* slot 1 <-> Lae'zel (back) */
                "DOWN A DOWN A "      /* slot 2 <-> Shadow. (back) */
                "B B");
    field_menu();
    T_ASSERT(!strcmp(G.pm[1].name, "LAE'ZEL") && !strcmp(G.pm[2].name, "SHADOW."),
             "round trip lost the walkers (%s, %s)", G.pm[1].name, G.pm[2].name);
    /* everything spent stayed spent, everything lost stayed lost */
    T_ASSERT(party5[1].hp == lz_max - 2, "Lae'zel hp %d, want %d",
             party5[1].hp, lz_max - 2);
    T_ASSERT(party5[1].used & USED_SECOND_WIND, "the bench refunded Second Wind");
    T_ASSERT(party5[2].hp == 0, "Shadowheart hp %d, want still 0", party5[2].hp);
    T_ASSERT(party5[2].conds & C_UNCONSCIOUS, "the bench woke Shadowheart");
    T_ASSERT(party5[2].slots[0] == 1, "cleric slots %d, want 1 (one spent)",
             party5[2].slots[0]);
    T_ASSERT(party5[2].hpmax == sh_max, "hpmax drifted %d -> %d",
             sh_max, party5[2].hpmax);
    /* the tourists came home clean */
    T_ASSERT(!strcmp(G.reserve[0].name, "ASTAR.") && !strcmp(G.reserve[1].name, "GALE"),
             "reserve order lost (%s, %s)", G.reserve[0].name, G.reserve[1].name);
    T_ASSERT(bench5[0].hp == bench5[0].hpmax, "Astarion took wounds while touring");
    T_ASSERT(bench5[1].slots[0] == 2, "Gale slots %d, want 2 (SRD wizard L1)",
             bench5[1].slots[0]);
    /* a spent pact pool survives the bench too: respec the sheet to a
     * warlock (the rebuild party5_refresh is the respec path), cast, tour */
    G.pm[1].cls = CLS_WARLOCK;
    party5_refresh(1);
    T_ASSERT(party5[1].rsrc[R5R_PACT] == 1, "warlock L1 pact pool %d",
             party5[1].rsrc[R5R_PACT]);
    T_ASSERT(r5_pact_cast(&party5[1]), "pact cast failed");
    party_swap(1, 0);
    party_swap(1, 0);
    T_ASSERT(G.pm[1].cls == CLS_WARLOCK && party5[1].rsrc[R5R_PACT] == 0,
             "the bench refunded the pact slot (cls %d rsrc %d)",
             G.pm[1].cls, party5[1].rsrc[R5R_PACT]);
}

static void t_rest_heals_bench(void) {
    sim_reset();
    mk_roster5(CLS_BARD, 1);
    /* Astarion benched at death's door, Gale benched dry of slots */
    bench5[0].hp = 0;
    bench5[0].conds |= C_UNCONSCIOUS;
    G.reserve[0].hp = 1;
    T_ASSERT(r5_spend_slot(&bench5[1], 1), "wizard slot spend failed");
    party_heal_full();
    T_ASSERT(bench5[0].hp == bench5[0].hpmax && bench5[0].hpmax > 0,
             "benched rogue hp %d/%d after the rest", bench5[0].hp, bench5[0].hpmax);
    T_ASSERT(!(bench5[0].conds & C_UNCONSCIOUS), "benched rogue still KO after rest");
    T_ASSERT(bench5[1].slots[0] == 2, "benched wizard slots %d, want 2 back",
             bench5[1].slots[0]);
    T_ASSERT(G.reserve[0].hp == G.reserve[0].hpmax, "reserve sheet hp %d/%d",
             G.reserve[0].hp, G.reserve[0].hpmax);
}

static void t_crash_scatter_regather(void) {
    /* The beach arc opens on party_scatter: the crash strands Tav alone,
     * whole (a narrative long rest), ship flags intact -- then the beach
     * recoveries re-add the scattered through the same party_add_* doors. */
    sim_reset();
    mk_roster5(CLS_BARD, 2);
    G.flags |= GF_LAEZEL | GF_SH_FREED | GF_ZHALK_DEAD;
    G.everburn = 1;
    party5[0].hp = 1;                          /* Tav limps off the helm */
    T_ASSERT(r5_spend_slot(&party5[0], 1), "bard slot spend failed");
    u16 flags = G.flags, xp0 = G.pm[0].xp;
    party_scatter();
    T_ASSERT(G.nparty == 1, "nparty %d after the crash, want 1", G.nparty);
    T_ASSERT(G.nreserve == 0, "nreserve %d after the crash, want 0", G.nreserve);
    T_ASSERT(G.flags == flags && G.everburn == 1,
             "the crash lost ship flags (%04x -> %04x, eb %d)",
             flags, G.flags, G.everburn);
    T_ASSERT(G.pm[0].hp == G.pm[0].hpmax, "sheet hp %d/%d after the long rest",
             G.pm[0].hp, G.pm[0].hpmax);
    T_ASSERT(party5[0].hp == party5[0].hpmax, "twin hp %d/%d after the long rest",
             party5[0].hp, party5[0].hpmax);
    T_ASSERT(party5[0].slots[0] == 3, "bard L2 slots %d, want 3 back (SRD)",
             party5[0].slots[0]);
    /* the beach recoveries: both walk back in at Tav's xp, twins rebuilt */
    party_add_shadowheart();
    party_add_laezel();
    T_ASSERT(G.nparty == 3, "nparty %d after both recoveries, want 3", G.nparty);
    T_ASSERT(!strcmp(G.pm[1].name, "SHADOW.") && !strcmp(G.pm[2].name, "LAE'ZEL"),
             "recovery order (%s, %s)", G.pm[1].name, G.pm[2].name);
    T_ASSERT(G.pm[1].xp == xp0 && G.pm[1].level == G.pm[0].level,
             "Shadowheart rejoined at xp %d L%d, want %d L%d",
             G.pm[1].xp, G.pm[1].level, xp0, G.pm[0].level);
    T_ASSERT(party5[2].hp == party5[2].hpmax && party5[2].hpmax > 0,
             "Lae'zel's twin not rebuilt (%d/%d)", party5[2].hp, party5[2].hpmax);
}

/* Companions auto-take their canon subclass (character2.md's origin table)
 * the moment their class reveals it -- SRD 5.1 timing: cleric at 1,
 * wizard at 2, fighter/rogue at 3. level_up_choices is a fake in this
 * suite, so passing proves the REAL leveling path resolves them; the
 * played hero (slot 0) is never auto-spec'd. */
static void t_companion_subclass_reveal(void) {
    sim_reset();
    mk_party(CLS_BARD, 1);
    party_add_laezel();
    party_add_shadowheart();
    T_ASSERT(G.pm[1].subclass == 255,
             "L1 Lae'zel subclass %d, want 255 (fighter reveals at 3)",
             G.pm[1].subclass);
    T_ASSERT(G.pm[2].subclass == R5SUB_DOMAIN_OF_MASKS,
             "L1 Shadowheart subclass %d, want Masks %d (cleric reveals at 1)",
             G.pm[2].subclass, R5SUB_DOMAIN_OF_MASKS);
    char nm[48];                   /* the contract: one name per up (6 here) */
    party_give_xp(900, nm);        /* the whole walking party hits L3... */
    party5_refresh_all();          /* ...and battle-end refreshes the twins */
    T_ASSERT(G.pm[1].level == 3, "Lae'zel L%d, want 3", G.pm[1].level);
    T_ASSERT(G.pm[1].subclass == R5SUB_CHAMPION,
             "L3 Lae'zel subclass %d, want Champion %d",
             G.pm[1].subclass, R5SUB_CHAMPION);
    T_ASSERT(party5[1].crit_min == 19,
             "Champion twin crit_min %d, want 19 (Improved Critical)",
             party5[1].crit_min);
    T_ASSERT(G.pm[0].subclass == 255,
             "leveling auto-spec'd Tav to %d -- the pick is the player's",
             G.pm[0].subclass);
}

/* The bench path: a recruit already past the reveal takes canon during
 * party_add's level_ups catch-up; one below it stays 255 on the sheet. */
static void t_companion_subclass_bench(void) {
    sim_reset();
    mk_party(CLS_BARD, 1);         /* an L1 party: nothing revealed yet */
    party_add_laezel();
    party_add_shadowheart();
    party_add_astarion();
    party_add_gale();
    T_ASSERT(G.reserve[0].subclass == 255 && G.reserve[1].subclass == 255,
             "L1 bench auto-spec'd (%d, %d), want 255/255 (reveals at 3/2)",
             G.reserve[0].subclass, G.reserve[1].subclass);
    sim_reset();
    mk_party(CLS_BARD, 3);         /* the party is L3 before anyone joins */
    party_add_laezel();
    party_add_shadowheart();
    party_add_astarion();
    party_add_gale();
    T_ASSERT(G.reserve[0].level == 3 && G.reserve[1].level == 3,
             "bench catch-up gave L%d/L%d, want 3/3",
             G.reserve[0].level, G.reserve[1].level);
    T_ASSERT(G.pm[1].subclass == R5SUB_CHAMPION,
             "walking Lae'zel past the reveal has %d, want Champion %d",
             G.pm[1].subclass, R5SUB_CHAMPION);
    T_ASSERT(G.reserve[0].subclass == R5SUB_THE_AMBUSH_ARTIST,
             "benched Astarion has %d, want Ambush %d (the assassin-shape)",
             G.reserve[0].subclass, R5SUB_THE_AMBUSH_ARTIST);
    T_ASSERT(G.reserve[1].subclass == R5SUB_EVOCATION,
             "benched Gale has %d, want Evocation %d",
             G.reserve[1].subclass, R5SUB_EVOCATION);
    T_ASSERT(bench5[0].crit_min == 19,
             "Ambush twin crit_min %d, want 19 (canon precedes the bench build)",
             bench5[0].crit_min);
}

static void t_party_row_gated(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    party_add_laezel();
    party_add_shadowheart();
    script_keys("SNAP B");
    field_menu();
    /* ship state: the five verbs + Close, exactly where they always were */
    T_ASSERT(!snap_contains(0, "Party"), "Party row shown with an empty reserve");
    T_ASSERT(strstr(snap_row(0, 15), "Close") != 0, "Close moved off row 15");
    party_add_astarion();
    script_keys("SNAP B");
    field_menu();
    T_ASSERT(snap_contains(1, "Party"), "Party row missing with a reserve");
    T_ASSERT(strstr(snap_row(1, 15), "Party") != 0, "Party not on row 15");
    T_ASSERT(strstr(snap_row(1, 17), "Close") != 0, "Close not on row 17");
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

/* ------------------------------------------------- battle internals
 * The WISELY tactic brain is the real dispatcher for the spell-cast
 * family (cast_sleep / cast_missiles / cast_heal / cast_save_spell):
 * stack the deck -- composition, tactics, a downed companion -- and the
 * brain must cast. The ROM scenarios keep pinning structure end-to-end;
 * these assert the BEHAVIOR (the cast fired, the downed woke, the ally
 * fought) natively under ASan/UBSan. */

static void t_battle_wizard_sleep(void) {
    sim_reset();
    mk_party(CLS_WIZARD, 1);
    party_add_laezel();
    G_DEMO = 1;
    /* four thralls: even after Lae'zel's opener drops one, three stand --
     * the WISELY sleep gate is side_up(1) >= 3 at the wizard's turn */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    int n2 = field_add_npc(10, 6, 0, 0, 0, 0);
    int n3 = field_add_npc(12, 6, 0, 0, 0, 0);
    EncSpawn es[4] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
        { R5M_THRALL, (s8)n2, 10, 1 },
        { R5M_THRALL, (s8)n3, 10, 1 },
    };
    sim_budget(2000000, 0);
    /* surprise=1: enemies lose round 1 (the 5e surprise branch) */
    int r = encounter_run(es, 4, 0, 1);
    T_ASSERT(log_contains("sleep pool="),
             "3+ foes up + a slot: WISELY wizard never cast Sleep");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_wizard_missiles(void) {
    sim_reset();
    mk_party(CLS_WIZARD, 1);
    party_add_laezel();
    G_DEMO = 1;
    G_MANUAL_BAT = 1;              /* manual mode reads G.tactics... */
    G.tactics[0] = TAC_ALLOUT;     /* ...ALLOUT spends slots on missiles */
    G.tactics[1] = TAC_WISELY;     /* (2 foes only, so Sleep never outbids) */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    EncSpawn es[2] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
    };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    T_ASSERT(log_contains("missile "),
             "ALLOUT wizard with slots never cast Magic Missile");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_cleric_rescue(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    party_add_laezel();
    party5[1].hp = 0;                        /* she went down before the fight */
    party5[1].conds |= C_UNCONSCIOUS;
    G_DEMO = 1;
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    EncSpawn es[2] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
    };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    T_ASSERT(log_contains("heal LAE'ZEL"),
             "WISELY cleric never Cure Wounds'd the downed companion");
    T_ASSERT(log_contains("spell Guiding Bolt"),
             "cleric with a slot left never cast Guiding Bolt");
    T_ASSERT(party5[1].hp > 0, "Lae'zel still at %d hp", party5[1].hp);
    T_ASSERT(!(party5[1].conds & C_UNCONSCIOUS), "Lae'zel still unconscious");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_bard_rescue(void) {
    sim_reset();
    mk_party(CLS_BARD, 1);
    party_add_laezel();
    party5[1].hp = 0;
    party5[1].conds |= C_UNCONSCIOUS;
    G_DEMO = 1;
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    EncSpawn es[1] = { { R5M_THRALL, (s8)n0, 10, 1 } };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 1, 0, 0);
    T_ASSERT(log_contains("heal LAE'ZEL"),
             "WISELY bard never Healing Word'd the downed companion");
    /* thralls are psychic-immune, so the save-spell resolves to 0 damage --
     * the cast + the IMMUNE branch of deal() are the coverage, not the kill */
    T_ASSERT(log_contains("save-spell Vicious Mockery"),
             "bard never cast Vicious Mockery");
    T_ASSERT(party5[1].hp > 0, "Lae'zel still at %d hp", party5[1].hp);
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_helm_allies(void) {
    sim_reset();
    mk_party(CLS_FIGHTER, 1);
    party_add_laezel();
    G_DEMO = 1;                    /* demo hero seizes the nerves on round 2 */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(12, 4, 0, 0, 0, 0);
    EncSpawn es[2] = {
        { R5M_CAMBION, (s8)n0, 450, 1 },     /* tough enough to outlive round 1 */
        { R5M_FLAYER,  (s8)n1, 0,   2 },     /* the helm ally fights beside us */
    };
    sim_budget(2000000, 0);
    /* helm_rounds=10: after round 1 the countdown hits 9 and a cambion warps */
    int r = encounter_run(es, 2, 10, 0);
    T_ASSERT(log_contains("atk Flayer"), "the ally flayer never took a turn");
    T_ASSERT(log_contains("cambion warps in"), "the countdown never warped one in");
    T_ASSERT(r == ENC_CONNECTED, "battle result %d, want ENC_CONNECTED", r);
}

/* a companion downed BEFORE the fight takes no turns at 0 hp. The turn
 * loop logs "turn rN NAME sideS hp=H" only when a turn is actually taken
 * and hp clamps at 0, so "LAE'ZEL side0 hp=0" is the exact zombie
 * signature; once the WISELY cleric's heal wakes her, she must act. */
static void t_battle_no_zombie_turns(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    party_add_laezel();
    party5[1].hp = 0;                    /* downed in a previous encounter */
    party5[1].conds |= C_UNCONSCIOUS;
    G_DEMO = 1;
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    EncSpawn es[2] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
    };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    T_ASSERT(!log_contains("LAE'ZEL side0 hp=0"),
             "a downed companion took a turn at 0 hp (zombie turn)");
    T_ASSERT(log_contains("heal LAE'ZEL"), "the cleric never rescued her");
    T_ASSERT(log_contains("LAE'ZEL side0 hp="),
             "the healed companion never took a live turn");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

/* helm-style fight (helm_rounds > 0) entered with Tav already down: under
 * G_DEMO a conscious companion seizes the nerves on the same round-2
 * timing. Without the fallback the countdown wipes, the retry restores
 * the same downed Tav, and the demo softlocks forever. Manual play keeps
 * the Tav-must-stand rule -- that is design, and t_battle_menu_variants
 * pins the Nerve! row's gate. */
static void t_battle_helm_tav_down(void) {
    sim_reset();
    mk_party(CLS_ROGUE, 3);              /* L3: she outlives cambion rounds */
    party_add_laezel();
    party5[0].hp = 0;                    /* Tav went down a battle ago */
    party5[0].conds |= C_UNCONSCIOUS;
    G_DEMO = 1;
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    EncSpawn es[1] = { { R5M_CAMBION, (s8)n0, 450, 1 } };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 1, 10, 0);
    T_ASSERT(!log_contains("TAV side0 hp=0"), "downed Tav took a zombie turn");
    T_ASSERT(log_contains("nerve seized pi=1"),
             "no conscious companion seized the nerves for downed Tav");
    T_ASSERT(r == ENC_CONNECTED, "battle result %d, want ENC_CONNECTED", r);
}

/* wild shape is a menu row (code 25), so this one drives the real menu:
 * [DOWN, A] in a fresh root menu lands on row 1 = WildShape (druid L2 kit),
 * then the eb_gen handoff pattern gives the boar to the tactic AI */
static int ws_step, ws_done;
static u32 ws_rng;
static u16 ws_gen(u32 frame) {
    (void)frame;
    if (!ws_done && log_contains("wildshape boar")) ws_done = 1;
    if (ws_done) {
        G.tactics[0] = TAC_WISELY;      /* re-pin: a wipe-retry restores G */
        int ph = (int)(ws_step++ % 40);
        if (ph < 24) return (ph & 1) ? 0 : KEY_DOWN;
        if (ph == 24) return KEY_A;
        return 0;
    }
    G.tactics[0] = TAC_ORDERS;
    int ph = ws_step++;
    if (ph == 0) return KEY_DOWN;
    if (ph == 2) return KEY_A;
    if (ph >= 4) {
        ws_rng ^= ws_rng << 13; ws_rng ^= ws_rng >> 17; ws_rng ^= ws_rng << 5;
        ws_step = -(int)(ws_rng % 9);
    }
    return 0;
}

static void t_battle_druid_wildshape(void) {
    sim_reset();
    mk_party(CLS_DRUID, 2);            /* L2: the shape pool opens */
    party_add_laezel();
    G_DEMO = 1;
    G_MANUAL_BAT = 1;
    G.tactics[1] = TAC_WISELY;
    ws_step = 0; ws_done = 0; ws_rng = 0xB0A12u;
    /* three thralls: the fight must outlive the menu-seek phase, or
     * Lae'zel ends it before the WildShape row ever gets picked */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    int n2 = field_add_npc(10, 6, 0, 0, 0, 0);
    EncSpawn es[3] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
        { R5M_THRALL, (s8)n2, 10, 1 },
    };
    script_gen(ws_gen);
    sim_budget(2000000, 0);
    int r = encounter_run(es, 3, 0, 0);
    T_ASSERT(log_contains("wildshape boar"), "the WildShape row never fired");
    T_ASSERT(log_contains("wildshape revert"),
             "the druid never stepped back out of the beast");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
    T_ASSERT(!party5[0].conds, "druid conds %04x after revert", party5[0].conds);
}

/* manual Attack: [A, A] = Attack row, then pick5 confirms the first target.
 * The frames spent inside pick5 run the sneak/provoke advisors (sneak_ok)
 * with a live rogue -- the pip logic the ROM can only screenshot. */
static int rp_step, rp_done;
static u32 rp_rng;
static u16 rp_gen(u32 frame) {
    (void)frame;
    if (!rp_done && log_contains("atk TAV")) rp_done = 1;
    if (rp_done) {
        G.tactics[0] = TAC_WISELY;
        int ph = (int)(rp_step++ % 40);
        if (ph < 24) return (ph & 1) ? 0 : KEY_DOWN;
        if (ph == 24) return KEY_A;
        return 0;
    }
    G.tactics[0] = TAC_ORDERS;
    int ph = rp_step++;
    if (ph == 0 || ph == 2) return KEY_A;
    if (ph >= 4) {
        rp_rng ^= rp_rng << 13; rp_rng ^= rp_rng >> 17; rp_rng ^= rp_rng << 5;
        rp_step = -(int)(rp_rng % 9);
    }
    return 0;
}

static void t_battle_rogue_pick(void) {
    sim_reset();
    mk_party(CLS_ROGUE, 1);
    party_add_laezel();
    G_DEMO = 1;
    G_MANUAL_BAT = 1;
    G.tactics[1] = TAC_WISELY;
    rp_step = 0; rp_done = 0; rp_rng = 0x5EAC7u;
    /* three thralls, same reason as the wildshape fight above */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    int n2 = field_add_npc(10, 6, 0, 0, 0, 0);
    EncSpawn es[3] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
        { R5M_THRALL, (s8)n2, 10, 1 },
    };
    script_gen(rp_gen);
    sim_budget(2000000, 0);
    int r = encounter_run(es, 3, 0, 0);
    T_ASSERT(log_contains("atk TAV"), "the rogue never attacked off the menu");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_ranger_conc(void) {
    sim_reset();
    mk_party(CLS_RANGER, 2);           /* L2: first slots, Hunter's Mark */
    party_add_laezel();
    G_DEMO = 1;
    /* five thralls: the mark lands on the ranger's first turn, and the
     * fight must run long enough for hits to land on him AFTER it */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    int n2 = field_add_npc(10, 6, 0, 0, 0, 0);
    int n3 = field_add_npc(12, 6, 0, 0, 0, 0);
    int n4 = field_add_npc(8, 6, 0, 0, 0, 0);
    EncSpawn es[5] = {
        { R5M_THRALL, (s8)n0, 10, 1 },
        { R5M_THRALL, (s8)n1, 10, 1 },
        { R5M_THRALL, (s8)n2, 10, 1 },
        { R5M_THRALL, (s8)n3, 10, 1 },
        { R5M_THRALL, (s8)n4, 10, 1 },
    };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 5, 0, 0);
    /* the WISELY ranger marks; three thralls' worth of hits forces CON
     * saves while concentrating, and this seed breaks at least one */
    T_ASSERT(log_contains("conc save"),
             "the marked ranger never took damage while concentrating");
    T_ASSERT(log_contains("broken"), "no concentration save ever failed");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

/* ------------------------------------------------- the dark (stone 4)
 * The darkvision doctrine (docs/character2.md), pinned at the attack-flag
 * choke point through REAL battles: in a DARK encounter, actors WITHOUT
 * TR_DARKVISION roll ranged and spell attacks at disadvantage -- melee
 * exempt, per actor, both sides; devils and the crypt's dead see fine; the
 * equipped Everburn Blade lights the room outright. Expectations are
 * written against the doctrine, not the code. */

static void t_battle_dark_caster_disadvantage(void) {
    sim_reset();
    mk_party(CLS_WIZARD, 2);           /* race none: no darkvision */
    party_add_laezel();                /* melee escort: her swings stay clean */
    G_DEMO = 1;
    encounter_set_dark(1);
    T_ASSERT(encounter_dark(), "DARK room reads lit with no Everburn equipped");
    /* the SRD skeleton: darkvision 60, and bludgeoning vulnerability rides
     * the stat block into the creature */
    T_ASSERT(r5_monsters[R5M_SKELETON].vulnerable == (1u << DT_BLUDGEONING),
             "skeleton lost its bludgeoning vulnerability");
    /* two skeletons: the escort's opener can't end round 1, so the wizard
     * always gets a cast -- and 2 foes stays under the WISELY Sleep gate */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    EncSpawn es[2] = { { R5M_SKELETON, (s8)n0, 30, 1 },
                       { R5M_SKELETON, (s8)n1, 30, 1 } };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    encounter_set_dark(0);
    T_ASSERT(log_contains("spell Fire Bolt"), "the wizard never cast in the dark");
    T_ASSERT(log_contains("dark dis TAV"),
             "no-darkvision caster's spell attack not disadvantaged in the dark");
    T_ASSERT(!log_contains("dark dis LAE'ZEL"),
             "a melee attacker was taxed by the dark (melee is exempt)");
    T_ASSERT(!log_contains("dark dis Skeleton"),
             "the darkvision skeleton was taxed by the dark");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_dark_darkvision_race_clean(void) {
    sim_reset();
    mk_party(CLS_WIZARD, 2);
    party_set_identity(R5RACE_HIGH_ELF, R5BG_SAGE, 0);   /* fey eyes */
    T_ASSERT(party5[0].traits & TR_DARKVISION, "high elf lost darkvision");
    party_add_laezel();
    G_DEMO = 1;
    encounter_set_dark(1);
    /* two skeletons: the escort's opener can't end round 1, so the wizard
     * always gets a cast -- and 2 foes stays under the WISELY Sleep gate */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    EncSpawn es[2] = { { R5M_SKELETON, (s8)n0, 30, 1 },
                       { R5M_SKELETON, (s8)n1, 30, 1 } };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    encounter_set_dark(0);
    T_ASSERT(log_contains("spell Fire Bolt"), "the elf wizard never cast");
    T_ASSERT(!log_contains("dark dis"),
             "a darkvision caster was taxed by the dark");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_dark_imp_exempt(void) {
    sim_reset();
    /* devils see 120 ft and are never blinded: the bit rides the senses
     * line of the stat block, human thralls stay blind (monster lore) */
    T_ASSERT(r5_monsters[R5M_IMP].traits & TR_DARKVISION,
             "the SRD imp lost devil's sight");
    T_ASSERT(r5_monsters[R5M_LESSER_IMP].traits & TR_DARKVISION,
             "the lesser imp lost devil's sight");
    T_ASSERT(r5_monsters[R5M_SKELETON].traits & TR_DARKVISION,
             "the skeleton lost its darkvision");
    T_ASSERT(!(r5_monsters[R5M_THRALL].traits & TR_DARKVISION),
             "the human thrall grew darkvision");
    mk_party(CLS_FIGHTER, 2);
    G_DEMO = 1;
    encounter_set_dark(1);
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    EncSpawn es[1] = { { R5M_LESSER_IMP, (s8)n0, 40, 1 } };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 1, 0, 0);
    encounter_set_dark(0);
    T_ASSERT(!log_contains("dark dis"),
             "someone was taxed in a fighter-vs-imp dark fight");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
}

static void t_battle_dark_everburn_lights(void) {
    sim_reset();
    mk_party(CLS_WIZARD, 2);
    party_add_laezel();
    G.weapon[1] = R5W_EVERBURN;          /* the trophy, equipped and burning */
    encounter_set_dark(1);
    T_ASSERT(!encounter_dark(), "the equipped Everburn failed to light the room");
    G_DEMO = 1;
    /* two skeletons: the escort's opener can't end round 1, so the wizard
     * always gets a cast -- and 2 foes stays under the WISELY Sleep gate */
    int n0 = field_add_npc(9, 5, 0, 0, 0, 0);
    int n1 = field_add_npc(11, 5, 0, 0, 0, 0);
    EncSpawn es[2] = { { R5M_SKELETON, (s8)n0, 30, 1 },
                       { R5M_SKELETON, (s8)n1, 30, 1 } };
    sim_budget(2000000, 0);
    int r = encounter_run(es, 2, 0, 0);
    T_ASSERT(log_contains("spell Fire Bolt"), "the wizard never cast");
    T_ASSERT(!log_contains("dark dis"),
             "an Everburn-lit room still taxed the caster");
    T_ASSERT(r == ENC_WIN, "battle result %d, want ENC_WIN", r);
    /* stow the blade and the dark closes back in */
    G.weapon[1] = (u8)party5_default_weapon(CLS_FIGHTER);
    T_ASSERT(encounter_dark(), "stowing the blade failed to re-darken the room");
    encounter_set_dark(0);
}

/* ------------------------------------------------- races + backgrounds
 * The compatibility law first: race 0 ("none") leaves every sheet exactly
 * classful -- the state every pre-Character-2.0 flow still runs in. Then
 * the mechanics: fixed 5.1 ASIs, Dwarven Toughness hp, resistances, trait
 * bits. Expected numbers are derived from the SRD by hand, per class
 * preset (the standard-array spreads the design blesses as presets). */

static const s8 preset_ab[CLS_COUNT][6] = {
    [CLS_BARD]      = { 8, 14, 13, 10, 12, 15 },
    [CLS_ROGUE]     = { 8, 15, 13, 14, 10, 12 },
    [CLS_RANGER]    = { 12, 15, 13, 8, 14, 10 },
    [CLS_WIZARD]    = { 8, 13, 14, 15, 12, 10 },
    [CLS_FIGHTER]   = { 15, 13, 14, 10, 12, 8 },
    [CLS_CLERIC]    = { 13, 10, 14, 8, 15, 12 },
    [CLS_BARBARIAN] = { 15, 13, 14, 8, 12, 10 },
    [CLS_DRUID]     = { 10, 13, 14, 8, 15, 12 },
    [CLS_MONK]      = { 10, 15, 13, 8, 14, 12 },
    [CLS_PALADIN]   = { 15, 10, 13, 8, 12, 14 },
    [CLS_SORCERER]  = { 8, 13, 14, 10, 12, 15 },
    [CLS_WARLOCK]   = { 8, 13, 14, 12, 10, 15 },
};

static void t_race_none_baseline(void) {
    for (int cls = 0; cls < CLS_COUNT; cls++) {
        sim_reset();
        mk_party(cls, 1);       /* sim_reset leaves G.origin = ORIG_CUSTOM */
        T_ASSERT(G.pm[0].race == R5RACE_NONE && G.pm[0].background == R5BG_NONE,
                 "%s: custom Tav booted with race %d bg %d",
                 cls_display[cls], G.pm[0].race, G.pm[0].background);
        for (int a = 0; a < 6; a++)
            T_ASSERT(party5[0].ab[a] == preset_ab[cls][a],
                     "%s ab[%d] %d, want the classful preset %d",
                     cls_display[cls], a, party5[0].ab[a], preset_ab[cls][a]);
        T_ASSERT(party5[0].traits == 0, "%s: race-none sheet carries traits %02x",
                 cls_display[cls], party5[0].traits);
        T_ASSERT(party5[0].resist == 0, "%s: race-none sheet resists %04x",
                 cls_display[cls], party5[0].resist);
        /* SRD hp at level 1: max hit die + CON mod, nothing racial */
        int want_hp = r5_classes[cls].hit_die + r5_mod(preset_ab[cls][R5_CON]);
        T_ASSERT(party5[0].hpmax == want_hp, "%s hpmax %d, want %d",
                 cls_display[cls], party5[0].hpmax, want_hp);
    }
}

static void t_race_mechanics_sheet(void) {
    /* half-orc fighter: STR 17 CON 15, the half-orc trait bits */
    sim_reset();
    mk_party(CLS_FIGHTER, 1);
    G.pm[0].race = R5RACE_HALF_ORC;
    party5_refresh(0);
    T_ASSERT(party5[0].ab[R5_STR] == 17 && party5[0].ab[R5_CON] == 15,
             "half-orc fighter STR %d CON %d, want 17/15",
             party5[0].ab[R5_STR], party5[0].ab[R5_CON]);
    T_ASSERT(party5[0].traits == (TR_DARKVISION | TR_RELENTLESS | TR_SAVAGE),
             "half-orc traits %02x", party5[0].traits);
    T_ASSERT(party5[0].hpmax == 12,          /* d10 + CON 15 (+2) */
             "half-orc fighter hpmax %d, want 12", party5[0].hpmax);

    /* hill-dwarf fighter: Toughness pays +1 per level, at 1 and at 3 */
    sim_reset();
    mk_party(CLS_FIGHTER, 1);
    G.pm[0].race = R5RACE_HILL_DWARF;
    party5_refresh(0);
    T_ASSERT(party5[0].hpmax == 14,          /* 10 + CON 16 (+3) + 1 */
             "hill-dwarf fighter L1 hpmax %d, want 14", party5[0].hpmax);
    T_ASSERT(party5[0].resist == (1u << DT_POISON),
             "dwarf resist %04x, want poison", party5[0].resist);
    T_ASSERT(party5[0].traits & TR_POISON_RESIL, "dwarf lost Resilience");
    sim_reset();
    mk_party(CLS_FIGHTER, 3);
    G.pm[0].race = R5RACE_HILL_DWARF;
    party5_refresh(0);
    T_ASSERT(party5[0].hpmax == 34,          /* 13 + 2x(6+3) + 3 toughness */
             "hill-dwarf fighter L3 hpmax %d, want 34", party5[0].hpmax);

    /* tiefling barbarian (Karlach's shape): fire resistance on the sheet */
    sim_reset();
    mk_party(CLS_BARBARIAN, 1);
    G.pm[0].race = R5RACE_TIEFLING;
    party5_refresh(0);
    T_ASSERT(party5[0].resist == (1u << DT_FIRE),
             "tiefling resist %04x, want fire", party5[0].resist);
    T_ASSERT(party5[0].ab[R5_CHA] == 12 && party5[0].ab[R5_INT] == 9,
             "tiefling barb CHA %d INT %d, want 12/9",
             party5[0].ab[R5_CHA], party5[0].ab[R5_INT]);

    /* lightfoot bard: Lucky lands where the dice core reads it */
    sim_reset();
    mk_party(CLS_BARD, 1);
    G.pm[0].race = R5RACE_LIGHTFOOT;
    party5_refresh(0);
    T_ASSERT(party5[0].traits == TR_LUCKY, "lightfoot traits %02x",
             party5[0].traits);

    /* human cleric: every score +1; WIS 16 lifts the spell DC to 13 */
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    G.pm[0].race = R5RACE_HUMAN;
    party5_refresh(0);
    T_ASSERT(party5_spell_dc(&party5[0]) == 13,
             "human cleric DC %d, want 13", party5_spell_dc(&party5[0]));

    /* high-elf rogue: DEX 17 raises leather AC to 14; refresh must be
     * idempotent (a second pass may not stack the ASI again) */
    sim_reset();
    mk_party(CLS_ROGUE, 1);
    G.pm[0].race = R5RACE_HIGH_ELF;
    party5_refresh(0);
    party5_refresh(0);
    T_ASSERT(party5[0].ab[R5_DEX] == 17, "high-elf rogue DEX %d, want 17",
             party5[0].ab[R5_DEX]);
    T_ASSERT(party5[0].ac == 14, "high-elf rogue AC %d, want 14", party5[0].ac);
}

static void t_origin_sheet_identities(void) {
    /* character2.md "Origin sheet identities", restated */
    static const struct { int origin, cls, race, bg; } ident[] = {
        { ORIG_ASTARION, CLS_ROGUE,     R5RACE_HIGH_ELF,  R5BG_CRIMINAL },
        { ORIG_GALE,     CLS_WIZARD,    R5RACE_HUMAN,     R5BG_SAGE },
        { ORIG_KARLACH,  CLS_BARBARIAN, R5RACE_TIEFLING,  R5BG_OUTLANDER },
        { ORIG_LAEZEL,   CLS_FIGHTER,   R5RACE_GITHYANKI, R5BG_SOLDIER },
        { ORIG_SHADOW,   CLS_CLERIC,    R5RACE_HALF_ELF,  R5BG_ACOLYTE },
        { ORIG_WYLL,     CLS_WARLOCK,   R5RACE_HUMAN,     R5BG_FOLK_HERO },
    };
    for (unsigned i = 0; i < sizeof ident / sizeof *ident; i++) {
        sim_reset();
        G.origin = (u8)ident[i].origin;
        mk_party(ident[i].cls, 1);
        T_ASSERT(G.pm[0].race == ident[i].race,
                 "origin %d race %d, want %d", ident[i].origin,
                 G.pm[0].race, ident[i].race);
        T_ASSERT(G.pm[0].background == ident[i].bg,
                 "origin %d bg %d, want %d", ident[i].origin,
                 G.pm[0].background, ident[i].bg);
    }
    /* spot the sheets: Lae'zel-as-hero swings STR 17 (gith +2)... */
    sim_reset();
    G.origin = ORIG_LAEZEL;
    mk_party(CLS_FIGHTER, 1);
    T_ASSERT(party5[0].ab[R5_STR] == 17, "origin Lae'zel STR %d, want 17",
             party5[0].ab[R5_STR]);
    /* ...Shadowheart heals off WIS 16 (half-elf pin) with fey ancestry... */
    sim_reset();
    G.origin = ORIG_SHADOW;
    mk_party(CLS_CLERIC, 1);
    T_ASSERT(party5[0].ab[R5_WIS] == 16 && party5_spell_dc(&party5[0]) == 13,
             "origin Shadowheart WIS %d DC %d, want 16/13",
             party5[0].ab[R5_WIS], party5_spell_dc(&party5[0]));
    T_ASSERT(party5[0].traits == (TR_DARKVISION | TR_FEY),
             "origin Shadowheart traits %02x", party5[0].traits);
    /* ...and Gale is +1 across the board (INT 16, CON 15) */
    sim_reset();
    G.origin = ORIG_GALE;
    mk_party(CLS_WIZARD, 1);
    T_ASSERT(party5[0].ab[R5_INT] == 16 && party5[0].ab[R5_CON] == 15,
             "origin Gale INT %d CON %d, want 16/15",
             party5[0].ab[R5_INT], party5[0].ab[R5_CON]);
    /* the Urge (like custom Tav) starts race-none: the pickers own its blood */
    sim_reset();
    G.origin = ORIG_DURGE;
    mk_party(CLS_SORCERER, 1);
    T_ASSERT(G.pm[0].race == R5RACE_NONE && G.pm[0].background == R5BG_NONE,
             "the Urge pre-picker race %d bg %d", G.pm[0].race, G.pm[0].background);
    /* companions carry canon identity in every flow */
    sim_reset();
    mk_party(CLS_BARD, 1);
    party_add_laezel();
    party_add_shadowheart();
    T_ASSERT(G.pm[1].race == R5RACE_GITHYANKI && party5[1].ab[R5_STR] == 17,
             "companion Lae'zel race %d STR %d", G.pm[1].race, party5[1].ab[R5_STR]);
    T_ASSERT(G.pm[2].race == R5RACE_HALF_ELF && party5[2].ab[R5_WIS] == 16,
             "companion Shadowheart race %d WIS %d", G.pm[2].race,
             party5[2].ab[R5_WIS]);
    T_ASSERT(party5[2].traits & TR_FEY, "companion Shadowheart lost fey ancestry");
}

/* character2.md's origin table, subclass column: one source (game.c's
 * origins[]) feeds origin-PLAYED creation and companion canon alike.
 * Classes revealing at level 1 wear it from creation; every later reveal
 * leaves the played hero's pick to the player. */
static void t_origin_subclass_single_source(void) {
    static const struct { int origin, cls, sub; } want[] = {
        { ORIG_ASTARION, CLS_ROGUE,     255 },              /* reveals at 3 */
        { ORIG_GALE,     CLS_WIZARD,    255 },              /* reveals at 2 */
        { ORIG_KARLACH,  CLS_BARBARIAN, 255 },              /* reveals at 3 */
        { ORIG_LAEZEL,   CLS_FIGHTER,   255 },              /* reveals at 3 */
        { ORIG_SHADOW,   CLS_CLERIC,    R5SUB_DOMAIN_OF_MASKS },  /* at 1 */
        { ORIG_WYLL,     CLS_WARLOCK,   R5SUB_FIEND },            /* at 1 */
    };
    for (unsigned i = 0; i < sizeof want / sizeof *want; i++) {
        sim_reset();
        G.origin = (u8)want[i].origin;
        mk_party(want[i].cls, 1);
        T_ASSERT(G.pm[0].subclass == want[i].sub,
                 "origin %d creation subclass %d, want %d",
                 want[i].origin, G.pm[0].subclass, want[i].sub);
    }
    /* played Gale reaching wizard's reveal keeps the pick UI: no auto-take */
    sim_reset();
    G.origin = ORIG_GALE;
    mk_party(CLS_WIZARD, 2);
    T_ASSERT(G.pm[0].level == 2 && G.pm[0].subclass == 255,
             "played Gale at L%d auto-took %d -- his soul IS the player's",
             G.pm[0].level, G.pm[0].subclass);
    /* the canon column itself, straight off the table (Karlach never
     * party_adds in the prologue; her Berserker identity still holds) */
    T_ASSERT(origin_subclass(ORIG_ASTARION) == R5SUB_THE_AMBUSH_ARTIST
             && origin_subclass(ORIG_GALE) == R5SUB_EVOCATION
             && origin_subclass(ORIG_KARLACH) == R5SUB_BERSERKER
             && origin_subclass(ORIG_LAEZEL) == R5SUB_CHAMPION,
             "canon column %d/%d/%d/%d, want Ambush/Evocation/Berserker/Champion",
             origin_subclass(ORIG_ASTARION), origin_subclass(ORIG_GALE),
             origin_subclass(ORIG_KARLACH), origin_subclass(ORIG_LAEZEL));
    T_ASSERT(origin_subclass(ORIG_DURGE) == 255
             && origin_subclass(ORIG_CUSTOM) == 255,
             "the Urge/custom canon %d/%d, want 255/255 (their pick to make)",
             origin_subclass(ORIG_DURGE), origin_subclass(ORIG_CUSTOM));
}

static void t_background_skill_union(void) {
    /* Astarion: rogue picks (Stealth+Sleight) + Criminal (Deception+Stealth)
     * + high-elf Keen Senses (Perception); Expertise stays the CLASS pair */
    sim_reset();
    G.origin = ORIG_ASTARION;
    mk_party(CLS_ROGUE, 1);
    u32 cls_pair = (1u << SK_STEALTH) | (1u << SK_SLEIGHT_OF_HAND);
    u32 want = cls_pair | (1u << SK_DECEPTION) | (1u << SK_PERCEPTION);
    T_ASSERT(G.pm[0].skills == want, "Astarion skills %x, want %x",
             (unsigned)G.pm[0].skills, (unsigned)want);
    T_ASSERT(G.pm[0].expert == cls_pair, "Astarion expertise %x, want %x",
             (unsigned)G.pm[0].expert, (unsigned)cls_pair);
    /* Shadowheart: cleric Religion+Medicine + Acolyte Insight+Religion */
    sim_reset();
    G.origin = ORIG_SHADOW;
    mk_party(CLS_CLERIC, 1);
    want = (1u << SK_RELIGION) | (1u << SK_MEDICINE) | (1u << SK_INSIGHT);
    T_ASSERT(G.pm[0].skills == want, "Shadowheart skills %x, want %x",
             (unsigned)G.pm[0].skills, (unsigned)want);
    /* custom Tav keeps exactly the class pair -- no phantom background */
    sim_reset();
    mk_party(CLS_WIZARD, 1);
    want = (1u << SK_ARCANA) | (1u << SK_HISTORY);
    T_ASSERT(G.pm[0].skills == want, "custom wizard skills %x, want %x",
             (unsigned)G.pm[0].skills, (unsigned)want);
}

/* ------------------------------------------------- creation UI (game.c)
 * The real screens, driven by real keys: race picker (with the subrace
 * step), background picker, the standard-array swap UI, and the whole
 * game_creation flow -- manual, demo-poked, and origin-skipped. */

static void t_race_picker_reaches_all(void) {
    /* every playable entry is reachable by cursor; sub lists confirm once
     * more (all four SRD subrace groups hold exactly one subrace today) */
    for (int b = 0; b < R5RACE_BASE_COUNT; b++) {
        sim_reset();
        G_DEMO = 0;
        sim_budget(200000, 0);
        char script[160] = "";
        for (int i = 0; i < b; i++) strcat(script, "DOWN ");
        strcat(script, r5_race_bases[b].sub ? "A A" : "A");
        script_keys(script);
        int e = game_race_pick(ORIG_CUSTOM);
        T_ASSERT(e == r5_race_bases[b].first,
                 "base %d picked entry %d, want %d", b, e, r5_race_bases[b].first);
    }
    /* the Urge's cursor wakes on Dragonborn */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("A");
    int e = game_race_pick(ORIG_DURGE);
    T_ASSERT(e == R5RACE_DRAGONBORN, "durge default pick %d, want dragonborn", e);
}

static void t_race_picker_card_and_back(void) {
    /* the dwarf card: merged entry name, fixed ASIs, and a blurb that
     * wraps on word boundaries ("shrug" starting a row proves no
     * mid-word split -- a naive 26-cut would render "hrug...") */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("SNAP A SNAP B DOWN A SNAP A");
    int e = game_race_pick(ORIG_CUSTOM);
    T_ASSERT(snap_count() == 3, "race picker: %d snaps", snap_count());
    T_ASSERT(snap_contains(0, "Hill Dwarf"), "card lacks the merged entry name");
    T_ASSERT(snap_contains(0, "+2 CON"), "card lacks the headline ASI");
    T_ASSERT(snap_contains(0, "+1 WIS"), "card lacks the subrace ASI");
    T_ASSERT(snap_contains(0, "shrug off poison; every"),
             "dwarf blurb wrapped mid-word");
    /* snap 1: the subrace list after A on Dwarf */
    T_ASSERT(snap_contains(1, "Hill Dwarf"), "subrace list missing");
    /* B returned to the bases; DOWN A A landed the high elf */
    T_ASSERT(snap_contains(2, "High Elf"), "post-back card should show High Elf");
    T_ASSERT(e == R5RACE_HIGH_ELF, "picked %d after back-out, want high elf", e);
    /* human: the +1 ALL card and a subrace-free confirm */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("DOWN DOWN DOWN SNAP A");
    e = game_race_pick(ORIG_CUSTOM);
    T_ASSERT(e == R5RACE_HUMAN, "picked %d, want human", e);
    T_ASSERT(snap_contains(0, "+1 ALL"), "human card lacks +1 ALL");
}

static void t_bg_picker_flow(void) {
    /* row 0 = Acolyte: skills line + flavor line land whole */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("SNAP A");
    int bg = game_bg_pick(ORIG_CUSTOM);
    T_ASSERT(bg == R5BG_ACOLYTE, "picked %d, want acolyte", bg);
    T_ASSERT(snap_contains(0, "Insight, Religion"), "skills line missing");
    T_ASSERT(snap_contains(0, "Temple-raised; the rites"),
             "acolyte blurb wrapped mid-word");
    /* B backs out to the race picker */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("B");
    bg = game_bg_pick(ORIG_CUSTOM);
    T_ASSERT(bg == -1, "B gave %d, want -1 (back)", bg);
    /* the Urge's cursor wakes on Haunted One */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("SNAP A");
    bg = game_bg_pick(ORIG_DURGE);
    T_ASSERT(bg == R5BG_HAUNTED_ONE, "durge default %d, want haunted one", bg);
    T_ASSERT(snap_contains(0, "Investigation, Survival"),
             "haunted one skills line missing");
}

static void t_stat_swap_ui(void) {
    /* wizard preset 8/13/14/15/12/10: grab STR, drop on INT, Begin */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    s8 out[6] = { 0 };
    script_keys("A DOWN DOWN DOWN A DOWN DOWN DOWN A");
    int r = game_stats_assign(CLS_WIZARD, R5RACE_NONE, out);
    T_ASSERT(r == 1, "stat screen returned %d, want 1", r);
    static const s8 want[6] = { 15, 13, 14, 8, 12, 10 };
    for (int a = 0; a < 6; a++)
        T_ASSERT(out[a] == want[a], "swapped ab[%d] %d, want %d",
                 a, out[a], want[a]);
    /* live ASI display: high-elf wizard shows base, bonus, total, mod */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("SNAP DOWN DOWN DOWN DOWN DOWN DOWN A");
    r = game_stats_assign(CLS_WIZARD, R5RACE_HIGH_ELF, out);
    T_ASSERT(r == 1, "asi stat screen returned %d", r);
    T_ASSERT(snap_contains(0, "DEX 13  +2  15 (+2)"), "DEX row misrendered");
    T_ASSERT(snap_contains(0, "INT 15  +1  16 (+3)"), "INT row misrendered");
    T_ASSERT(snap_contains(0, "STR  8       8 (-1)"), "STR row misrendered");
    T_ASSERT(snap_contains(0, "Begin"), "Begin row missing");
    /* B with a score in hand releases it; B empty-handed backs out */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    script_keys("A B B");
    r = game_stats_assign(CLS_WIZARD, R5RACE_NONE, out);
    T_ASSERT(r == 0, "B-back returned %d, want 0", r);
}

static void t_creation_flow_custom(void) {
    /* the whole flow, manual: hill-dwarf sage wizard, preset spread,
     * default name (grid END row) */
    sim_reset();
    G_DEMO = 0;
    sim_budget(400000, 0);
    script_keys("A A "                                   /* Dwarf -> Hill Dwarf */
                "DOWN DOWN A "                           /* Sage */
                "DOWN DOWN DOWN DOWN DOWN DOWN A "       /* Begin (preset) */
                "DOWN DOWN DOWN DOWN A");                /* name: END -> TAV */
    game_creation(CLS_WIZARD, ORIG_CUSTOM);
    T_ASSERT(G.pm[0].race == R5RACE_HILL_DWARF, "race %d", G.pm[0].race);
    T_ASSERT(G.pm[0].background == R5BG_SAGE, "bg %d", G.pm[0].background);
    T_ASSERT(!strcmp(G.pm[0].name, "TAV"), "name '%s'", G.pm[0].name);
    T_ASSERT(party5[0].ab[R5_CON] == 16 && party5[0].ab[R5_WIS] == 13,
             "dwarf wizard CON %d WIS %d, want 16/13",
             party5[0].ab[R5_CON], party5[0].ab[R5_WIS]);
    T_ASSERT(party5[0].hpmax == 10,      /* d6 + CON 16 (+3) + toughness 1 */
             "dwarf wizard hpmax %d, want 10", party5[0].hpmax);
    /* sage duplicates the wizard picks: the union stays the class pair */
    T_ASSERT(G.pm[0].skills == ((1u << SK_ARCANA) | (1u << SK_HISTORY)),
             "skills %x", (unsigned)G.pm[0].skills);
    T_ASSERT(log_contains("create race=1 bg=3"), "create log line missing");
}

static void t_creation_flow_origin_skips(void) {
    /* a fixed origin walks straight through: no picker consumes a key
     * (an empty queue would hang a shown screen into the frame budget) */
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    game_creation(CLS_CLERIC, ORIG_SHADOW);
    T_ASSERT(!strcmp(G.pm[0].name, "Shadowh"), "name '%s'", G.pm[0].name);
    T_ASSERT(G.pm[0].race == R5RACE_HALF_ELF && G.pm[0].background == R5BG_ACOLYTE,
             "shadowheart identity race %d bg %d", G.pm[0].race, G.pm[0].background);
    T_ASSERT(party5[0].ab[R5_WIS] == 16, "WIS %d, want 16", party5[0].ab[R5_WIS]);
    T_ASSERT(G.pm[0].subclass == R5SUB_DOMAIN_OF_MASKS, "subclass %d",
             G.pm[0].subclass);
    T_ASSERT(log_contains("create race=7 bg=1"), "create log line missing");
}

static void t_creation_demo_paths(void) {
    /* unpoked demo: the legacy race-none/preset sheet, auto-advanced */
    sim_reset();
    G_DEMO = 1;
    sim_budget(400000, 0);
    game_creation(CLS_BARD, ORIG_CUSTOM);
    T_ASSERT(G.pm[0].race == R5RACE_NONE && G.pm[0].background == R5BG_NONE,
             "unpoked demo race %d bg %d", G.pm[0].race, G.pm[0].background);
    T_ASSERT(party5[0].ab[R5_CHA] == 15, "unpoked demo CHA %d, want preset 15",
             party5[0].ab[R5_CHA]);
    T_ASSERT(!strcmp(G.pm[0].name, "TAV"), "demo name '%s'", G.pm[0].name);
    /* ...the Urge included (durge_check compatibility) */
    sim_reset();
    G_DEMO = 1;
    sim_budget(400000, 0);
    game_creation(CLS_ROGUE, ORIG_DURGE);
    T_ASSERT(G.pm[0].race == R5RACE_NONE, "unpoked durge race %d", G.pm[0].race);
    /* poked demo: race, background, and a rearranged (legal) spread land */
    sim_reset();
    G_DEMO = 1;
    sim_budget(400000, 0);
    G_DEMO_RACE = R5RACE_HILL_DWARF;
    G_DEMO_BG = R5BG_SAGE;
    G_DEMO_AB = 1;
    static const u8 spread[6] = { 8, 13, 15, 14, 12, 10 };   /* CON/INT swap */
    for (int a = 0; a < 6; a++) G_AB_BUF[a] = spread[a];
    game_creation(CLS_WIZARD, ORIG_CUSTOM);
    T_ASSERT(G.pm[0].race == R5RACE_HILL_DWARF && G.pm[0].background == R5BG_SAGE,
             "poked demo race %d bg %d", G.pm[0].race, G.pm[0].background);
    T_ASSERT(party5[0].ab[R5_CON] == 17 && party5[0].ab[R5_INT] == 14,
             "poked demo CON %d INT %d, want 17/14",
             party5[0].ab[R5_CON], party5[0].ab[R5_INT]);
    T_ASSERT(log_contains("create race=1 bg=3 ab=8/13/17/14/13/10"),
             "poked demo create log missing");
    /* a poke that is NOT the class array is rejected, loudly */
    sim_reset();
    G_DEMO = 1;
    sim_budget(400000, 0);
    G_DEMO_AB = 1;
    for (int a = 0; a < 6; a++) G_AB_BUF[a] = 18;
    game_creation(CLS_WIZARD, ORIG_CUSTOM);
    T_ASSERT(log_contains("spread rejected"), "illegal spread not rejected");
    T_ASSERT(party5[0].ab[R5_INT] == 15, "rejected spread INT %d, want preset 15",
             party5[0].ab[R5_INT]);
}

/* ------------------------------------------------- party5 + audit units */

static void t_spell_dc_math(void) {
    sim_reset();
    mk_party(CLS_WIZARD, 1);
    /* SRD: DC = 8 + prof + casting mod. L1 wizard: 8 + 2 + INT 15 (+2) */
    T_ASSERT(party5_spell_dc(&party5[0]) == 12,
             "wizard L1 DC %d, want 12", party5_spell_dc(&party5[0]));
    R5Creature c = { 0 };
    c.cls = CLS_WARLOCK; c.level = 1; c.ab[R5_CHA] = 20;
    T_ASSERT(party5_spell_dc(&c) == 15, "CHA-20 warlock DC %d, want 15",
             party5_spell_dc(&c));
    /* r5_prof clamps to the prologue domain (levels 1..3): a "level 5"
     * sheet still gets +2, not the open-SRD +3 */
    c.cls = CLS_CLERIC; c.level = 5; c.ab[R5_WIS] = 10;
    T_ASSERT(party5_spell_dc(&c) == 10,
             "L5 cleric DC %d, want 10 (prologue prof clamp)", party5_spell_dc(&c));
}

static void t_heal_full_rest(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    party_add_laezel();
    party5[0].hp = 1;
    r5_spend_slot(&party5[0], 1);
    party5[1].hp = 0;
    party5[1].conds |= C_UNCONSCIOUS;
    party_heal_full();
    T_ASSERT(party5[0].hp == party5[0].hpmax, "cleric hp %d/%d after full rest",
             party5[0].hp, party5[0].hpmax);
    T_ASSERT(party5[0].slots[0] == 2, "cleric slots %d, want 2 (SRD L1)",
             party5[0].slots[0]);
    T_ASSERT(party5[1].hp == party5[1].hpmax, "Lae'zel hp %d/%d after full rest",
             party5[1].hp, party5[1].hpmax);
    T_ASSERT(!(party5[1].conds & C_UNCONSCIOUS), "Lae'zel still unconscious");
}

/* 5e: unconsciousness ends when you regain hit points -- never because a
 * bookkeeping rebuild ran. C_UNCONSCIOUS is bit 13 of u16 conds; the
 * shipped truncation (a u8 carry in build()) zeroed it at every
 * party5_refresh, so a member downed in one battle stood up as a ghost
 * at the next encounter's refresh. This is the regression pin. */
static void t_ko_survives_refresh(void) {
    sim_reset();
    mk_party(CLS_CLERIC, 1);
    party_add_laezel();
    party5[1].hp = 0;                    /* downed in some earlier battle */
    party5[1].conds |= C_UNCONSCIOUS;
    party5_refresh(1);                   /* every encounter start does this */
    T_ASSERT(party5[1].hp == 0, "refresh revived her to %d hp", party5[1].hp);
    T_ASSERT(party5[1].conds & C_UNCONSCIOUS,
             "refresh dropped the KO flag (the u8 truncation)");
    party5_refresh_all();                /* and the bulk path */
    T_ASSERT(party5[1].conds & C_UNCONSCIOUS, "refresh_all dropped the KO flag");
    r5_heal(&party5[1], 5);              /* healing still wakes her */
    T_ASSERT(party5[1].hp == 5 && !(party5[1].conds & C_UNCONSCIOUS),
             "heal failed to wake her (hp %d conds %04x)",
             party5[1].hp, party5[1].conds);
    /* a level-up's max-hp growth is regained hp: it wakes her (the carry
     * measures missing hp against the OLD max, so hp comes back > 0 --
     * hp > 0 while KO'd would be a sleep no heal could ever end) */
    party5[1].hp = 0;
    party5[1].conds |= C_UNCONSCIOUS;
    G.pm[1].level = 2;
    party5_refresh(1);
    T_ASSERT(party5[1].hp > 0, "L2 rebuild left her at %d hp", party5[1].hp);
    T_ASSERT(!(party5[1].conds & C_UNCONSCIOUS),
             "hp %d yet still KO: an unwakeable sleep", party5[1].hp);
}

/* the on-ROM audit_check pins the audits' CONTENT against a byte-exact
 * golden; running them here adds what the golden can't -- ASan/UBSan
 * eyes on the same string assembly */
static void t_audit_native(void) {
    sim_reset();
    mk_party(CLS_BARD, 1);
    ability_audit();
    sheet_audit();
    T_ASSERT(log_contains("audit cls=11"), "ability audit never reached cls 11");
    T_ASSERT(log_contains("sheet cls=11"), "sheet audit never reached cls 11");
}

/* ------------------------------------------------- title flows (game.c)
 * jukebox/karaoke are reachable ONLY via SELECT on the title screen; no
 * ROM scenario presses it, so until now no test had ever run them. */

static void t_title_jukebox_karaoke(void) {
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    /* the title fade_in eats 20 frames (keys land after W24); L L flips
     * attract mode on and back off; SELECT opens the jukebox; nine DOWNs
     * land on track 9 = SELUNE; A plays it and, since it carries lyrics,
     * drops into karaoke; the SNAP catches the synced lyric sheet around
     * playback rows 224..287; B backs out twice; START leaves the title. */
    script_keys("W24 L L SELECT W4 "
                "DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN DOWN A "
                "W200 SNAP B W8 B W8 START");
    game_title();
    T_ASSERT(snap_count() == 1, "snap never fired (script derailed)");
    T_ASSERT(snap_contains(0, "morning takes") || snap_contains(0, "all that's left"),
             "no Under Selune lyric on the karaoke sheet");
    T_ASSERT(sim_last_music == SONG_PRELUDE,
             "music %d after backing out, want the title PRELUDE", sim_last_music);
    T_ASSERT(!G_DEMO, "L L should have left attract mode off");
}

static void t_crawl_pages(void) {
    sim_reset();
    G_DEMO = 1;
    sim_budget(200000, 0);
    game_crawl();
    T_ASSERT(log_contains("say: The city of Baldur's Gate"),
             "the crawl never told page 1");
    T_ASSERT(log_contains("dragonfire"), "the crawl never told page 4");
}

static void t_origin_choose_flow(void) {
    sim_reset();
    /* demo-driven: 8 = "this class's origin row", computed in the chooser */
    G_DEMO = 1;
    G_DEMO_ORIGIN = ORIG_COUNT;
    sim_budget(200000, 0);
    int o = game_origin_choose(CLS_CLERIC);
    T_ASSERT(o == ORIG_SHADOW, "cleric origin row gave %d, want Shadowheart", o);
    G_DEMO_ORIGIN = ORIG_DURGE;
    o = game_origin_choose(CLS_MONK);      /* no monk companion: 2-row menu */
    T_ASSERT(o == ORIG_DURGE, "Dark Urge row gave %d", o);
    /* manual: DOWN, A takes "Play Gale" under wizard */
    G_DEMO = 0;
    script_keys("W16 DOWN A");
    o = game_origin_choose(CLS_WIZARD);
    T_ASSERT(o == ORIG_GALE, "manual pick gave %d, want Gale", o);
    T_ASSERT(!strcmp(origin_name(ORIG_SHADOW), "Shadowheart"),
             "origin 4 named '%s'", origin_name(ORIG_SHADOW));
    /* identity portraits win for every fixed origin (the borrowed-faces law) */
    for (int i = 0; i < ORIG_CUSTOM; i++) {
        int cls = origin_class(i) < 0 ? CLS_BARD : origin_class(i);
        T_ASSERT(member_look(i, cls).por == origin_portrait(i),
                 "origin %d portrait drifted from its table", i);
    }
}

static void t_name_entry_grid(void) {
    sim_reset();
    G_DEMO = 0;
    sim_budget(200000, 0);
    char nm[8];
    /* B B B clears the default TAV; seven As at grid (0,0) type AAAAAA and
     * prove the 6-char cap holds; DOWN x4 reaches END; A confirms */
    script_keys("W12 B B B A A A A A A A DOWN DOWN DOWN DOWN A");
    game_name_entry(nm);
    T_ASSERT(!strcmp(nm, "AAAAAA"), "name '%s', want AAAAAA (6-char cap)", nm);
    /* straight to END keeps the default */
    script_keys("W12 DOWN DOWN DOWN DOWN A");
    game_name_entry(nm);
    T_ASSERT(!strcmp(nm, "TAV"), "name '%s', want the TAV default", nm);
    G_DEMO = 1;                        /* demo path: no screen at all */
    game_name_entry(nm);
    T_ASSERT(!strcmp(nm, "TAV"), "demo name '%s', want TAV", nm);
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

/* each class's longest blurb line, from the design: it must land intact on
 * ONE grid row -- blurb_draw once split "mends allies." into "mends allie/s."
 * because the card was narrower than the copy */
static const char* const blurb_line[CLS_COUNT] = {
    "foes, mends", "Strikes out", "sets traps.", "Brilliant.",
    "and surging", "answer you.", "off blades.", "magic. Wild",
    "Fists like",  "Smite evil,", "Bends every", "power owed.",
};

static void t_class_select_art(void) {
    for (int cls = 0; cls < CLS_COUNT; cls++) {
        sim_reset();
        char script[256] = "";
        for (int i = 0; i < cls; i++) strcat(script, "DOWN ");
        strcat(script, "SNAP A");
        script_keys(script);
        int r = game_class_select();
        T_ASSERT(snap_contains(0, blurb_line[cls]),
                 "%s blurb line wrapped mid-word", cls_display[cls]);
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
    { "roster_recruit_overflow",   t_roster_recruit_overflow },
    { "party_swap_menu",           t_party_swap_menu },
    { "bench_round_trip",          t_bench_round_trip },
    { "rest_heals_bench",          t_rest_heals_bench },
    { "crash_scatter_regather",    t_crash_scatter_regather },
    { "companion_subclass_reveal", t_companion_subclass_reveal },
    { "companion_subclass_bench",  t_companion_subclass_bench },
    { "party_row_gated",           t_party_row_gated },
    { "battle_auto_win",           t_battle_auto_win },
    { "battle_manual_eldritch_blast", t_battle_manual_eldritch_blast },
    { "battle_wizard_sleep",       t_battle_wizard_sleep },
    { "battle_wizard_missiles",    t_battle_wizard_missiles },
    { "battle_cleric_rescue",      t_battle_cleric_rescue },
    { "battle_bard_rescue",        t_battle_bard_rescue },
    { "battle_helm_allies",        t_battle_helm_allies },
    { "battle_no_zombie_turns",    t_battle_no_zombie_turns },
    { "battle_helm_tav_down",      t_battle_helm_tav_down },
    { "battle_druid_wildshape",    t_battle_druid_wildshape },
    { "battle_rogue_pick",         t_battle_rogue_pick },
    { "battle_ranger_conc",        t_battle_ranger_conc },
    { "battle_dark_caster_disadvantage", t_battle_dark_caster_disadvantage },
    { "battle_dark_darkvision_race_clean", t_battle_dark_darkvision_race_clean },
    { "battle_dark_imp_exempt",    t_battle_dark_imp_exempt },
    { "battle_dark_everburn_lights", t_battle_dark_everburn_lights },
    { "race_none_baseline",        t_race_none_baseline },
    { "race_mechanics_sheet",      t_race_mechanics_sheet },
    { "origin_sheet_identities",   t_origin_sheet_identities },
    { "origin_subclass_single_source", t_origin_subclass_single_source },
    { "background_skill_union",    t_background_skill_union },
    { "race_picker_reaches_all",   t_race_picker_reaches_all },
    { "race_picker_card_and_back", t_race_picker_card_and_back },
    { "bg_picker_flow",            t_bg_picker_flow },
    { "stat_swap_ui",              t_stat_swap_ui },
    { "creation_flow_custom",      t_creation_flow_custom },
    { "creation_flow_origin_skips", t_creation_flow_origin_skips },
    { "creation_demo_paths",       t_creation_demo_paths },
    { "spell_dc_math",             t_spell_dc_math },
    { "heal_full_rest",            t_heal_full_rest },
    { "ko_survives_refresh",       t_ko_survives_refresh },
    { "audit_native",              t_audit_native },
    { "title_jukebox_karaoke",     t_title_jukebox_karaoke },
    { "crawl_pages",               t_crawl_pages },
    { "origin_choose_flow",        t_origin_choose_flow },
    { "name_entry_grid",           t_name_entry_grid },
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
