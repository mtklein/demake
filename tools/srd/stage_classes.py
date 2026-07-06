# stage_classes.py -- STAGED SRD extraction: the six classes missing from
# srd_data.CLASSES, plus all twelve SRD subclasses, levels 1-3.  Review,
# then merge into srd_data.py (character2.md, stone 1).
#
# Plain Python literals only: no imports, no dataclasses.  This file feeds
# the same C code generator as srd_data.py.
#
# Provenance (see tools/srd/ATTRIBUTION.md for legal text):
#   CLASSES_NEW - SRD 5.1 (dnd5eapi /api/2014/classes/*/levels, fetched
#                 2026-07-06).  hit_die/saves from the class records;
#                 per-level numbers from the class_specific and
#                 spellcasting blocks, feature lists from the level
#                 records, mechanics comments from /api/2014/features/*.
#   SUBCLASSES  - SRD 5.1 (dnd5eapi /api/2014/subclasses/*/levels and
#                 /api/2014/features/*).  Domain/oath/circle/patron bonus
#                 spells from the subclass "spells" prerequisite records.
#   5.2.1 notes - structural placement verified against dnd5eapi
#                 /api/2024/features/* level indices (the 2024 levels
#                 endpoint is unimplemented); numeric 5.2.1 details are
#                 from the published 5.2.1 text and marked inline.
#
# Schema matches srd_data.CLASSES exactly ("saves", not "save_prof");
# dice strings are "NdS"; mod = (score - 10) // 2.

# --------------------------------------------------------------------------
# CLASSES_NEW (levels 1-3, SRD 5.1 progression)
#
# hit_die, saves, prof_bonus per level, feature names per level (SRD
# names), spell_slots for casters, cantrips_known / spells_known where the
# class has them, plus per-class resource columns (rages, ki_points,
# sorcery_points, lay_on_hands, wild_shape, pact_slots).
#
# SRD 5.2.1 differences (not encoded): every class picks its subclass at
# level 3.  Barbarian 1 adds Weapon Mastery, 3 adds Primal Knowledge, and
# one spent rage returns on a short rest; Druid 1 gains Primal Order,
# Druid 2 adds Wild Companion, and Wild Shape becomes a bonus action;
# Monk's Martial Arts die starts at d6 (not d4), Ki is renamed Monk's
# Focus, 2 adds Uncanny Metabolism, 3 replaces Deflect Missiles with
# Deflect Attacks; Paladin 1 gains Spellcasting (slots from level 1) and
# Weapon Mastery, loses Divine Sense (a Channel Divinity option at 3
# instead), 2 replaces Divine Smite with Paladin's Smite, 3 drops Divine
# Health, and Lay on Hands becomes a bonus action; Sorcerer 1 gains
# Innate Sorcery and Metamagic moves to 2; Warlock invocations start at
# level 1, 2 adds Magical Cunning, and Pact Boons become invocations.
# --------------------------------------------------------------------------

