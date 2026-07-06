# Combat Engine Invariants

Testable statements the battle engine must never violate. Mined from the
formal Quint specification of SRD 5.2.1 combat at
[dearlordylord/5e-quint](https://github.com/dearlordylord/5e-quint)
(its machine-checked inductive invariants, rule-core definitions, curated
`ASSUMPTIONS.md` rulings, and `battle/REQUIREMENTS.md` catalog), plus SRD text
itself, plus a few `[supplemental]` items reasoned out for this game.

Source tags:

- `[quint:<file>]` — appears in a machine-checked Quint invariant or rule-core
  definition in 5e-quint (paths relative to `packages/shared-algebras/proofs/`
  unless noted).
- `[quint:A#]` — curated ruling in 5e-quint `ASSUMPTIONS.md` (these entries
  formalize what the SRD leaves implicit; the repo's community-Q&A assertion
  lane was retired, and these are its surviving curated output).
- `[quint:R#]` — 5e-quint `battle/REQUIREMENTS.md`, each item cited to SRD
  5.2.1 text.
- `[quint:UL]` — 5e-quint `UBIQUITOUS_LANGUAGE.md` canonical definitions.
- `[SRD]` — direct SRD 5.1/5.2.1 rule text.
- `[supplemental]` — our own carefully-reasoned addition, not found verbatim
  in the sources.

Where SRD 5.1 and 5.2.1 disagree, the difference is noted; the game's data
baseline (see `srd_data.py`) is 5.1 mechanics with 5.2.1 winning on weapon
numbers.

## Scope at the prologue

The engine is prologue-scoped; an invariant about a mechanic the game does
not have is **dormant** — it binds the day the mechanic lands, and no
dormant item below is an oversight:

- **Death saves (25, 27–35): deliberate deviation, not a gap.** PCs at
  0 HP are downed-unconscious with no dying track (docs/battle2.md,
  standing law). Item 23 is the half we keep; 24 holds for monsters.
- Dormant until their mechanics exist: grapple (41), standalone prone
  (44), exhaustion (45), two-weapon fighting (51), Ready/Help flags (in
  50), legendary actions (63), multiattack dispatch (64), upcasting (68),
  readied spells (78).

The rest is live law for rules/ and the encounter engine, asserted by the
rules suite and the host battles.

## Dice and d20 mechanics

1. Advantage rolls two d20 and keeps the higher; disadvantage keeps the lower.
   Both are binary: multiple sources of advantage (or disadvantage) never
   stack. `[quint:UL]`
2. Advantage and disadvantage on the same roll cancel to a single straight
   d20, no matter how many sources of each apply. `[SRD]`
3. A passive check equals `10 + all modifiers`, with advantage counted as +5
   and disadvantage as -5. `[quint:UL]`
4. An ability modifier is exactly `floor((score - 10) / 2)`; scores are
   bounded 1..30. `[quint:UL]`
5. Halving damage always rounds down (integer division), including
   save-for-half results and resistance. `[quint:rule-core/spell-save-gate.qnt]`
6. A damage roll whose modifiers would take it below zero deals 0 damage,
   never negative.
   `[quint:rule-core/damage-component-adjustments.qnt::adjustedDamageRollAmount]`
7. Proficiency bonus is derived from level/CR only (+2 to +6); it is added at
   most once to any given d20 roll, doubled only by expertise on ability
   checks. `[quint:UL]` `[SRD]`

## Attack rolls and critical hits

8. An attack roll is a critical hit if and only if the natural d20 meets or
   exceeds the critical threshold (20, or 19 with an improved-critical
   feature); a critical hit hits regardless of target AC.
   `[quint:rule-core/attack-roll-damage-dice-core.qnt]`
9. A natural 1 on an attack roll always misses, regardless of modifiers and
   target AC. `[quint:rule-core/attack-roll-damage-dice-core.qnt]`
10. A non-natural-1, non-critical attack hits exactly when
    `d20 + modifiers >= AC` (meeting the AC hits). `[SRD]`
11. Critical hits double the number of damage dice rolled — exactly
    `2 x base dice` — and never double flat modifiers.
    `[quint:rule-core/attack-roll-damage-dice-core.qnt::criticalDamageDiceCount]`
12. Extra damage dice riding on a hit (sneak attack, hunter's mark, imp
    poison, charge) are damage dice, so a critical hit doubles them too.
    `[SRD]` `[supplemental: unified rider-dice reading used by the engine]`
13. An attack either hits or misses; on-hit riders apply only on hits, and
    save-gated riders on a hit are resolved after the hit is established.
    `[quint:rule-core/attack-damage-composition.qnt]`

## Damage, resistance, and hit points

14. Per damage type, adjustments apply in this order: immunity zeroes the
    damage; otherwise clamp to >= 0, then resistance halves (round down),
    then vulnerability doubles. Vulnerability applies after resistance.
    `[quint:rule-core/damage-component-adjustments.qnt::damageAmountAfterTargetAdjustments]`
15. Multiple instances of resistance (or vulnerability) to the same damage
    type never stack; each type is adjusted at most once. `[quint:UL]`
16. Total damage dealt equals the sum of the per-type damage amounts after
    adjustment — no damage appears or disappears in aggregation.
    `[quint:rule-core/damage-component-adjustments-inductive.qnt]`
17. A single scalar reduction (Uncanny Dodge, Parry) applied to a mixed-type
    damage roll is capped at the total damage and allocated across the typed
    components deterministically before resistance/vulnerability/immunity are
    applied. `[quint:A43]`
18. Temporary hit points absorb damage first; only the remainder reduces real
    HP. `[quint:rule-core/hit-point-damage.qnt::absorbTemporaryHitPoints]`
19. Temporary hit points are never negative, are not hit points (healing
    cannot restore them), and do not stack — gaining temp HP keeps the higher
    of old and new. `[quint:UL]`
20. Current HP is always within `[0, hit point maximum]`: damage never drives
    HP negative and healing never exceeds the maximum.
    `[quint:rule-core/hit-point-damage.qnt::legalVitals, clampHitPoints]`
21. Healing and damage are no-ops on a dead creature (a dead creature has no
    HP and cannot regain them); temp HP grants are not blocked by death.
    `[quint:A16]`
22. Damage in excess of the HP needed to reach 0 is tracked only to test
    instant death; it never carries over to another creature or a later
    state. `[quint:rule-core/hit-point-damage-inductive.qnt]`

## Death and dying

23. A player character reduced to 0 HP (without instant death) gains the
    unconscious condition; it is not dead.
    `[quint:rule-core/hit-point-damage-inductive.qnt]`
24. A monster reduced to 0 HP dies immediately (the engine may choose the PC
    rules for special monsters, but never both).
    `[quint:rule-core/hit-point-damage-inductive.qnt]`
25. Instant death: if damage reduces a creature to 0 HP and the remaining
    damage is >= its HP maximum, it dies outright.
    `[quint:rule-core/hit-point-damage.qnt]` `[quint:UL]`
26. Dead implies HP == 0, always.
    `[quint:rule-core/hit-point-recovery-inductive.qnt]`
27. Death-save successes and failures are each bounded 0..3 and never
    negative. `[quint:death-saves-algebra-inductive.qnt]`
28. Death saving throws are unmodified d20 rolls: 10+ is a success, 9- is a
    failure, a natural 1 counts as two failures, and a natural 20 restores
    1 HP and consciousness immediately.
    `[quint:death-saves-algebra-inductive.qnt::qResolveDeathSavingThrow]`
29. Three death-save successes make the creature stable; three failures make
    it dead. Stable, dead, and regained-HP are mutually exclusive terminal
    states of the dying process, and further death saves in a terminal state
    change nothing. `[quint:death-saves-algebra-inductive.qnt::qValidState]`
30. Becoming stable (or regaining HP) resets both death-save counters to
    zero. `[quint:death-saves-algebra-inductive.qnt::qValidState]`
31. A stable creature is still at 0 HP and still unconscious; taking any
    damage while stable restarts dying with one (or two, if critical)
    failures. `[quint:rule-core/hit-point-recovery-inductive.qnt]` `[SRD]`
32. Damage taken while at 0 HP adds death-save failures: one normally, two
    from a critical hit, and immediate death if the damage is >= the HP
    maximum. `[quint:rule-core/zero-hit-point-lifecycle.qnt::applyDamageAtZeroHitPoints]`
33. Temp HP absorb damage even at 0 HP; damage fully absorbed by temp HP adds
    no death-save failure. `[quint:rule-core/zero-hit-point-lifecycle.qnt]`
    (5e-quint ruling; RAW is silent on temp HP at 0 HP.)
34. Regaining any HP from 0 ends unconsciousness-from-dying and clears the
    death-save counters; the creature wakes with at least 1 HP.
    `[quint:rule-core/hit-point-recovery-inductive.qnt]`
35. At the start of a turn at 0 HP, the mandatory death save resolves before
    optional start-of-turn effects. `[quint:A6]` (Flagged there as a
    simultaneous-effects modeling choice; RAW would let the dying creature's
    side pick the order.)

## Conditions

36. Paralyzed, petrified, stunned, and unconscious each imply incapacitated;
    the derived incapacitated flag is true exactly when direct incapacitation
    or one of those four is present, and clears when the last cause clears.
    `[quint:conditions-algebra-inductive.qnt::qComputeHasIncapacitated]`
37. Unconscious implies prone (the creature falls prone and drops what it
    holds). `[quint:conditions-algebra-inductive.qnt]`
38. An incapacitated creature can take no actions, no bonus actions, and no
    reactions. `[SRD]` `[quint:UL]` (5.2.1 additionally breaks concentration
    and blocks speech via incapacitated itself.)
39. Conditions are boolean, not stacked: applying a condition a creature
    already has adds nothing; durations are tracked per-effect, and the
    condition ends only when its last source ends.
    `[quint:conditions-algebra-inductive.qnt]` `[SRD]`
40. Immunity to a condition also suppresses the condition's implied effects —
    a creature immune to paralysis is not incapacitated by paralysis.
    `[quint:UL]`
41. Grappled and restrained creatures have speed 0 and cannot benefit from
    any speed bonus; a grapple ends when the grappler is incapacitated or the
    target leaves its reach; while an active grapple holds, effective speed
    is exactly 0.
    `[quint:rule-core/movement-spatial-grapple-inductive.qnt]`
42. Paralyzed, petrified, stunned, and unconscious creatures automatically
    fail Strength and Dexterity saving throws. `[quint:UL]`
43. Any hit against a paralyzed or unconscious creature from within 5 feet is
    a critical hit. `[SRD]`
44. Prone: the creature's own attacks have disadvantage; attacks against it
    have advantage within 5 feet and disadvantage beyond. Standing costs half
    speed, and a creature that cannot pay a nonzero cost cannot stand.
    `[SRD]` `[quint:A17]`
45. Exhaustion is a separate six-level track, not one of the 14 conditions;
    levels are cumulative and level 6 is death. `[quint:UL]`

## Action economy and turns

46. On each turn a creature gets at most one action, at most one bonus
    action, and movement up to its speed; spending is monotone — no quota
    ever goes negative or above its maximum.
    `[quint:action-economy-algebra-inductive.qnt]`
    `[quint:rule-core/action-turn-procedures-inductive.qnt]`
47. Action Surge grants one additional action (never an extra bonus action),
    and its granted action is itself spent at most once.
    `[quint:rule-core/unit-feature-procedure-profiles-inductive.qnt]`
48. Feature uses remaining never exceed the feature's maximum uses, and never
    go negative.
    `[quint:rule-core/unit-feature-procedure-profiles-inductive.qnt]`
49. Sneak Attack applies at most once per turn — any turn, so an opportunity
    attack can carry it on another creature's turn, but never twice in one
    turn. `[quint:rule-core/unit-feature-procedure-profiles-inductive.qnt::qSneakAttackUsedThisTurn]`
50. Dash grants extra movement equal to current speed (after modifiers);
    Disengage, Dodge, Help, Hide, and Ready set flags that expire at their
    stated boundary (start or end of the next turn).
    `[quint:rule-core/action-turn-procedures.qnt]`
51. Two-weapon fighting requires both weapons to be light melee weapons, and
    the extra attack is a bonus action after taking the Attack action.
    `[quint:A8]` (5.2.1 text is laxer; the melee requirement is the curated
    conservative ruling.)
52. Turn structure is strictly sequential: one creature acts at a time; there
    is no simultaneous action resolution. `[quint:R1]`
53. When two effects would resolve at the same moment on a turn, the side
    whose turn it is chooses the order. `[quint:R4]`
54. A round is 6 seconds; durations tick in whole rounds/turns, never
    fractions. `[quint:A4]`

## Initiative and reactions

55. Initiative is rolled once at combat start; the order is fixed for the
    whole encounter and repeats every round. Ties are broken once, not
    re-decided per round. `[quint:R1.1, R1.2]`
56. The initiative list is sorted non-increasing by initiative count, holds
    no duplicate combatants, and advancing past the last combatant increments
    the round counter exactly once and restarts at the top; the round counter
    starts at 1 and never decreases.
    `[quint:initiative-algebra-invariant.qnt::qStackInvariant, nextInitiative]`
57. Dead or unconscious combatants stay in the initiative order until
    explicitly removed; unconscious PCs keep taking their (incapacitated)
    turns for death saves. `[quint:A33]`
58. Each creature gets at most one reaction per round, refreshed at the start
    of its own turn. `[quint:R2]`
59. A reaction resolves immediately after its trigger, except opportunity
    attacks, which resolve right before the mover leaves reach; an
    interrupted turn resumes right after the reaction. `[quint:R2, R5]`
60. Declining an offered reaction window does not consume the reaction.
    `[quint:rule-core/reactions-continuations-concentration.qnt::declineReactionWindow]`
61. Opportunity attacks trigger only when a hostile creature the reactor can
    see leaves its reach using its own movement, action, bonus action,
    reaction, or Speed — never on teleportation or forced movement.
    `[quint:rule-core/movement-spatial-grapple-inductive.qnt::opportunityAttackTriggered]`
62. Reaction nesting is bounded only by the number of creatures with unused
    reactions and matching triggers; the reaction stack resolves innermost
    first. Multiple creatures may react to one trigger, each spending its own
    reaction. `[quint:R3, R30.1]`
63. Legendary actions are taken only when it is NOT the monster's own turn
    (at the end of another creature's turn) and only while legendary uses
    remain; uses replenish at the start of the monster's turn.
    `[quint:rule-core/stat-block-controls-inductive.qnt]`
64. Multiattack sub-attacks are dispatched only on the monster's own turn,
    and the number dispatched never exceeds the multiattack profile; movement
    may interleave between the attacks, other actions may not. `[quint:A44]`
    `[quint:rule-core/stat-block-controls-inductive.qnt]`

## Resources and spellcasting

65. Casting a leveled spell requires and consumes exactly one unexpended slot
    of the chosen level, which must be >= the spell's level; cantrips never
    consume slots. `[quint:rule-core/spell-slot-expenditure.qnt::canExpendSpellSlot]`
66. Slot counts per level are never negative and never exceed the level's
    maximum; expending is the only way down and resting the only way up.
    `[quint:rule-core/spell-slot-expenditure.qnt::legalSpellSlotLedger]`
67. A rejected cast (illegal slot, illegal target count, cannot act) changes
    no state: no slot, no action, no effect.
    `[quint:rule-core/spell-invocation-resource-core.qnt]`
68. Upcasting uses the chosen slot's tier for effects (magic missile gains
    exactly one dart per slot level above 1st; sleep gains 2d8 per level).
    `[quint:rule-core/spell-procedure-profiles.qnt]` `[SRD]`
69. Target cardinality is enforced per spell profile: a one-target spell
    affects one target; bless affects at most 3 + upcast targets; magic
    missile darts may repeat a target but total darts are fixed by slot.
    `[quint:rule-core/spell-invocation-target-cardinality-core.qnt]`
70. Expending a spell slot or starting concentration requires being able to
    act: alive and not incapacitated. `[quint:A1]`
71. A creature concentrates on at most one effect; starting a new
    concentration effect immediately ends the previous one (a lock, not a
    stack). `[quint:UL]`
    `[quint:rule-core/reactions-continuations-concentration.qnt]`
72. Taking damage while concentrating forces a Constitution save at
    DC = max(10, floor(damage / 2)); failure ends concentration. 5.2.1 caps
    the DC at 30 (5.1 has no cap). Each separate damage event forces a
    separate save.
    `[quint:rule-core/reactions-continuations-concentration.qnt::concentrationSavingThrowDc]`
73. Becoming incapacitated or dying ends concentration immediately, no save.
    `[quint:rule-core/reactions-continuations-concentration.qnt::breakConcentrationIfPrevented]`
74. When concentration ends, every effect the spell placed on every target
    ends at once (bless bonuses vanish, hunter's mark drops, restrained-by-
    ensnaring-strike ends). `[quint:R33]`
75. Save-or-half spells: a failed save takes full rolled damage plus riders;
    a successful save takes exactly half rounded down and no riders.
    Save-negates spells deal nothing on a success. A nonpositive damage roll
    deals 0 either way. `[quint:rule-core/spell-save-gate.qnt]`
76. Spell save DC is caster-owned (8 + proficiency + casting modifier); the
    target rolls against it. AC and attack-roll reactions (Shield, Parry)
    never apply to save-based spells. `[quint:R31]`
77. An AoE spell spends the caster's action and slot once; every creature in
    the area saves and takes damage independently, allies included unless
    the spell says otherwise. `[quint:R32]`
78. A readied spell expends its slot when readied, is held by concentration,
    and is lost with the slot still spent if concentration breaks or the
    trigger never occurs before the start of the caster's next turn.
    `[quint:R6]`
79. Rangers have no spell slots at level 1; their slots begin at class level
    2. `[SRD]` (dnd5eapi class tables; also 5e-quint class records.)
80. Healing word and cure wounds cannot heal constructs or undead (5.1
    baseline). `[SRD]`

## Game-data sanity (supplemental, for the C generator)

81. Every versatile weapon has a versatile die exactly one step larger than
    its one-handed die. `[supplemental]` (Holds for all SRD versatile
    weapons; cheap generator assert.)
82. Every weapon with the thrown or ammunition property has a (normal, long)
    range with 0 < normal < long. `[supplemental]`
83. A monster's average HP equals the mean of its hit-dice expression rounded
    down (e.g. imp 3d4+3 -> 10). `[supplemental]`
84. For every stat-block attack with an attack roll in `srd_data.py`, to-hit
    equals one of the monster's ability modifiers (STR, DEX, or its casting
    ability) plus its proficiency bonus, and the flat damage bonus is either
    0 or that same modifier. `[supplemental]`
85. Any condition named in a spell/monster rider exists in CONDITIONS, and
    every "implies" target exists in CONDITIONS. `[supplemental]`
