# Battle 2.0 — Chrono-Trigger presentation, real 5e rules

> **Status: SHIPPED.** The cutover is complete — every encounter (deck imps,
> awakened thralls, the helm finale with countdown, warping cambions, and the
> Zhalk/flayer duel) runs on encounter.c + rules/; the FF4 battle system is
> deleted. All three encounter triggers below are live: proximity patrols
> with forward vision cones, scripted fights, and player-initiated ambushes,
> with surprise rounds in both directions. This doc is kept as the design
> rationale.

The FF4 battle system (separate scene, ATB, abstract numbers) gets replaced by
encounters fought **in place on the field map** with **SRD 5e mechanics** and
**visible dice**. This is one combined effort because both halves gut
`battle.c`; rewriting it twice would be waste.

## Why this fits BG3

BG3's combat is: authored encounters you can see coming (and sometimes avoid),
fought in the space where you stand, with initiative, an action + bonus action
per turn, and dice you watch land. Chrono Trigger is the canonical 16-bit
expression of the first half; the 5e rules core (`rules/`) now provides the
second half. The mosaic-swirl into a separate battle screen was the most FF4
and least BG3 thing in v1 — it goes.

## The model

**Encounter** = a group of field NPCs sharing an activation. Three triggers:
- *Proximity*: patrolling imps aggro on line-of-sight radius. Avoidable.
- *Touch/scripted*: story fights (Lae'zel's imps, the helm) with cutscene
  openings; ambushes give the enemy side a surprise round (5e surprise: you
  simply don't act in round one).
- *Player-initiated*: A-button attack on a visible enemy — party gets the
  surprise round instead.

**Engage**: camera locks to the encounter zone, UI slides in over the bottom
rows (built on the layout system), combatants stay where they stand.
Exploration music ducks into battle music without a scene cut.

**Initiative**: d20 + DEX mod per combatant, ROLLED VISIBLY at engage (small
popups over each head), producing the turn-order strip along the top edge.

**Turn** (5e action economy, honestly):
- Move: shift within the zone — melee walks to its target to swing (CT-style
  approach animation IS the movement); disengaging from melee without the
  Disengage action provokes an opportunity attack. That one rule gives
  positioning teeth without a tile grid.
- Action: Attack / Cast / Dash / Disengage / Dodge / Use item / class action.
- Bonus action: healing word, rogue Cunning Action, Second Wind, hunter's
  mark application, etc. The menu greys out what's spent.

**Dice display**: every resolution shows its math in the message bar —
`d20+5: [14]+5=19 vs AC 13 -- HIT` then `1d8+3: [6]+3=9 slashing` — with a
d20 sprite tumbling beside it. Nat 20 flashes gold; nat 1 thuds. All data
comes recorded from `rules/` (R5Attack carries every die thrown).

**Party**: the three PCs act on their initiative; allied guests (Us, the
helm mind flayer) roll initiative too and act on simple scripts, exactly as
they do today.

**Death**: PCs at 0 HP collapse where they stand (downed, no death saves —
documented deviation), wake on healing/Revivify. Monsters die in place and
stay as field corpses — the room remembers the fight. Defeat = instant retry
of the encounter, as today.

## What stays

- `rules/` is presentation-free: Battle 2.0 is a *consumer*. No game logic
  in the UI layer; anything numeric routes through R5Attack/R5Save.
- Enemies already exist as field sprites with patrol-capable NPC structs.
- The verification harness: encounters must be drivable by the demo flags,
  and the four scenario playthroughs stay the merge gate.

## Cutover plan (main stays finishable at every commit)

1. `encounter.c` lands alongside `battle.c`; one pilot encounter (the deck
   imps) switches to it behind a build flag; both paths verified.
2. Party stats migrate to `R5Creature` (single source of truth); the FF4
   battle reads through a shim during the transition.
3. Remaining encounters convert (thralls, helm — the helm keeps its round
   counter, now literally "rounds" in the 5e sense).
4. FF4 `battle.c` deleted; scenario scripts updated; v2 tag.

## Content implications

- Encounters are authored in room data: spawn groups, patrol paths, aggro
  radii, surprise rules. New optional fights become cheap (wandering imps on
  the deck you can sneak past — reward for stealth: rogue's Hide gets field
  meaning).
- Stat blocks come from generated SRD tables; "Lesser" prologue variants are
  explicit overrides in one file (mirroring what Larian did), not scattered
  constants.
- XP switches to SRD-by-CR with a prologue multiplier tuned so levels land
  where they do today (L2 by the deck, L3 possible by the helm).

## Open items

- Zone size vs. camera: encounters larger than one screen (the helm) —
  camera follows the active combatant; verify readability.
- Enemy movement animation budget: walking a 32x64 Zhalk across the zone —
  cap approach distance, or teleport-step with dust puffs like CT does for
  long moves.
- Initiative strip art (small head icons) — art-agent task once the system
  exists.