CLASSES_NEW = {
    "barbarian": {
        "hit_die": 12,
        "saves": ["str", "con"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Rage", "Unarmored Defense"],
            2: ["Reckless Attack", "Danger Sense"],
            3: ["Primal Path"],
        },
        # Rage: bonus action; while raging (and not in heavy armor):
        # advantage on STR checks and saves, +rage_bonus damage on
        # STR-based melee weapon attacks, resistance to bludgeoning/
        # piercing/slashing; no spellcasting or concentration.  Lasts 1
        # minute; ends early if unconscious, or if a turn passes without
        # attacking or taking damage.  All uses return on a long rest.
        "rages": {1: 2, 2: 2, 3: 3},
        "rage_bonus": {1: 2, 2: 2, 3: 2},
        # Unarmored Defense: AC = 10 + DEX mod + CON mod; shield allowed.
        # Reckless Attack: on your first attack of the turn, advantage on
        # STR melee weapon attacks this turn, but attacks against you
        # have advantage until your next turn.
        # Danger Sense: advantage on DEX saves vs effects you can see
        # (lost while blinded, deafened, or incapacitated).
    },
    "druid": {
        "hit_die": 8,
        "saves": ["int", "wis"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Druidic", "Spellcasting"],
            2: ["Wild Shape", "Druid Circle"],
            3: [],  # 2nd-level slots only; circle spells expand
        },
        "spell_slots": {1: {1: 2}, 2: {1: 3}, 3: {1: 4, 2: 2}},
        "cantrips_known": {1: 2, 2: 2, 3: 2},
        # prepared caster: prepares WIS mod + druid level spells daily
        # Wild Shape: action; become a beast you have seen; take the
        # beast's stat block (keep INT/WIS/CHA and proficiencies), swap
        # to its HP, revert to prior HP at 0 with excess damage carrying
        # over; no spellcasting in beast form.  Two uses, BOTH regained
        # on a short or long rest.  Beast ceiling by druid level: 2-3
        # cap at CR 1/4 with no flying or swimming speed (per SRD text;
        # CR 1/2 no fly at 4, CR 1 at 8 -- out of scope).
        "wild_shape": {
            1: None,
            2: {"uses": 2, "max_cr": "1/4", "fly": False, "swim": False},
            3: {"uses": 2, "max_cr": "1/4", "fly": False, "swim": False},
        },
    },
    "monk": {
        "hit_die": 8,
        "saves": ["str", "dex"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Unarmored Defense", "Martial Arts"],
            2: ["Ki", "Flurry of Blows", "Patient Defense",
                "Step of the Wind", "Unarmored Movement (+10 ft)"],
            3: ["Monastic Tradition", "Deflect Missiles"],
        },
        # Unarmored Defense: AC = 10 + DEX mod + WIS mod; no shield.
        # Martial Arts (while unarmored, using unarmed strikes or monk
        # weapons = shortsword + simple melee without two-handed/heavy):
        # DEX may replace STR for attack and damage; damage die may be
        # replaced by martial_arts_die; one unarmed strike as a bonus
        # action after taking the Attack action with these.
        "martial_arts_die": {1: "1d4", 2: "1d4", 3: "1d4"},
        # Ki (from 2): points refresh on a short or long rest; ki save
        # DC = 8 + prof + WIS mod.  Spend 1 point to: Flurry of Blows
        # (two unarmed strikes as a bonus action after the Attack
        # action), Patient Defense (Dodge as a bonus action), or Step of
        # the Wind (Disengage or Dash as a bonus action, jump doubled).
        "ki_points": {1: 0, 2: 2, 3: 3},
        # Deflect Missiles: reaction when hit by a ranged weapon attack;
        # reduce the damage by 1d10 + DEX mod + monk level; if reduced
        # to 0, may catch the missile and (1 ki) throw it back as part
        # of the same reaction (monk-weapon attack, range 20/60).
    },
    "paladin": {
        "hit_die": 10,
        "saves": ["wis", "cha"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Divine Sense", "Lay on Hands"],
            2: ["Fighting Style", "Spellcasting", "Divine Smite"],
            3: ["Divine Health", "Sacred Oath",
                "Channel Divinity (1/rest)"],
        },
        # Paladins get NO slots (and no Spellcasting) at level 1.
        "spell_slots": {1: {}, 2: {1: 2}, 3: {1: 3}},
        # prepared caster: prepares CHA mod + half paladin level
        # (rounded down, min 1) spells daily
        # Lay on Hands: action, touch; spend any amount of the pool to
        # restore that many HP (or 5 to cure one disease or poison);
        # pool refills on a long rest; no undead or constructs.
        "lay_on_hands": {1: 5, 2: 10, 3: 15},  # pool = 5 x paladin level
        # Divine Sense: action, 1 + CHA mod uses per long rest; locate
        # celestials/fiends/undead within 60 ft until end of next turn.
        # Divine Smite: on a melee weapon hit, expend a spell slot for
        # +2d8 radiant, +1d8 per slot level above 1st (max 5d8), +1d8 if
        # the target is undead or a fiend.
        # Divine Health: immune to disease.
        # The API also lists Oath Spells at 3; that rule rides the
        # Devotion entry in SUBCLASSES, as cleric's domain spells do.
    },
    "sorcerer": {
        "hit_die": 6,
        "saves": ["con", "cha"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Spellcasting", "Sorcerous Origin"],
            2: ["Font of Magic"],
            3: ["Metamagic"],
        },
        "spell_slots": {1: {1: 2}, 2: {1: 3}, 3: {1: 4, 2: 2}},
        "cantrips_known": {1: 4, 2: 4, 3: 4},
        "spells_known": {1: 2, 2: 3, 3: 4},
        # Font of Magic: sorcery points refresh on a long rest.
        # Flexible Casting, bonus action: points -> slot (1st-level slot
        # = 2 points, 2nd = 3); or sacrifice a slot -> points equal to
        # the slot's level.
        "sorcery_points": {1: 0, 2: 2, 3: 3},
        # Metamagic: choose 2 options at 3 (SRD 5.1 has all eight:
        # Careful/Distant/Empowered/Extended/Heightened/Quickened/
        # Subtle/Twinned).
    },
    "warlock": {
        "hit_die": 8,
        "saves": ["wis", "cha"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Otherworldly Patron", "Pact Magic"],
            2: ["Eldritch Invocations"],
            3: ["Pact Boon"],
        },
        # Pact Magic is NOT standard slots: every pact slot is cast at
        # the same (highest) level, and ALL of them refresh on a SHORT
        # or long rest.  At warlock 3 the two slots become 2nd level --
        # there are no 1st-level slots to fall back on.
        "pact_slots": {1: {1: 1}, 2: {1: 2}, 3: {2: 2}},
        "cantrips_known": {1: 2, 2: 2, 3: 2},
        "spells_known": {1: 2, 2: 3, 3: 4},
        # Invocations (from 2, retrainable on level-up) are a separate
        # extraction; the design doc wants Agonizing Blast and Repelling
        # Blast, and eldritch blast is not in srd_data.SPELLS yet.
        "invocations_known": {1: 0, 2: 2, 3: 2},
        # Pact Boon at 3: Pact of the Blade / the Chain / the Tome.
    },
}

