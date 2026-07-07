# OBJ palettes — a per-scene resource, not a per-game one

> **Status: SHIPPED.** Replaced the static 16-bank assignment that forced the
> 2026-07-08 "black cursor" and "samey blue party" bugs. Landed in four
> gate-green stones (core → party → enemies → dice); src/palette.{c,h} is the
> allocator, test/host the invariant suite (t_pal_*). Every real fight fits the
> nine transient banks with headroom — the worst cases are the helm and the
> grove gates, and each scene logs its live peak as `pal peak=N/9`.

## The problem this fixes

The GBA gives sprites sixteen 16-color OBJ palette banks. We were spending
that as a **per-game** budget: Lae'zel was *always* bank 1, the imps
*always* bank 4, the eight damage-type dice colors *always* banks 8–15 —
one static layout, frozen for the whole game. So all sixteen banks read as
"taken," and every new face hit a wall that forced ugly trades (repaint the
cursor, or let the party go samey, or sacrifice a dice color).

But **nothing on screen ever needs all sixteen at once.** A battle needs the
~3 party members actually fighting, that fight's 2–4 enemy types, the few
damage colors it actually rolls, and the cursor. A field room needs the
party plus that room's NPCs. Sixteen is a *per-frame* ceiling; we hit it
only because we froze one layout across every frame of the game.

## The model: persistent + transient

Banks split into two regions.

**Persistent** — loaded once, owned forever, never repacked:

| bank | owner |
|------|-------|
| 0    | Tav (the played hero; its class palette is loaded per class) |
| 1    | Lae'zel |
| 2    | Shadowheart |
| 3    | Astarion |
| 4    | Gale |
| 5    | Wyll |
| 7    | UI cursor |

Every party identity owns a bank. The party is distinct **by construction** —
"almost all the player sprites are the same blue" cannot happen, because no
two members ever share a bank. The cursor owns bank 7; nothing else can land
on it. (Six party banks sit loaded even when a member is benched — cheap, and
it means no repack when the party composition changes at a swap.)

**Transient** — banks 6 and 8–15 (nine banks), repacked on every scene entry:

Everything that is scene-specific — a fight's enemy stat blocks, the
damage-type dice colors that fight rolls, field garnish (tether, cones,
popups). A scene declares what it needs; the allocator packs those palettes
into the transient banks and hands each drawer its current bank.

The two regions never sum past sixteen because they never coexist beyond a
scene's real need: persistent is 7 (cursor + six party); transient is 9, and
a battle's honest draw — a few enemy types + the handful of damage colors in
play + garnish — fits with headroom. The dice stop reserving eight banks for a
game that rarely shows three damage types *at once*: a fight leases a damage
color when its dice are on screen and frees it when the tray clears (`pal_hold`
once the combatants are in, `pal_release` per attack), so only the palettes
**simultaneously** on screen count against the nine — never a whole 15-round
fight's cumulative damage types.

## The API (src/palette.c/.h)

Palettes are named by a stable **id** (an enum: `PAL_TAV`, `PAL_LAEZEL`, …,
`PAL_IMP`, `PAL_GOBLIN`, `PAL_DICE_FIRE`, …), backed by a build-time colors
table (one 16-color palette per id, generated from tools/art like today —
the *data* is unchanged, only the *bank assignment* moves to runtime).

- `pal_boot()` — at startup: load the persistent palettes (party + cursor)
  into their fixed banks and record `pal_bank[id]`.
- `pal_tav_class(cls)` — load class `cls`'s Tav palette into bank 0.
- `pal_scene_begin()` — free the transient region (invalidate its `pal_bank`
  entries); called from `room_enter`. A battle inherits the room's combatants
  rather than repacking them, so `encounter_run` does not scene-begin.
- `pal_use(id)` — return the bank holding palette `id`, loading it into the
  next free transient bank on first request this scene. Persistent ids
  return their fixed bank immediately.
- `pal_hold()` / `pal_release()` — a fight `pal_hold`s once its combatants are
  leased, then `pal_release`s each attack to free that attack's dice colors
  back to the mark (simultaneous, not cumulative).
- `pal_bank[id]` — the current bank of a loaded palette (0xFF = not loaded);
  what a drawer passes to `obj_set`.

Drawers stop hard-coding bank numbers. `member_look` returns each member's
persistent bank (via `pal_persistent_bank`). `field_add_npc` for an enemy
passes `pal_use(PAL_that_mob)`; for an ally, its persistent bank. `DPAL(type)`
becomes `pal_use(dmg_pal_id[type])` — a small table folding the 13 damage types
onto the eight dice colors, exactly as the old dmgpal ramps did. The cursor
uses its persistent bank. ~A handful of source points change; the `obj_set`
call sites read their bank from those sources, not from constants.

## Why this is also the testable answer

The invariants are now checkable, not by-eye:

- **Persistent banks are never overwritten.** `pal_use` allocates only from
  the transient region; a build/runtime assert catches any attempt to load a
  persistent id transiently, or a transient request landing on bank 7/0–5.
  The "looter repaints the cursor" bug becomes *inexpressible*.
- **A scene fits its budget.** If a scene's `pal_use` requests exceed the
  nine transient banks, the allocator logs/asserts loudly (a real "this
  fight needs too many palettes" signal) instead of silently colliding.
- The mkassets build-assert that every class has a Tav palette stays; the
  allocator makes the whole assignment inspectable at a single seam.

## Stones, in order (each gate-green)

1. **Core.** `palette.c/.h`: the id enum, colors table (fed from tools/art),
   `pal_boot`/`pal_scene_begin`/`pal_use`/`pal_bank`. Boot loads persistent.
   Assert wiring. No drawer changes yet — banks resolve to today's numbers so
   appearance is byte-identical; the mechanism is proven first.
2. **Party persistent.** `member_look` returns persistent banks; Astarion→3,
   Gale→4, Wyll→5 (evicting the enemy palettes that squatted 3–5). The
   samey-party bug dies here. Enemies temporarily double up until stone 3.
3. **Enemies transient.** Each room/battle loads its mobs via `pal_use`;
   `field_add_npc` enemy calls read the allocated bank. Enemies leave the
   party's region for good.
4. **Dice transient.** `DPAL` allocates a damage type's color on first roll;
   the eight fixed dice banks are freed into the transient pool.

Verification-first per the testing ladder: the allocator is host-reachable
(pure data + logic, no hardware) — `pal_use`/eviction/budget/persistent-guard
get host tests; each stone keeps the full scenario fleet green (palette data,
not timing, so no RNG shift), and the per-scene-fits and persistent-never-
touched asserts run in the gate.
