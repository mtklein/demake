# Character 2.0 — twelve classes, subclasses, choices, origins

> **Status: IN PROGRESS.** This doc pins the scope and architecture; the
> stones land bottom-up (data → rules → creation/level-up UI → origins → art
> → gate). Main stays finishable after every stone.

The player character grows from "pick one of four classes" to the full BG3
prologue promise: **all twelve classes, at least three subclasses each,
leveling with real choices, prepared-caster respec, and the origin
characters — including The Dark Urge.**

## Scope decisions

- **Character levels 1–3, spell levels 0–2.** "Up to level 3" pins the whole
  matrix: subclass features at their 5.1 levels, second-level slots at
  character 3. The existing CLASSES tables already stop at 3; we stay on the
  SRD 5.1 progression (subclasses at 1 for cleric/sorcerer/warlock, 2 for
  druid/wizard, 3 for the rest).
- **XP thresholds are the 5e table: 300 → 2, 900 → 3.** A thorough prologue
  run earns ~700xp — level 2 comfortably, level 3 *just* out of reach. That
  is deliberate: level 3 (and most subclass reveals) is the hook the Ravaged
  Beach arc pays off.
- **Subclass licensing — the piscodemon doctrine, ratified for classes.**
  Each class gets its SRD 5.1 subclass verbatim (CC-BY-4.0, attributed) plus
  two homebrew subclasses: mechanically *adjacent* to beloved non-SRD
  designs, but original names and original text, features built strictly
  from engine primitives. Never a copied stat block or feature text.
- **Origins.** Six fixed-persona origins (Astarion/rogue, Gale/wizard,
  Karlach/barbarian, Lae'zel/fighter, Shadowheart/cleric, Wyll/warlock) plus
  The Dark Urge, which is the full custom flow (any class, any choices) with
  intrusive-thought interjections at key story beats. Only Lae'zel and
  Shadowheart exist on the ship as NPCs, so only they need story surgery
  when player-selected: their recruitment beats are replaced, their party
  slot stays empty for the prologue (BG3-accurate: you meet the others on
  the beach).

## Rulesets and precedence

Three real rulesets orbit this game; the amalgam is explicit:

1. **SRD 5.1 (the 2014 rules) is the chassis.** All progression, subclass
   timing (cleric domains and warlock patrons at level 1, wizard/druid at 2,
   the rest at 3), slots, and feature mechanics. CC-BY-4.0, attributed.
2. **BG3-isms are adopted deliberately, one at a time, and listed here.**
   Current list: **bonus-action potions** (Larian homebrew that the 2024
   rules later made RAW — the most defensible tweak in the pile).
3. **SRD 5.2.1 (the 2024 revision, colloquially 5.5e) is cross-check and
   gap-filler only** — used where 5.1 is silent (Ensnaring Strike precedent)
   with divergences noted inline. Its biggest structural change (all
   subclasses at level 3) is explicitly NOT adopted; Shadowheart being a
   Sharran trickery cleric from level 1 is the whole point of 5.1 timing.
4. **Homebrew fills non-SRD gaps** under the piscodemon doctrine (original
   names, original text, engine-primitive mechanics).

## Origins: class and subclass identity

| Origin | Class | Subclass | Default |
|---|---|---|---|
| Astarion | Rogue | at 3 | assassin-shape |
| Gale | Wizard | at 2 | Evocation (SRD) |
| Karlach | Barbarian | at 3 | Berserker (SRD) |
| Lae'zel | Fighter | at 3 | Champion (SRD) |
| Shadowheart | Cleric | **at 1** | trickery-shape (Shar) |
| Wyll | Warlock | **at 1** | The Fiend (SRD — Mizora) |
| Dark Urge | any | per class | storm-shape sorcerer suggested |

Origin Lae'zel gets a **githyanki warrior companion for the nautiloid
only** — he fills her usual slot in the deck fight (her sprite sheet,
palette-swapped; fighter kit; terse) and is gone by the beach. Origin
Shadowheart's pod stands empty; the rune beat reroutes.

Wild Shape presentation note: the beast is a replacement actor with its own
HP pool that hands back the druid at 0 — mechanically closer to FFX's Aeons
than to anything else in the 16-bit canon, and staged like a summon
(entrance flash when the form takes over).

## Data model

- `PMember` grows: `u8 subclass;` `u8 choice[4];` (class-indexed meanings:
  fighting style / expertise / invocations / metamagic / pact boon),
  `u32 known;` (spells-known bitmask into the class spell list — every list
  fits in 32 at these levels), `u32 prepared;` (prepared casters only).
- `R5Creature` grows a small resource block: `u8 rage, ki, sorc_pts, lay_hp,
  channel, wildshape, pact_slots;` — flat counters, refreshed by rest rules
  (pact slots and ki on short rest = resto pod; the rest on long rest).
- Spell lists per class are generated tables (`srd_tables.c`) with per-spell
  mechanics records the encounter engine can execute: attack / save-half /
  save-negate / heal / buff-rider / condition / multi-bolt / hp-pool, plus
  concentration and target shape.

## New mechanics budget (levels 1–3 only)

Rage (+dmg, resistance, Reckless advantage both ways) · Bardic Inspiration
(die gifted, spent on a roll) · Channel Divinity (domain-keyed) · Wild Shape
(the druid's R5Creature swaps to a beast stat block — an HP pool that reverts
at 0; beasts come from the monster pipeline) · Martial Arts + ki (Flurry /
Patient Defense / Step of Wind) · Lay on Hands (pool) + Divine Smite (slot →
radiant rider) · Font of Magic (points ↔ slots) + 3 metamagics · Pact Magic
(short-rest slots) + Eldritch Blast with Agonizing/Repelling invocations ·
prepared-vs-known casting, domain/oath spells always-prepared.

Everything lands in `rules/` first with property tests; the encounter engine
consumes records, not special cases (the Everburn rider pattern).

## UI flows

- **Creation:** origin picker → (Dark Urge or custom) class → level-1
  choices. Origins skip straight to confirmation with fixed sheets.
- **Level-up:** fires between fights (field, at rest, or post-battle
  prompt): HP roll shown as a visible die, then the level's choice screens.
- **Prepare:** party-menu screen for cleric/wizard/druid/paladin — swap
  prepared spells anywhere outside combat. Known-list casters see a
  read-only spellbook.

## Verification

The 7 story scenarios stay. New: one deck-fight smoke per class (12) that
exercises the kit's signature mechanic (rage damage, smite rider, wild
shape swap, pact-slot EB…), plus creation-flow scenarios for one origin and
a Dark Urge custom. All in `make gate`. Choice flows get poked-buffer demo
coverage exactly like dialog choices do today (G_CHOICE_BUF pattern).

## Stones, in order

1. Data: six new classes + 12 SRD subclasses + spell list (agent-extracted,
   staged, merged with provenance notes).
2. Homebrew subclass drafts (staged → overrides.py after review).
3. Rules core: resources + mechanics + tests (native suite grows).
4. PMember/creation/level-up/prepare UI.
5. Origins + Dark Urge content pass.
6. Art fan-out (class sprites, origin portraits).
7. Gate expansion; README; release when player-visible.
