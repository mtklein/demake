/* The dice a roll is drawn AS IT LANDS: the shared face+digits primitive and
 * the headline tumble, used by both the combat tray and the field skill check.
 * Kept engine-thin -- only obj_set/obj_hide/memcpy16, mgba_logf, and the
 * generated tile ids -- so both a GBA build and the host sim link the SAME
 * code. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "dice_ui.h"

#define DICE_TUMBLE_F 12      /* headline spin length in frames (~0.2s) */
#define DICE_KEEP_F   8       /* resolve-beat length (advantage/disadvantage) */

static int die_objt(int sides) {
    switch (sides) {
        case 4:  return OBJT_DIE_D4;
        case 6:  return OBJT_DIE_D6;
        case 8:  return OBJT_DIE_D8;
        case 10: return OBJT_DIE_D10;
        case 12: return OBJT_DIE_D12;
        default: return OBJT_DIE_D20;
    }
}

void dice_stage_digits(void) {
    memcpy16((vu16*)((u32)OBJ_TILES + OBJ_TILE_COUNT * 32),
             &ui_tiles[('0' - 32) * 16], 10 * 16);
}

void dice_draw_one(int base, int x, int y, int sides, int value, int pal) {
    /* digits take the LOWER OAM slots so they render above the die face */
    obj_set(base + 2, x, y, 1, die_objt(sides), pal, 0);
    if (value >= 10) {
        obj_set(base, x + 1, y + 4, 0, OBJ_TILE_COUNT + value / 10, 10, 0);
        obj_set(base + 1, x + 8, y + 4, 0, OBJ_TILE_COUNT + value % 10, 10, 0);
    } else {
        obj_set(base, x + 4, y + 4, 0, OBJ_TILE_COUNT + value, 10, 0);
        obj_hide(base + 1);
    }
}

static void die_hide(int base) {
    obj_hide(base); obj_hide(base + 1); obj_hide(base + 2);
}

int dice_roll_headline(int base, int x0, int spacing, int y, int sides,
                       const u8* vals, int n, int total, int pal,
                       void (*step)(void)) {
    /* purely presentational spin: seeded off the frame clock and the target
     * value so it looks lively and never repeats identically, but drawn from
     * NO game rng -- the result was decided before we animate toward it, so
     * nothing here can move it. Deliberately no file-scope state: a static
     * .bss in this unit would sit ahead of field.c in the link and shift the
     * player-state addresses the deterministic scenarios poke. */
    u32 s = (u32)g_frame * 2654435761u + (u32)(vals[0] + 1) * 40503u + 0x9E3779B9u;
    for (int f = 0; f < DICE_TUMBLE_F; f++) {
        for (int i = 0; i < n; i++) {
            s = s * 1664525u + 1013904589u;
            dice_draw_one(base + i * 3, x0 + i * spacing, y, sides,
                          1 + (int)(s % (u32)sides), 10);
        }
        step();
    }
    for (int i = 0; i < n; i++)
        dice_draw_one(base + i * 3, x0 + i * spacing, y, sides, vals[i], pal);

    /* Resolve beat for an advantage/disadvantage pair: show which die was
     * kept. `total` is the die USED (higher under advantage, lower under
     * disadvantage), so the KEEPER is the one whose value == total. In one
     * continuous motion with the tumble: the DISCARD slides down and drops out
     * (no spare OBJ palette to dim it, and the tray sits over open board with no
     * bar to recede behind -- a clean drop reads best by eye), while the KEEPER
     * shudders and eases into the lead slot so the tray compacts with no gap.
     * A TIE (both == total) has no loser, and a straight (n==1) roll no pair:
     * both settle untouched. Returns the slot count the result now occupies (1
     * once a pair resolves to its keeper, else n) so the tray packs cleanly. */
    if (n != 2 || vals[0] == vals[1]) return n;

    int keep = vals[0] == total ? 0 : 1, drop = keep ^ 1;
    int kbase = base + keep * 3, kx = x0 + keep * spacing;
    int dbase = base + drop * 3, dx = x0 + drop * spacing;
    mgba_logf("d20 keep=%d drop=%d", vals[keep], vals[drop]);
    /* one rises, one falls: the keeper hops (jy) and shudders (jx) as it eases
     * into the lead slot; the discard sinks straight down and out. Stack
     * tables, not .bss -- a static here would shift the poked player addrs. */
    const signed char jx[DICE_KEEP_F] = { 0, 3, -3, 2, -2, 1, -1, 0 };
    const signed char jy[DICE_KEEP_F] = { 0, -2, -3, -2, -1, 0, 0, 0 };
    for (int f = 0; f < DICE_KEEP_F; f++) {
        /* the discard slides down out of the tray, then is gone for good */
        if (f < 4) dice_draw_one(dbase, dx, y + f * 4, sides, vals[drop], pal);
        else       die_hide(dbase);
        /* the keeper eases from its landing spot into the lead slot, leaping */
        int cx = kx + (x0 - kx) * (f + 1) / DICE_KEEP_F;
        dice_draw_one(kbase, cx + jx[f], y + jy[f], sides, vals[keep], pal);
        step();
    }
    /* land the keeper in the lead slot; free the slot it drifted in from */
    if (keep != 0) die_hide(kbase);
    dice_draw_one(base, x0, y, sides, vals[keep], pal);
    return 1;
}
