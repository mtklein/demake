#ifndef DICE_UI_H
#define DICE_UI_H
#include "gba.h"

/* Shared dice presentation, drawn once and used by BOTH the combat tray
 * (encounter.c) and the field skill check (events.c) -- one implementation,
 * no mirror. A die is a polyhedron face sprite plus its 1-2 value digits,
 * laid across three consecutive OAM slots (base, base+1 = digits; base+2 =
 * face) so the digits render above the face.
 *
 * step() pumps exactly ONE frame in the caller's world -- combat's pump()
 * (which keeps the live board drawing) or the field's bare frame() (which
 * leaves the dialog frozen). The animators call it so each caller keeps its
 * own feel while sharing the roll. */

/* Copy the ten digit tiles ('0'..'9') into OBJ scratch VRAM at OBJ_TILE_COUNT
 * so values can be overdrawn on a die face. Stage before drawing values. */
void dice_stage_digits(void);

/* Draw one settled die at screen (x,y): the sides-appropriate face in `pal`,
 * `value` overdrawn in white. Uses OAM slots base, base+1, base+2. */
void dice_draw_one(int base, int x, int y, int sides, int value, int pal);

/* Roll the headline die(s) into place: spin every face a beat, then land die
 * i on vals[i] in `pal`. n is 1 (straight roll) or 2 (advantage/disadvantage,
 * both dice spinning together). Die i sits at (x0 + i*spacing, y) on OAM slots
 * base + i*3. The spin uses a THROWAWAY counter, never the game rng -- the
 * outcome was decided before we animate toward it, so nothing here can shift a
 * result. Digits must be staged; step() pumps one frame per spin.
 *
 * `total` is the die USED after advantage/disadvantage picks between the pair
 * (higher under adv, lower under dis; == vals[0] for a straight roll). On an
 * n==2 roll it drives the resolve beat that follows the settle: the KEEPER
 * (value == total) shudders and eases into the lead slot, the DISCARD slides
 * down and out -- so the kept die reads at a glance and the tray compacts.
 * A tie (both == total) skips the beat. Purely presentational: `total` only
 * chooses which die to emphasise, never a result.
 *
 * Returns the number of tray slots the settled result occupies -- 1 once a
 * pair resolves to its single keeper (or a straight roll), 2 for a tie -- so
 * the caller can pack the dice that follow with no hole where the discard
 * was. */
int dice_roll_headline(int base, int x0, int spacing, int y, int sides,
                       const u8* vals, int n, int total, int pal,
                       void (*step)(void));

#endif