# --------------------------------------------------------------------------
# SUBCLASSES (the twelve SRD 5.1 subclasses, one per class; levels 1-3)
#
# "level" is the class level at which the subclass is chosen (SRD 5.1:
# 1 for cleric/sorcerer/warlock, 2 for druid/wizard, 3 for the rest --
# SRD 5.2.1 moves ALL twelve choices to class level 3).  "features"
# always carries keys 1-3; levels before the choice, or without gains,
# are empty lists.  Each feature is {"name", "srd", "mech"}; mech kinds:
#
#   passive_bonus   always-on modifier (what / amount)
#   resource        activated ability fueled by a pool (pool / refresh /
#                   effect); channel_divinity pools refresh on short rest
#   rider           extra effect hung on an existing action (trigger /
#                   dice / type / effect ...)
#   advantage,      grant / impose (on what) -- unused by these twelve
#   impose_dis      at 1-3, reserved for the homebrew subclass stage
#   reaction        (trigger / effect)
#   always_prepared spells by CLASS level, always prepared, not counted
#                   against the prepared limit
#   spell_list_extend  adds options to a known-caster's class spell list
#                   (the Fiend's Expanded Spell List is NOT auto-known or
#                   auto-prepared; no base kind fits, so this kind is
#                   introduced here -- flag at merge review)
#   heal_bonus      (amount / trigger)
#   crit_range      (min_roll)
#   extra_prof      (what)
#   flavor          real SRD feature, out of scope for level-3 combat
#
# Spells referenced below but not yet in srd_data.SPELLS: lesser
# restoration, spiritual weapon, invisibility, pass without trace,
# protection from evil and good, sanctuary, command, blindness/deafness,
# scorching ray.  The SPELLS table must grow with the merge.
# --------------------------------------------------------------------------

