#ifndef PALETTE_H
#define PALETTE_H
#include "gba.h"

/* The OBJ palette allocator -- docs/palettes.md.
 *
 * The GBA's sixteen 16-color OBJ banks are a PER-FRAME budget, not a
 * per-game one. Palettes are named by a stable id:
 *
 *   Persistent ids (party + cursor) own a fixed bank, loaded once at pal_boot
 *   and never repacked -- the party is distinct BY CONSTRUCTION.
 *
 *   Transient ids (a scene's enemies, the damage-type dice it rolls, field
 *   garnish) are packed into banks {6, 8..15} per scene: pal_scene_begin frees
 *   the region, pal_use hands a scene each palette on first request and asserts
 *   loudly if the scene overruns the nine transient banks.
 *
 * The enum order MUST match tools/mkassets.py obj_palette_table(); a
 * _Static_assert in palette.c binds PAL_* to the generated PALI_* so any
 * reorder breaks the build rather than silently miscoloring a sprite. */
enum {
    /* persistent -- fixed banks (persist_bank[] in palette.c): every id here
     * is <= PAL_CURSOR, which is how is_persistent() tells the regions apart */
    PAL_TAV, PAL_LAEZEL, PAL_SHADOW, PAL_ASTARION, PAL_GALE, PAL_WYLL, PAL_CURSOR,
    /* transient -- enemies & field NPCs */
    PAL_US, PAL_IMP, PAL_FLAYER, PAL_ZHALK, PAL_ZEVLOR, PAL_BOAR, PAL_SCAV,
    PAL_DEVOURER, PAL_LOOTER, PAL_WARRYN, PAL_WITHERS, PAL_SKELETON, PAL_GOBLIN,
    /* transient -- dice / garnish damage-type colors */
    PAL_DICE_HEAL, PAL_DICE_RADIANT, PAL_DICE_PHYS, PAL_DICE_FIRE, PAL_DICE_POISON,
    PAL_DICE_FORCE, PAL_DICE_PSYCHIC, PAL_DICE_COLD,
    PAL_ID_COUNT
};

#define PAL_NOT_LOADED 0xFF

/* the current bank of each id (0xFF = not loaded this scene); a drawer passes
 * pal_bank[id] -- or the pal_use(id) return -- straight to obj_set */
extern u8 pal_bank[PAL_ID_COUNT];

void pal_boot(void);          /* load persistent palettes; call once at startup */
void pal_tav_class(int cls);  /* load class cls's Tav palette into bank 0 */
void pal_scene_begin(void);   /* free the transient region (room/battle entry) */
int  pal_use(int id);         /* the bank holding id, loading it transiently if new */
int  pal_persistent_bank(int id);  /* fixed bank of a persistent id -- pure, no
                                    * boot needed (member_look's art identity) */
void pal_hold(void);          /* mark the transient high-water once a scene's lasting
                               * palettes (a fight's combatants) are in */
void pal_release(void);       /* free every transient leased since pal_hold (a battle
                               * pops each attack's dice colors back to the mark) */

#endif
