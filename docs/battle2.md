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
`d20+5: [14]+5=19 vs AC 13 -- HIT` then `1d8+3: [6]+3=9 slashing` — with the
headline d20 sprite *tumbling* into place beside it: the face spins a beat,
then settles on the rolled value (both dice spinning together on advantage or
disadvantage). A crit's d20 settles gold. Only the "does it land" die tumbles
— the damage, bless, and rider dice appear as thrown, which keeps the drama on
the meaningful roll and bounds the added frames. The tumble is presentational
only: the value is decided in `rules/` first (R5Attack carries every die
thrown), then animated toward with a throwaway counter, so nothing on screen
can move a result. The same primitive draws the field skill-check die
(`src/dice_ui.c`, shared with `events.c`).

**Party**: the three PCs act on their initiative; allied guests (Us, the
helm mind flayer) roll initiative too and act on simple scripts, exactly as
they do today.

**Death**: PCs at 0 HP collapse where they stand (downed, no death saves —
documented deviation), wake on healing/Revivify. Monsters die in place and
stay as field corpses — the room remembers the fight. Defeat = instant retry
of the encounter, as today.

## Standing law (the rest of this doc is rationale; this part binds)

- `rules/` is presentation-free: Battle 2.0 is a *consumer*. No game logic
  in the UI layer; anything numeric routes through R5Attack/R5Save.
- Encounters must be drivable by the demo flags; behavior is pinned in
  test/host, structure by the scenario fleet (the testing ladder in
  CLAUDE.md).
- **Documented deviation from 5e:** PCs at 0 HP are downed where they
  stand — no death saves. They wake on healing/Revivify; a full party
  down is an instant retry. tools/srd/INVARIANTS.md marks the death-save
  items dormant on this basis.
- Stat blocks come from generated SRD tables; "Lesser" prologue variants
  are explicit overrides in one file (mirroring what Larian did), not
  scattered constants.

The cutover plan that used to live here was executed in full (pilot
encounter behind a flag → R5Creature migration → helm conversion →
battle.c deleted); git history has it.
