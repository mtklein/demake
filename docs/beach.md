# The Ravaged Beach — Act 1 begins

> **Status: RATIFIED 2026-07-06.** Scope set in conversation: the full
> stretch from the crash to the grove-gates battle as finale. No save
> system (emulator save states cover a game this size; revisit only if
> playtime badly outgrows a sitting). Party stays three on foot,
> CT-style, with **swap available any time out of combat** — not gated
> to camp.

The nautiloid burns down into Avernus and the tally promises "THE ADVENTURE
BEGINS ...IN BALDUR'S GATE." This release keeps that promise: wake alone on
the sand, re-gather the scattered, brave the crypt, share one night at
camp, and hold the grove gates.

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
  Everburn) are precisely the world-state the beach opens from.

## Scope

The crash scatters everyone — you wake alone, and rebuilding the party is
the arc's spine. Ship flags color each reunion, they don't skip it.

- **Beach:** the crash site and dunes; **intellect devourers** on the
  sand; **the dying mind flayer** in the wreckage — a story beat with an
  INT save through the field-check system (approach and it grasps at your
  mind), kill it or leave it to die.
- **Recoveries:** Shadowheart ashore (saved on ship: she wakes near you;
  left behind: found unconscious, a Medicine check wakes her), Lae'zel
  caged by scared tiefling scavengers on the dune path (ship-ally or not
  shades the scene).
- **Recruits:** Astarion by the shore (his knife-at-your-throat beat),
  Gale at the portal rune. An origin-played companion doesn't appear as a
  recruit; their beat reroutes (same story-surgery doctrine as the ship).
- **Chapel + crypt:** bandits arguing outside the tomb door; inside, the
  first DARK rooms, skeleton ambushes, the guardian set piece as far as
  it will stretch, and **Withers**, who wakes to offer revival and
  re-picks (exact respec depth scoped when the stone lands).
- **Camp:** a campsite room; the one-time *Under Selûne* night scene
  (karaoke tech as story); the campfire is the long rest thereafter
  (party5_heal_full + refresh — mechanics already exist).
- **Finale (ratified 2026-07-06, second pass):** the battle at the grove
  gates, with **Zevlor** holding the door and **Wyll** introduced
  mid-fight — the Blade of Frontiers is exactly who'd be out front. Wyll
  fights the battle as an ally (the helm's side-2 machinery) and joins
  after it: the roster's **sixth soul** (RESERVE_MAX grows 2 → 3 when he
  lands; the "five souls" comments in game.h/data.c update with it).
  Origin-played Wyll reroutes per the story-surgery doctrine. Then the
  tally returns with the arc's accounting, and the release.
  **The grove interior is a different release entirely** — a settlement
  hub, not a gauntlet; nothing this arc builds should presume its shape.

## The two systems this content forces

1. **Roster + swap.** The walking party stays G.pm[3]/party5[3]; a
   reserve roster joins Game. Tav is locked in slot 0; slots 1–2 swap
   from reserve through a party screen reachable from the Start menu
   **any time out of combat**. Reserve members refresh on rest like
   everyone else. This is pure menu/data logic — it lands first, proven
   by host tests with a fabricated five-soul roster before any beach
   content exists.
2. **Darkvision rooms.** Per the character2.md doctrine, unchanged: DARK
   flag dims the screen, ranged/spell attacks at disadvantage for actors
   without darkvision, devils exempt, the Everburn Blade lights the room,
   and Light/Produce Flame earn their field purpose.

Still deferred: race/background pickers, standard array, Pace/Jump —
nothing on the beach demands them. No SRAM, no Continue screen.

## Stones, in order (each gate-green, each release-shaped)

1. Roster + swap: data model, party screen, host tests (swap in/out,
   Tav locked, stats/damage/slots survive the bench round-trip).
2. Beach rooms opened from the carried-in tally flags: crash site, the
   Shadowheart and Lae'zel recoveries, devourer fights, the dying mind
   flayer beat.
3. Astarion and Gale recruit beats — the roster outgrows three and swap
   earns its keep.
4. Chapel bandits; the crypt: darkvision debut, skeletons, Withers.
5. Camp night: the *Under Selûne* scene + the long-rest cycle.
6. The grove-gates battle — Zevlor at the door, Wyll allied in the fight
   and recruited after (roster to six) — XP tuning across the arc
   (gates ≈ 900), the tally's return, release.

Verification-first, per the testing ladder: every stone's logic lands with
host tests (roster/bench swap is exactly the buffer-overflow habitat the
host layer exists for); each new room-flow gets one scenario; the ratchets
only rise.
