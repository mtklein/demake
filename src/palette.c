/* The OBJ palette allocator (docs/palettes.md). Persistent ids own fixed
 * banks loaded at boot; transient ids (a scene's enemies + dice colors) are
 * packed into banks {6, 8..15} per scene. The data is the generated
 * pal_colors[] table (tools/mkassets.py); this file owns only the runtime
 * bank policy. */
#include "gba.h"
#include "assets.h"
#include "game.h"      /* CLS_COUNT (pal_tav_class bound) */
#include "palette.h"

/* Bind the public PAL_* enum to the generated PALI_* order: a reorder of
 * either the enum or obj_palette_table() breaks the build here, not by eye. */
_Static_assert(PAL_ID_COUNT == PALI_N, "palette id count vs tools/mkassets.py");
_Static_assert(
    PAL_TAV == PALI_TAV && PAL_LAEZEL == PALI_LAEZEL && PAL_SHADOW == PALI_SHADOW &&
    PAL_ASTARION == PALI_ASTARION && PAL_GALE == PALI_GALE && PAL_WYLL == PALI_WYLL &&
    PAL_CURSOR == PALI_CURSOR && PAL_US == PALI_US && PAL_IMP == PALI_IMP &&
    PAL_FLAYER == PALI_FLAYER && PAL_ZHALK == PALI_ZHALK && PAL_ZEVLOR == PALI_ZEVLOR &&
    PAL_BOAR == PALI_BOAR && PAL_SCAV == PALI_SCAV && PAL_DEVOURER == PALI_DEVOURER &&
    PAL_LOOTER == PALI_LOOTER && PAL_WARRYN == PALI_WARRYN && PAL_WITHERS == PALI_WITHERS &&
    PAL_SKELETON == PALI_SKELETON && PAL_GOBLIN == PALI_GOBLIN &&
    PAL_DICE_HEAL == PALI_DICE_HEAL && PAL_DICE_RADIANT == PALI_DICE_RADIANT &&
    PAL_DICE_PHYS == PALI_DICE_PHYS && PAL_DICE_FIRE == PALI_DICE_FIRE &&
    PAL_DICE_POISON == PALI_DICE_POISON && PAL_DICE_FORCE == PALI_DICE_FORCE &&
    PAL_DICE_PSYCHIC == PALI_DICE_PSYCHIC && PAL_DICE_COLD == PALI_DICE_COLD,
    "palette id order must match tools/mkassets.py obj_palette_table()");

u8 pal_bank[PAL_ID_COUNT];

/* Persistent ids come first in the enum (0..PAL_CURSOR) and own these banks.
 * Note PAL_CURSOR owns bank 7, not 6: bank 6 is the first transient slot. */
static const u8 persist_bank[] = { 0, 1, 2, 3, 4, 5, 7 };
static int is_persistent(int id) { return id >= 0 && id <= PAL_CURSOR; }

int pal_persistent_bank(int id) { return is_persistent(id) ? persist_bank[id] : -1; }

/* The transient pool, banks {6, 8..15}. Bank 6 first, then the dice banks
 * high-to-low: while stone 3 has enemies transient but the dice still static,
 * this protects the hottest dice colors (physical 10, radiant 9, heal 8) from
 * an enemy landing on them until stone 4 packs both together. */
static const u8 trans_pool[] = { 6, 15, 14, 13, 12, 11, 10, 9, 8 };
#define TRANS_N ((int)(sizeof trans_pool / sizeof *trans_pool))
static int trans_next;   /* next free slot in trans_pool this scene */

static void pal_load(int id, int bank) {
    memcpy16(PAL_OBJ + bank * 16, pal_colors[id], 16);
}

/* Stone 3: the dice are still drawn at fixed banks 8-15 (DPAL, dice_ui) while
 * enemies have gone transient. A crowded fight's enemy may borrow a high dice
 * bank (the pool hands out 15,14,.. before 8,9,10 for exactly this reason);
 * reloading the eight dice colors at every scene entry -- before that scene's
 * enemies allocate -- keeps the borrow from carrying its miscolor into the
 * next room. Stone 4 makes the dice transient and this disappears. */
static void pal_dice_static(void) {
    for (int d = 0; d < 8; d++) pal_load(PAL_DICE_HEAL + d, 8 + d);
}

void pal_boot(void) {
    for (int i = 0; i < PAL_ID_COUNT; i++)
        pal_bank[i] = is_persistent(i) ? persist_bank[i] : PAL_NOT_LOADED;
    trans_next = 0;

    /* The persistent set: every party identity owns a bank (Astarion 3,
     * Gale 4, Wyll 5 now evict the enemies that squatted there), plus the
     * cursor on 7. Bank 0 stays the class-swapped Tav slot (pal_tav_class).
     *
     * Banks 6 and 8-15 still carry raw statics -- Zhalk and the eight dice
     * colors -- that stones 3 and 4 will move onto pal_use. They keep
     * pal_bank == NOT_LOADED: the allocator's view is that no scene has begun,
     * only that colors sit in the transient region for hard-coded drawers.
     * The enemies that lost 3-5 (Us, imps, flayer) are not loaded anywhere
     * this stone -- their sprites double up on the party's banks until stone 3
     * hands them their own transient leases. */
    static const struct { u8 id, bank; } boot[] = {
        { PAL_TAV, 0 }, { PAL_LAEZEL, 1 }, { PAL_SHADOW, 2 }, { PAL_ASTARION, 3 },
        { PAL_GALE, 4 }, { PAL_WYLL, 5 }, { PAL_CURSOR, 7 },
        { PAL_ZHALK, 6 },
        { PAL_DICE_HEAL, 8 }, { PAL_DICE_RADIANT, 9 }, { PAL_DICE_PHYS, 10 },
        { PAL_DICE_FIRE, 11 }, { PAL_DICE_POISON, 12 }, { PAL_DICE_FORCE, 13 },
        { PAL_DICE_PSYCHIC, 14 }, { PAL_DICE_COLD, 15 },
    };
    for (unsigned i = 0; i < sizeof boot / sizeof *boot; i++)
        pal_load(boot[i].id, boot[i].bank);
}

void pal_tav_class(int cls) {
    if (cls < 0 || cls >= CLS_COUNT) return;   /* UBSan once caught a read past [4] */
    memcpy16(PAL_OBJ, pal_tav_classes[cls], 16);
    pal_bank[PAL_TAV] = 0;
}

void pal_scene_begin(void) {
    for (int i = 0; i < PAL_ID_COUNT; i++)
        if (!is_persistent(i)) pal_bank[i] = PAL_NOT_LOADED;
    trans_next = 0;
    pal_dice_static();
}

int pal_use(int id) {
    if (is_persistent(id)) return persist_bank[id];   /* fixed bank, no allocation */
    if (pal_bank[id] != PAL_NOT_LOADED) return pal_bank[id];   /* stable within a scene */
    if (trans_next >= TRANS_N)
        panic("pal_use: scene transient budget exceeded", (u32)id);
    int bank = trans_pool[trans_next++];
    if (bank <= 5 || bank == 7)                       /* the pool must never yield */
        panic("pal_use: transient alloc hit a persistent bank", (u32)bank);
    pal_load(id, bank);
    pal_bank[id] = (u8)bank;
    mgba_logf("pal id=%d bank=%d used=%d/%d", id, bank, trans_next, TRANS_N);
    return bank;
}