SUBCLASSES = {
    # Path of the Berserker (barbarian 3).  5.2.1: Frenzy instead adds
    # extra rage-damage dice to Reckless Attacks and drops the
    # exhaustion cost.
    "berserker": {
        "class": "barbarian", "level": 3,
        "features": {
            1: [],
            2: [],
            3: [
                {"name": "Frenzy", "srd": True,
                 "mech": {"kind": "rider",
                          "trigger": "while frenzy-raging: bonus action "
                                     "on each turn after the one you "
                                     "entered the rage",
                          "effect": "one melee weapon attack",
                          "cost": "one level of exhaustion when the "
                                  "rage ends"}},
            ],
        },
    },
    # College of Lore (bard 3).  5.2.1 keeps both features at 3 but lets
    # Cutting Words react to a roll already known to have succeeded.
    "lore": {
        "class": "bard", "level": 3,
        "features": {
            1: [],
            2: [],
            3: [
                {"name": "Bonus Proficiencies", "srd": True,
                 "mech": {"kind": "extra_prof",
                          "what": "three skills of your choice"}},
                {"name": "Cutting Words", "srd": True,
                 "mech": {"kind": "reaction",
                          "trigger": "creature you can see within 60 ft "
                                     "makes an attack roll, ability "
                                     "check, or damage roll",
                          "effect": "expend one Bardic Inspiration use; "
                                    "roll the die and subtract it from "
                                    "the creature's roll",
                          "dice": "1d6",  # the Bardic Inspiration die
                          "limits": "declared after the roll, before "
                                    "the outcome; wasted if the target "
                                    "can't hear you or is charm-"
                                    "immune"}},
            ],
        },
    },
    # Life Domain (cleric 1).  5.2.1 moves the whole domain to cleric 3,
    # drops the heavy-armor grant (armor training comes from Divine
    # Order: Protector instead), and keeps Disciple of Life and
    # Preserve Life with the same numbers.
    "life": {
        "class": "cleric", "level": 1,
        "features": {
            1: [
                {"name": "Domain Spells", "srd": True,
                 "mech": {"kind": "always_prepared",
                          "spells": {1: ["bless", "cure wounds"],
                                     3: ["lesser restoration",
                                         "spiritual weapon"]}}},
                {"name": "Bonus Proficiency", "srd": True,
                 "mech": {"kind": "extra_prof", "what": "heavy armor"}},
                {"name": "Disciple of Life", "srd": True,
                 "mech": {"kind": "heal_bonus",
                          "amount": "2 + spell level",
                          "trigger": "a level 1+ spell you cast "
                                     "restores HP to a creature"}},
            ],
            2: [
                {"name": "Channel Divinity: Preserve Life", "srd": True,
                 "mech": {"kind": "resource",
                          "pool": "channel_divinity",
                          "refresh": "short",
                          "amount": "5 x cleric level",
                          "effect": "action: restore that many HP split "
                                    "among creatures within 30 ft, none "
                                    "above half its HP max; no undead "
                                    "or constructs"}},
            ],
            3: [],
        },
    },
    # Circle of the Land (druid 2).  Land type pinned: GRASSLAND -- the
    # most combat-neutral of the seven lists at these levels
    # (invisibility + pass without trace; coast would pull in the
    # non-SRD-encoded mirror image + misty step, arctic hold person +
    # spike growth, etc.).  5.2.1 rebuilds the circle (choice at 3,
    # land type re-picked per long rest, Land's Aid) and moves Natural
    # Recovery to druid 6.
    "land": {
        "class": "druid", "level": 2,
        "features": {
            1: [],
            2: [
                {"name": "Bonus Cantrip", "srd": True,
                 "mech": {"kind": "passive_bonus",
                          "what": "cantrips_known", "amount": 1}},
                {"name": "Natural Recovery", "srd": True,
                 "mech": {"kind": "resource",
                          "pool": "spell_slots", "refresh": "long",
                          "effect": "once per day during a short rest, "
                                    "recover expended slots totaling "
                                    "<= half druid level rounded up "
                                    "(= one 1st-level slot at druid "
                                    "2-3), none 6th level or higher"}},
            ],
            3: [
                {"name": "Circle Spells (grassland)", "srd": True,
                 "mech": {"kind": "always_prepared",
                          "spells": {3: ["invisibility",
                                         "pass without trace"]}}},
            ],
        },
    },
    # Champion (fighter 3).  5.2.1 keeps Improved Critical at 3 and
    # adds Remarkable Athlete.
    "champion": {
        "class": "fighter", "level": 3,
        "features": {
            1: [],
            2: [],
            3: [
                {"name": "Improved Critical", "srd": True,
                 "mech": {"kind": "crit_range", "min_roll": 19}},
            ],
        },
    },
    # Way of the Open Hand (monk 3).  5.2.1 renames the options (Addle /
    # Push / Topple); same Flurry-of-Blows trigger.
    "open hand": {
        "class": "monk", "level": 3,
        "features": {
            1: [],
            2: [],
            3: [
                {"name": "Open Hand Technique", "srd": True,
                 "mech": {"kind": "rider",
                          "trigger": "each Flurry of Blows hit; choose "
                                     "one effect",
                          "dc": "ki (8 + prof + WIS mod)",
                          "options": [
                              {"save": "dex",
                               "effect": "knocked prone"},
                              {"save": "str",
                               "effect": "pushed up to 15 ft away"},
                              {"save": None,
                               "effect": "no reactions until the end "
                                         "of your next turn"},
                          ]}},
            ],
        },
    },
    # Oath of Devotion (paladin 3).  5.2.1 keeps the oath spells and
    # Sacred Weapon (as a bonus action) at 3 and drops Turn the Unholy.
    "devotion": {
        "class": "paladin", "level": 3,
        "features": {
            1: [],
            2: [],
            3: [
                {"name": "Oath Spells", "srd": True,
                 "mech": {"kind": "always_prepared",
                          "spells": {3: ["protection from evil and good",
                                         "sanctuary"]}}},
                {"name": "Channel Divinity: Sacred Weapon", "srd": True,
                 "mech": {"kind": "resource",
                          "pool": "channel_divinity",
                          "refresh": "short",
                          "effect": "action: one held weapon gains "
                                    "+CHA mod (min +1) to attack rolls "
                                    "for 1 minute, glows (bright light "
                                    "20 ft), and counts as magical"}},
                {"name": "Channel Divinity: Turn the Unholy",
                 "srd": True,
                 "mech": {"kind": "resource",
                          "pool": "channel_divinity",
                          "refresh": "short",
                          "save": "wis",  # paladin spell DC
                          "effect": "action: each fiend or undead "
                                    "within 30 ft that can see or hear "
                                    "you saves or is turned (flees, no "
                                    "reactions) for 1 minute or until "
                                    "it takes damage"}},
            ],
        },
    },
    # Hunter (ranger 3).  Hunter's Prey: pick ONE of the three options.
    # 5.2.1 keeps Colossus Slayer and Horde Breaker (re-pickable on a
    # rest), drops Giant Killer, and adds Hunter's Lore.
    "hunter": {
        "class": "ranger", "level": 3,
        "features": {
            1: [],
            2: [],
            3: [
                {"name": "Hunter's Prey: Colossus Slayer", "srd": True,
                 "mech": {"kind": "rider",
                          "dice": "1d8", "type": "weapon",
                          "trigger": "once per turn, weapon hit against "
                                     "a creature below its HP max"}},
                {"name": "Hunter's Prey: Giant Killer", "srd": True,
                 "mech": {"kind": "reaction",
                          "trigger": "a Large or larger creature within "
                                     "5 ft hits or misses you with an "
                                     "attack",
                          "effect": "one weapon attack against it"}},
                {"name": "Hunter's Prey: Horde Breaker", "srd": True,
                 "mech": {"kind": "rider",
                          "trigger": "once per turn, on a weapon "
                                     "attack",
                          "effect": "one more attack with the same "
                                    "weapon against a different "
                                    "creature within 5 ft of the "
                                    "original target and in range"}},
            ],
        },
    },
    # Thief (rogue 3).  Both features are field-flavor for a level-3
    # combat game; Fast Hands' combat half (bonus-action Use an Object)
    # only matters once usable items exist -- revisit at merge if the
    # item system wants it.  5.2.1 keeps both at 3.
    "thief": {
        "class": "rogue", "level": 3,
        "features": {
            1: [],
            2: [],
            3: [
                {"name": "Fast Hands", "srd": True,
                 "mech": {"kind": "flavor",
                          "note": "Cunning Action bonus action may also "
                                  "Sleight of Hand, use thieves' tools, "
                                  "or take Use an Object"}},
                {"name": "Second-Story Work", "srd": True,
                 "mech": {"kind": "flavor",
                          "note": "climb at full speed; running jump "
                                  "distance +DEX mod ft"}},
            ],
        },
    },
    # Draconic Bloodline (sorcerer 1).  5.2.1 ("Draconic Sorcery") moves
    # the origin to 3, replaces Dragon Ancestor with an always-prepared
    # Draconic Spells list, and revises Draconic Resilience.
    "draconic bloodline": {
        "class": "sorcerer", "level": 1,
        "features": {
            1: [
                {"name": "Dragon Ancestor", "srd": True,
                 "mech": {"kind": "flavor",
                          "note": "choose a dragon type; its damage "
                                  "type keys features past level 3 "
                                  "(Elemental Affinity at 6); Draconic "
                                  "language and social perks"}},
                {"name": "Draconic Resilience", "srd": True,
                 "mech": {"kind": "passive_bonus",
                          "what": "max HP",
                          "amount": "+1 per sorcerer level",
                          "also": "unarmored AC = 13 + DEX mod"}},
            ],
            2: [],
            3: [],
        },
    },
    # The Fiend (warlock 1).  5.2.1 moves the patron to 3, makes the
    # bonus list always-prepared ("Fiend Spells"), and broadens Dark
    # One's Blessing to trigger on kills near you as well.
    "fiend": {
        "class": "warlock", "level": 1,
        "features": {
            1: [
                # Spells by WARLOCK level (2nd-level spells arrive with
                # the 2nd-level pact slots at 3).
                {"name": "Expanded Spell List", "srd": True,
                 "mech": {"kind": "spell_list_extend",
                          "spells": {1: ["burning hands", "command"],
                                     3: ["blindness/deafness",
                                         "scorching ray"]}}},
                {"name": "Dark One's Blessing", "srd": True,
                 "mech": {"kind": "rider",
                          "trigger": "you reduce a hostile creature to "
                                     "0 HP",
                          "effect": "gain temp HP = CHA mod + warlock "
                                    "level (min 1)"}},
            ],
            2: [],
            3: [],
        },
    },
    # School of Evocation (wizard 2).  5.2.1 ("Evoker") moves the school
    # to 3, keeps Evocation Savant, adds Potent Cantrip at 3, and defers
    # Sculpt Spells to wizard 6.
    "evocation": {
        "class": "wizard", "level": 2,
        "features": {
            1: [],
            2: [
                {"name": "Evocation Savant", "srd": True,
                 "mech": {"kind": "flavor",
                          "note": "halved gold and time to copy "
                                  "evocation spells into the "
                                  "spellbook"}},
                {"name": "Sculpt Spells", "srd": True,
                 "mech": {"kind": "passive_bonus",
                          "what": "your evocation spells spare chosen "
                                  "creatures: auto-succeed on the save "
                                  "and take no damage where a success "
                                  "would halve it",
                          "amount": "1 + spell level creatures"}},
            ],
            3: [],
        },
    },
}
