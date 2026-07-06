# The Ravaged Beach — Act 1 begins

> **Status: PROPOSED** (drafted 2026-07-06; ratify, cut, or reshape before
> any stone lands).

The nautiloid burns down into Avernus and the tally promises "THE ADVENTURE
BEGINS ...IN BALDUR'S GATE." This release keeps that promise: wake on the
sand, gather the scattered, brave the crypt, share one night at camp, and
march on the grove gates.

The codebase has been pointing here from several directions at once:

- character2.md tuned XP so level 3 lands *just* out of prologue reach —
  "the hook the Ravaged Beach arc pays off."
- The origin doctrine says "you meet the others on the beach" — Astarion
  and Gale already have portraits, identities, and member_look support.
- The darkvision doctrine names "a beach-arc cave" as its debut room; the
  Everburn Blade is already scripted to become the party's torch.
- *Under Selûne* was written "for the survivors on the Ravaged Beach...
  one quiet night under the moon" — the camp scene's music shipped two
  releases before the camp.
- The devourer stat block (Claws/Jolt) sits unused in overrides.py, and
  the tally flags (Us freed/mutilated, Lae'zel, Shadowheart, Zhalk,
  Everburn) are precisely the world-state a beach save file opens with.

## Scope

**Rooms:** crash beach (2–3 screens), the dune path, chapel ruins, the
crypt (first DARK rooms), the campsite, the grove gates (cliffhanger
tally). **Fights:** intellect devourers on the sand, SRD skeletons below,
a looter band at the chapel door, one set-piece at the gates.
**Recruits:** Astarion on the beach, Gale at the portal rune, Shadowheart
washed ashore if she was left in her pod (BG3-accurate), and Withers as a
service NPC. **Payoff:** the XP budget crosses 900 near the gates — the
level-3 subclass reveals fire on schedule with zero new mechanics.

## The three systems this content forces

1. **Battery save (SRAM).** The game outgrows one sitting, so this is the
   release that adds the cart battery: declare SRAM, serialize Game +
   party5 + bench with a version header, "Continue" on the title. Save at
   camp only — one savepoint keeps the semantics (and the test matrix)
   small. The serializer is a host-test dream: round-trip, version-bump
   rejection, and corrupt-header rejection all run under ASan before any
   ROM boots.
2. **Camp and rest.** The campfire is the long rest: party5_heal_full
   already does the mechanics; camp adds the place, the bench (recruits
   beyond the walking three), CT-style swap, and the one-time *Under
   Selûne* night scene reusing the karaoke tech as story.
3. **Darkvision rooms.** Per the character2.md doctrine, unchanged: DARK
   flag dims the screen, ranged/spell attacks at disadvantage for actors
   without darkvision, devils exempt, the Everburn Blade lights the room,
   and Light/Produce Flame earn their field purpose.

Party stays three on foot + the camp bench (CT-shaped, no churn in
party5/battle layout). Still deferred: race/background pickers, standard
array, Pace/Jump — nothing on the beach demands them.

## Stones, in order (each gate-green, each release-shaped)

1. SRAM plumbing + serializer + Continue, proven by host round-trip tests
   and a save/load scenario before any beach content exists.
2. Beach rooms + devourer encounters, opened from the carried-in tally
   flags (the ending state becomes the world state).
3. Recruits + bench + camp swap menus.
4. The crypt: darkvision debut + Withers (revive, and a re-pick service —
   scope his respec when we get there).
5. Camp night: the rest cycle + the Selûne scene.
6. Grove-gates set piece, XP tuning across the arc, release.

Verification-first, per the testing ladder: every stone's logic lands with
host tests (the serializer and bench-swap are exactly the buffer-overflow
habitat the host layer exists for); each new room-flow gets one scenario;
the ratchets only rise.

## Alternatives considered

- **Finish character creation** (races, backgrounds, standard array):
  completes the sheet but gives players no new place to use it. Fold in
  when a creation-flow revisit demands it.
- **More nautiloid content:** the ship is paced and done; padding dilutes.
- **Pure infrastructure release** (saves + polish, no content): saves
  belong in the same release as the content that makes them necessary.
