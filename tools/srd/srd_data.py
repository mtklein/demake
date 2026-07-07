# srd_data.py -- machine-readable D&D 5e SRD facts for the demake battle engine.
#
# Plain Python literals only: no imports, no dataclasses.  This file feeds a C
# code generator; every table is a dict of dicts of str/int/bool/tuple/list.
#
# Provenance (per table; see tools/srd/ATTRIBUTION.md for legal text):
#   WEAPONS    - SRD 5.2.1 weapons table, via the 5e-quint authored Surface
#                records (packages/surface/content/weapon_*.json), cross-checked
#                against SRD 5.1 (dnd5eapi /api/2014/equipment).  The two SRDs
#                agree on every weapon below except the trident (noted inline).
#   CLASSES    - SRD 5.1 (dnd5eapi /api/2014/classes/*/levels).  The 2024-rules
#                SRD 5.2.1 revises class tables heavily (Weapon Mastery, Tactical
#                Mind, subclass levels moved to 3); this game targets the classic
#                5.1 progression, which matches the pinned design spec.
#   SPELLS     - SRD 5.1 (dnd5eapi /api/2014/spells) baseline mechanics,
#                cross-checked against SRD 5.2.1 via 5e-quint Surface records.
#                5.1-vs-5.2.1 differences are noted inline.  Ensnaring Strike is
#                not in SRD 5.1 at all; it is encoded from SRD 5.2.1.
#   MONSTERS   - imp, boar, skeleton, bandit, and goblin from SRD 5.1
#                (dnd5eapi /api/2014/monsters).
#                intellect devourer, cambion, and mind flayer appear in NO SRD
#                (neither 5.1 nor 5.2.1); their game statistics are reconstructed
#                from the 2014 Monster Manual and are marked "srd": False.
#                (The goblin boss is likewise MM-only -- the gates' warchief is
#                original homebrew in overrides.py, per the piscodemon doctrine.)
#   CONDITIONS - SRD 5.1 (dnd5eapi /api/2014/conditions), condition-implication
#                structure cross-checked against the 5e-quint conditions algebra
#                (proofs/conditions-algebra-inductive.qnt).
#
# Dice strings are "NdS" (optionally with explicit "plus" fields alongside).
# Ability modifiers are not tabulated: mod = (score - 10) // 2.

# --------------------------------------------------------------------------
# WEAPONS
#
# name -> dice, dmg_type, properties (verbatim SRD property names), category,
# melee (True for melee weapons, including thrown ones; False for ranged),
# versatile_dice when versatile, range (normal, long) in feet for weapons with
# the ammunition or thrown property.
# --------------------------------------------------------------------------

WEAPONS = {
    "club": {
        "dice": "1d4", "dmg_type": "bludgeoning",
        "properties": ["light"],
        "category": "simple", "melee": True,
    },
    "dagger": {
        "dice": "1d4", "dmg_type": "piercing",
        "properties": ["finesse", "light", "thrown"],
        "range": (20, 60),
        "category": "simple", "melee": True,
    },
    "handaxe": {
        "dice": "1d6", "dmg_type": "slashing",
        "properties": ["light", "thrown"],
        "range": (20, 60),
        "category": "simple", "melee": True,
    },
    "javelin": {
        "dice": "1d6", "dmg_type": "piercing",
        "properties": ["thrown"],
        "range": (30, 120),
        "category": "simple", "melee": True,
    },
    "mace": {
        "dice": "1d6", "dmg_type": "bludgeoning",
        "properties": [],
        "category": "simple", "melee": True,
    },
    "quarterstaff": {
        "dice": "1d6", "dmg_type": "bludgeoning",
        "properties": ["versatile"],
        "versatile_dice": "1d8",
        "category": "simple", "melee": True,
    },
    "spear": {
        "dice": "1d6", "dmg_type": "piercing",
        "properties": ["thrown", "versatile"],
        "versatile_dice": "1d8",
        "range": (20, 60),
        "category": "simple", "melee": True,
    },
    "light crossbow": {
        "dice": "1d8", "dmg_type": "piercing",
        "properties": ["ammunition", "loading", "two-handed"],
        "range": (80, 320),
        "category": "simple", "melee": False,
    },
    "shortbow": {
        "dice": "1d6", "dmg_type": "piercing",
        "properties": ["ammunition", "two-handed"],
        "range": (80, 320),
        "category": "simple", "melee": False,
    },
    "sling": {
        "dice": "1d4", "dmg_type": "bludgeoning",
        "properties": ["ammunition"],
        "range": (30, 120),
        "category": "simple", "melee": False,
    },
    "greatsword": {
        "dice": "2d6", "dmg_type": "slashing",
        "properties": ["heavy", "two-handed"],
        "category": "martial", "melee": True,
    },
    "longsword": {
        "dice": "1d8", "dmg_type": "slashing",
        "properties": ["versatile"],
        "versatile_dice": "1d10",
        "category": "martial", "melee": True,
    },
    "rapier": {
        "dice": "1d8", "dmg_type": "piercing",
        "properties": ["finesse"],
        "category": "martial", "melee": True,
    },
    "scimitar": {
        "dice": "1d6", "dmg_type": "slashing",
        "properties": ["finesse", "light"],
        "category": "martial", "melee": True,
    },
    "shortsword": {
        "dice": "1d6", "dmg_type": "piercing",
        "properties": ["finesse", "light"],
        "category": "martial", "melee": True,
    },
    # Trident: SRD 5.2.1 values (1d8, versatile 1d10).  SRD 5.1 had 1d6,
    # versatile 1d8; per project rule, 5.2.1 wins on numeric disagreements.
    "trident": {
        "dice": "1d8", "dmg_type": "piercing",
        "properties": ["thrown", "versatile"],
        "versatile_dice": "1d10",
        "range": (20, 60),
        "category": "martial", "melee": True,
    },
    "warhammer": {
        "dice": "1d8", "dmg_type": "bludgeoning",
        "properties": ["versatile"],
        "versatile_dice": "1d10",
        "category": "martial", "melee": True,
    },
    "longbow": {
        "dice": "1d8", "dmg_type": "piercing",
        "properties": ["ammunition", "heavy", "two-handed"],
        "range": (150, 600),
        "category": "martial", "melee": False,
    },
}

# --------------------------------------------------------------------------
# CLASSES (levels 1-3, SRD 5.1 progression)
#
# hit_die, saves, prof_bonus per level, feature names per level (SRD names),
# spell_slots {level: {slot_level: count}} for casters (ranger's start at
# class level 2), cantrips_known for classes with cantrips, spells_known for
# known-casters (bard, ranger), sneak_attack dice for rogue.
#
# SRD 5.2.1 differences (not encoded): Fighter 1 adds Weapon Mastery and
# Fighter 2 adds Tactical Mind; Rogue 1 adds Weapon Mastery; every class picks
# its subclass at level 3; Cleric 1 gains Divine Order instead of a level-1
# Divine Domain; Ranger 1 has Spellcasting + Favored Enemy (as a Hunter's Mark
# rider) instead of Favored Enemy/Natural Explorer; Bard 2 Song of Rest is
# removed and Jack of All Trades moves; Wizard 3 gains Cantrip Formulas.
# --------------------------------------------------------------------------

CLASSES = {
    "bard": {
        "hit_die": 8,
        "saves": ["dex", "cha"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Spellcasting", "Bardic Inspiration (d6)"],
            2: ["Jack of All Trades", "Song of Rest (d6)"],
            3: ["Bard College", "Expertise"],
        },
        "spell_slots": {1: {1: 2}, 2: {1: 3}, 3: {1: 4, 2: 2}},
        "cantrips_known": {1: 2, 2: 2, 3: 2},
        "spells_known": {1: 4, 2: 5, 3: 6},
    },
    "cleric": {
        "hit_die": 8,
        "saves": ["wis", "cha"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Spellcasting", "Divine Domain"],
            2: ["Channel Divinity (1/rest)", "Channel Divinity: Turn Undead",
                "Divine Domain Feature"],
            3: [],  # 2nd-level slots only; domain spells expand
        },
        "spell_slots": {1: {1: 2}, 2: {1: 3}, 3: {1: 4, 2: 2}},
        "cantrips_known": {1: 3, 2: 3, 3: 3},
        # prepared caster: prepares WIS mod + cleric level spells daily
    },
    "fighter": {
        "hit_die": 10,
        "saves": ["str", "con"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Fighting Style", "Second Wind"],
            2: ["Action Surge"],
            3: ["Martial Archetype"],
        },
        # Second Wind: bonus action, self-heal 1d10 + fighter level, 1/rest.
        # Action Surge: one additional action on your turn, 1/rest.
    },
    "ranger": {
        "hit_die": 10,
        "saves": ["str", "dex"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Favored Enemy", "Natural Explorer"],
            2: ["Fighting Style", "Spellcasting"],
            3: ["Ranger Archetype", "Primeval Awareness"],
        },
        # Rangers get NO slots (and no Spellcasting) at level 1.
        "spell_slots": {1: {}, 2: {1: 2}, 3: {1: 3}},
        "spells_known": {1: 0, 2: 2, 3: 3},
        # no cantrips in SRD 5.1
    },
    "rogue": {
        "hit_die": 8,
        "saves": ["dex", "int"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Expertise", "Sneak Attack", "Thieves' Cant"],
            2: ["Cunning Action"],
            3: ["Roguish Archetype"],
        },
        # Once per turn, when hitting with a finesse or ranged weapon with
        # advantage (or with an enemy of the target within 5 ft and no
        # disadvantage), add sneak attack dice.
        "sneak_attack": {1: "1d6", 2: "1d6", 3: "2d6"},
    },
    "wizard": {
        "hit_die": 6,
        "saves": ["int", "wis"],
        "prof_bonus": {1: 2, 2: 2, 3: 2},
        "features": {
            1: ["Spellcasting", "Arcane Recovery"],
            2: ["Arcane Tradition"],
            3: [],  # 2nd-level slots only in SRD 5.1
        },
        "spell_slots": {1: {1: 2}, 2: {1: 3}, 3: {1: 4, 2: 2}},
        "cantrips_known": {1: 3, 2: 3, 3: 3},
        # spellbook caster: 6 spells at level 1, +2 per level gained
    },
}

# --------------------------------------------------------------------------
# SPELLS
#
# level 0 = cantrip.  cast is "action" or "bonus".  dice is either a plain
# "NdS" roll or a structured {"count", "dice", "plus"} for per-missile spells.
# plus_mod True means the spellcasting ability modifier is added to the roll.
# save is (ability, "half"|"negates") or None; "negates" means a successful
# save prevents the spell's damage/effect entirely.  attack is "ranged_spell"
# or None.  targets: "one", "all" (area / may spread), "party" (allies).
#
# Baseline is SRD 5.1; 5.2.1 changes are noted where they exist.
# --------------------------------------------------------------------------

SPELLS = {
    # SRD 5.2.1 changed vicious mockery to 1d6 psychic; 5.1 baseline is 1d4.
    "vicious mockery": {
        "level": 0, "school": "enchantment", "cast": "action",
        "concentration": False,
        "dice": "1d4", "dmg_type": "psychic",
        "save": ("wis", "negates"), "attack": None,
        "effect": "WIS save or take 1d4 psychic and have disadvantage on its "
                  "next attack roll before the end of its next turn",
        "targets": "one", "range": 60, "duration": "instantaneous",
    },
    "fire bolt": {
        "level": 0, "school": "evocation", "cast": "action",
        "concentration": False,
        "dice": "1d10", "dmg_type": "fire",
        "save": None, "attack": "ranged_spell",
        "effect": "ranged spell attack for 1d10 fire; ignites unattended "
                  "flammable objects",
        "targets": "one", "range": 120, "duration": "instantaneous",
    },
    # SRD 5.2.1 school is abjuration and heal is 2d4 + mod; 5.1 baseline kept.
    "healing word": {
        "level": 1, "school": "evocation", "cast": "bonus",
        "concentration": False,
        "dice": "1d4", "plus_mod": True, "dmg_type": None,
        "save": None, "attack": None,
        "effect": "one creature in range regains 1d4 + spellcasting mod HP; "
                  "no effect on undead or constructs; +1d4 per slot above 1",
        "targets": "one", "range": 60, "duration": "instantaneous",
    },
    # SRD 5.2.1 school is abjuration and heal is 2d8 + mod; 5.1 baseline kept.
    "cure wounds": {
        "level": 1, "school": "evocation", "cast": "action",
        "concentration": False,
        "dice": "1d8", "plus_mod": True, "dmg_type": None,
        "save": None, "attack": None,
        "effect": "touched creature regains 1d8 + spellcasting mod HP; no "
                  "effect on undead or constructs; +1d8 per slot above 1",
        "targets": "one", "range": 0, "duration": "instantaneous",
    },
    "magic missile": {
        "level": 1, "school": "evocation", "cast": "action",
        "concentration": False,
        "dice": {"count": 3, "dice": "1d4", "plus": 1}, "dmg_type": "force",
        "save": None, "attack": None,
        "effect": "3 darts, each auto-hits for 1d4+1 force; darts strike "
                  "simultaneously and may split among visible targets; "
                  "+1 dart per slot above 1",
        "targets": "all", "range": 120, "duration": "instantaneous",
    },
    "burning hands": {
        "level": 1, "school": "evocation", "cast": "action",
        "concentration": False,
        "dice": "3d6", "dmg_type": "fire",
        "save": ("dex", "half"), "attack": None,
        "effect": "15-ft cone; DEX save for half of 3d6 fire; ignites "
                  "unattended flammables; +1d6 per slot above 1",
        "targets": "all", "range": 0, "duration": "instantaneous",
    },
    # SRD 5.2.1 redesigned sleep entirely (5-ft sphere, WIS save gate to
    # incapacitated then unconscious, concentration).  The classic 5.1 HP-pool
    # mechanic below is what this game implements.
    "sleep": {
        "level": 1, "school": "enchantment", "cast": "action",
        "concentration": False,
        "dice": "5d8", "dmg_type": None,
        "save": None, "attack": None,
        "effect": "roll 5d8 for an HP pool; creatures within 20 ft of the "
                  "point fall unconscious in ascending order of current HP, "
                  "no save, subtracting each sleeper's HP from the pool; skip "
                  "a creature the remaining pool cannot cover; unconscious 1 "
                  "minute, ends on damage or if shaken awake (action); undead "
                  "and charm-immune creatures unaffected; +2d8 per slot",
        "targets": "all", "range": 90, "duration": "1 minute",
    },
    "guiding bolt": {
        "level": 1, "school": "evocation", "cast": "action",
        "concentration": False,
        "dice": "4d6", "dmg_type": "radiant",
        "save": None, "attack": "ranged_spell",
        "effect": "ranged spell attack for 4d6 radiant; next attack roll "
                  "against the target before the end of your next turn has "
                  "advantage; +1d6 per slot above 1",
        "targets": "one", "range": 120, "duration": "1 round",
    },
    "bless": {
        "level": 1, "school": "enchantment", "cast": "action",
        "concentration": True,
        "dice": "1d4", "dmg_type": None,
        "save": None, "attack": None,
        "effect": "up to 3 creatures add 1d4 to attack rolls and saving "
                  "throws for the duration; +1 target per slot above 1",
        "targets": "party", "range": 30, "duration": "1 minute",
    },
    # SRD 5.2.1 makes the extra damage Force and un-types it from weapons
    # ("when you hit it with an attack roll"); 5.1 baseline: weapon hits.
    "hunter's mark": {
        "level": 1, "school": "divination", "cast": "bonus",
        "concentration": True,
        "dice": "1d6", "dmg_type": None,
        "save": None, "attack": None,
        "effect": "mark one creature; your weapon attacks that hit it deal "
                  "+1d6 damage (weapon's type); advantage on Perception/"
                  "Survival checks to find it; bonus action to re-mark on "
                  "kill; 3rd-4th slot 8 h, 5th+ 24 h",
        "targets": "one", "range": 90, "duration": "1 hour",
    },
    # Not in SRD 5.1; encoded from SRD 5.2.1 (cast as a bonus action
    # immediately after hitting with a weapon).  The 2014 PHB version was
    # instead cast before the hit and triggered on your next weapon hit.
    "ensnaring strike": {
        "level": 1, "school": "conjuration", "cast": "bonus",
        "concentration": True,
        "dice": "1d6", "dmg_type": "piercing",
        "save": ("str", "negates"), "attack": None,
        "effect": "on a weapon hit, target makes a STR save (advantage if "
                  "Large or bigger) or is restrained by vines and takes 1d6 "
                  "piercing at the start of each of its turns; target or a "
                  "creature in reach may use an action, STR (Athletics) vs "
                  "spell DC, to end it; +1d6 per slot above 1",
        "targets": "one", "range": 0, "duration": "1 minute",
    },
    "shield of faith": {
        "level": 1, "school": "abjuration", "cast": "bonus",
        "concentration": True,
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "effect": "one creature in range gains +2 AC for the duration",
        "targets": "one", "range": 60, "duration": "10 minutes",
    },
    "thunderwave": {
        "level": 1, "school": "evocation", "cast": "action",
        "concentration": False,
        "dice": "2d8", "dmg_type": "thunder",
        "save": ("con", "half"), "attack": None,
        "effect": "15-ft cube from self; CON save or take 2d8 thunder and be "
                  "pushed 10 ft away (half damage, no push, on success); "
                  "audible 300 ft; +1d8 per slot above 1",
        "targets": "all", "range": 0, "duration": "instantaneous",
    },
}

# --------------------------------------------------------------------------
# MONSTERS
#
# imp and boar: SRD 5.1 stat blocks ("srd": True).
# intellect devourer, cambion, mind flayer: present in NO SRD (5.1 or 5.2.1);
# game statistics reconstructed from the 2014 Monster Manual ("srd": False).
# SRD 5.2.1 revises the imp (HP 21 = 6d4+6, drops the nonmagical-b/p/s
# resistance) and boar (HP 13 = 2d8+4, CON 14); 5.1 values kept as the game
# baseline.
#
# attacks: to_hit None means the action forces a save instead of an attack
# roll.  "extra" is a rider applied on hit.  traits are SRD/MM feature names;
# trait_notes gives each a one-line mechanical summary.
# --------------------------------------------------------------------------

MONSTERS = {
    "imp": {
        "srd": True,
        "ac": 13, "hp": 10, "hp_dice": "3d4+3",
        "abilities": {"str": 6, "dex": 17, "con": 13,
                      "int": 11, "wis": 12, "cha": 14},
        "speed": 20, "fly_speed": 40,
        "cr": 1,
        "attacks": [
            {"name": "Sting", "to_hit": 5, "dice": "1d4", "plus": 3,
             "dmg_type": "piercing",
             "extra": {"save": ("con", 11), "dice": "3d6",
                       "dmg_type": "poison", "half_on_save": True}},
        ],
        "traits": ["Shapechanger", "Devil's Sight", "Magic Resistance",
                   "Invisibility"],
        "trait_notes": {
            "Shapechanger": "action: polymorph into rat, raven, or spider "
                            "form (or back); stats identical except speed",
            "Devil's Sight": "magical darkness does not impede its "
                             "darkvision (120 ft)",
            "Magic Resistance": "advantage on saving throws against spells "
                                "and other magical effects",
            "Invisibility": "action: turn invisible until it attacks or "
                            "concentration ends",
        },
        "resistances": ["cold",
                        "bludgeoning/piercing/slashing from nonmagical "
                        "attacks not made with silvered weapons"],
        "immunities": ["fire", "poison"],
        "condition_immunities": ["poisoned"],
        "senses": {"darkvision": 120, "passive_perception": 11},
    },
    "boar": {
        "srd": True,
        "ac": 11, "hp": 11, "hp_dice": "2d8+2",
        "abilities": {"str": 13, "dex": 11, "con": 12,
                      "int": 2, "wis": 9, "cha": 5},
        "speed": 40,
        "cr": 0.25,
        "attacks": [
            {"name": "Tusk", "to_hit": 3, "dice": "1d6", "plus": 1,
             "dmg_type": "slashing"},
        ],
        "traits": ["Charge", "Relentless"],
        "trait_notes": {
            "Charge": "if it moves 20+ ft straight at a target and hits with "
                      "Tusk that turn: +1d6 slashing and DC 11 STR save or "
                      "the target is knocked prone",
            "Relentless": "1/rest: damage of 7 or less that would drop it to "
                          "0 HP drops it to 1 HP instead",
        },
        "resistances": [],
        "immunities": [],
        "condition_immunities": [],
        "senses": {"passive_perception": 9},
    },
    "intellect devourer": {
        "srd": False,  # 2014 Monster Manual; not in SRD 5.1 or 5.2.1
        "ac": 12, "hp": 21, "hp_dice": "6d4+6",
        "abilities": {"str": 6, "dex": 14, "con": 13,
                      "int": 12, "wis": 11, "cha": 10},
        "speed": 40,
        "cr": 2,
        "multiattack": "one Claws attack plus Devour Intellect",
        "attacks": [
            {"name": "Claws", "to_hit": 4, "dice": "2d4", "plus": 2,
             "dmg_type": "slashing"},
            {"name": "Devour Intellect", "to_hit": None,
             "dice": "2d10", "plus": 0, "dmg_type": "psychic",
             "save": ("int", 12), "half_on_save": False,
             "note": "target with a brain within 10 ft; on failed save also "
                     "roll 3d6: if >= target's INT, INT drops to 0 and the "
                     "target is stunned until it regains at least 1 INT"},
        ],
        "traits": ["Detect Sentience", "Devour Intellect", "Body Thief"],
        "trait_notes": {
            "Detect Sentience": "senses any creature with INT 3+ within 300 "
                                "ft unless warded by mind blank",
            "Devour Intellect": "action: DC 12 INT save or 2d10 psychic; on "
                                "fail roll 3d6 vs target INT - if equal or "
                                "higher, INT becomes 0 and target is stunned "
                                "until it regains a point of INT",
            "Body Thief": "action: INT contest vs an incapacitated humanoid "
                          "within 5 ft; on a win it consumes the brain, "
                          "teleports into the skull, and controls the body",
        },
        "resistances": ["bludgeoning/piercing/slashing from nonmagical "
                        "attacks"],
        "immunities": [],
        "condition_immunities": ["blinded"],
        "senses": {"blindsight": 60, "passive_perception": 12},
    },
    "cambion": {
        "srd": False,  # 2014 Monster Manual; not in SRD 5.1 or 5.2.1
        "ac": 19, "hp": 82, "hp_dice": "11d8+33",
        "abilities": {"str": 18, "dex": 18, "con": 16,
                      "int": 14, "wis": 12, "cha": 16},
        "speed": 30, "fly_speed": 60,
        "cr": 5,
        "saves": {"str": 7, "con": 6, "int": 5, "cha": 6},
        "multiattack": "two melee attacks or two Fire Rays",
        "attacks": [
            {"name": "Spear", "to_hit": 7, "dice": "1d6", "plus": 4,
             "dmg_type": "piercing",
             "extra": {"dice": "1d6", "dmg_type": "fire"},
             "note": "1d8+4 if wielded two-handed in melee; thrown 20/60"},
            {"name": "Fire Ray", "to_hit": 7, "dice": "3d6", "plus": 0,
             "dmg_type": "fire",
             "note": "ranged spell attack, range 120 ft"},
        ],
        "traits": ["Fiendish Blessing", "Innate Spellcasting",
                   "Fiendish Charm"],
        "trait_notes": {
            "Fiendish Blessing": "AC includes its CHA modifier",
            "Innate Spellcasting": "CHA, DC 14: 3/day each alter self, "
                                   "command, detect magic; 1/day plane shift "
                                   "(self only)",
            "Fiendish Charm": "action: one humanoid within 30 ft, DC 14 WIS "
                              "save or charmed and obedient for 1 day; "
                              "repeats save on harm; success = immune 24 h",
        },
        "resistances": ["cold", "fire", "lightning", "poison",
                        "bludgeoning/piercing/slashing from nonmagical "
                        "attacks"],
        "immunities": [],
        "condition_immunities": [],
        "senses": {"darkvision": 60, "passive_perception": 14},
    },
    "mind flayer": {
        "srd": False,  # 2014 Monster Manual; not in SRD 5.1 or 5.2.1
        "ac": 15, "hp": 71, "hp_dice": "13d8+13",
        "abilities": {"str": 11, "dex": 12, "con": 12,
                      "int": 19, "wis": 17, "cha": 17},
        "speed": 30,
        "cr": 7,
        "saves": {"int": 7, "wis": 6, "cha": 6},
        "attacks": [
            {"name": "Tentacles", "to_hit": 7, "dice": "2d10", "plus": 4,
             "dmg_type": "psychic",
             "extra": {"save": ("int", 15),
                       "condition": "stunned until grapple ends",
                       "half_on_save": False},
             "note": "Medium or smaller target is grappled (escape DC 15) "
                     "and must save or be stunned while grappled"},
            {"name": "Extract Brain", "to_hit": 7, "dice": "10d10", "plus": 0,
             "dmg_type": "piercing",
             "note": "one incapacitated humanoid grappled by the mind "
                     "flayer; kills the target if this drops it to 0 HP"},
            {"name": "Mind Blast", "to_hit": None, "dice": "4d8", "plus": 4,
             "dmg_type": "psychic",
             "save": ("int", 15), "half_on_save": False,
             "recharge": "5-6",
             "note": "60-ft cone; fail = damage and stunned 1 minute, save "
                     "repeats at end of each of the target's turns"},
        ],
        "traits": ["Magic Resistance", "Innate Spellcasting (Psionics)"],
        "trait_notes": {
            "Magic Resistance": "advantage on saving throws against spells "
                                "and other magical effects",
            "Innate Spellcasting (Psionics)": "INT, DC 15, no components: "
                                              "at will detect thoughts, "
                                              "levitate; 1/day each dominate "
                                              "monster, plane shift (self "
                                              "only)",
        },
        "resistances": [],
        "immunities": [],
        "condition_immunities": [],
        "senses": {"darkvision": 120, "passive_perception": 16},
    },
    "skeleton": {
        "srd": True,   # SRD 5.1 stat block, verbatim
        "ac": 13, "hp": 13, "hp_dice": "2d8+4",
        "abilities": {"str": 10, "dex": 14, "con": 15,
                      "int": 6, "wis": 8, "cha": 5},
        "speed": 30,
        "cr": 0.25,
        "attacks": [
            {"name": "Shortsword", "to_hit": 4, "dice": "1d6", "plus": 2,
             "dmg_type": "piercing"},
            {"name": "Shortbow", "ranged": True, "to_hit": 4,
             "dice": "1d6", "plus": 2, "dmg_type": "piercing",
             "note": "range 80/320"},
        ],
        "traits": [],
        "trait_notes": {},
        "vulnerabilities": ["bludgeoning"],
        "resistances": [],
        "immunities": ["poison"],
        "condition_immunities": ["exhaustion", "poisoned"],
        "senses": {"darkvision": 60, "passive_perception": 9},
    },
    "bandit": {
        "srd": True,   # SRD 5.1 stat block, verbatim
        "ac": 12, "hp": 11, "hp_dice": "2d6+4",
        "abilities": {"str": 11, "dex": 12, "con": 12,
                      "int": 10, "wis": 10, "cha": 10},
        "speed": 30,
        "cr": 0.125,
        "attacks": [
            {"name": "Scimitar", "to_hit": 3, "dice": "1d6", "plus": 1,
             "dmg_type": "slashing"},
            {"name": "Lt.Xbow", "ranged": True, "to_hit": 3,
             "dice": "1d8", "plus": 1, "dmg_type": "piercing",
             "note": "light crossbow, range 80/320"},
        ],
        "traits": [],
        "trait_notes": {},
        "resistances": [],
        "immunities": [],
        "condition_immunities": [],
        "senses": {"passive_perception": 10},
    },
    "goblin": {
        "srd": True,   # SRD 5.1 stat block, verbatim
        "ac": 15, "hp": 7, "hp_dice": "2d6",
        "abilities": {"str": 8, "dex": 14, "con": 10,
                      "int": 10, "wis": 8, "cha": 8},
        "speed": 30,
        "cr": 0.25,
        "attacks": [
            {"name": "Scimitar", "to_hit": 4, "dice": "1d6", "plus": 2,
             "dmg_type": "slashing"},
            {"name": "Shortbow", "ranged": True, "to_hit": 4,
             "dice": "1d6", "plus": 2, "dmg_type": "piercing",
             "note": "range 80/320"},
        ],
        "traits": ["Nimble Escape"],
        "trait_notes": {
            "Nimble Escape": "can take the Disengage or Hide action as a "
                             "bonus action on each of its turns",
        },
        "resistances": [],
        "immunities": [],
        "condition_immunities": [],
        "senses": {"darkvision": 60, "passive_perception": 9},
    },
}

# --------------------------------------------------------------------------
# CONDITIONS (the 14 SRD conditions; exhaustion is a separate 6-level track,
# not tabulated here).  "implies" lists conditions that are mechanically in
# force whenever this one is: paralyzed/petrified/stunned/unconscious imply
# incapacitated, and unconscious also drops the creature prone.  Structure
# cross-checked against the 5e-quint conditions algebra.
# --------------------------------------------------------------------------

CONDITIONS = {
    "blinded": {
        "effects": ["can't see; auto-fails checks requiring sight",
                    "attack rolls against it have advantage",
                    "its attack rolls have disadvantage"],
        "implies": [],
    },
    "charmed": {
        "effects": ["can't attack the charmer or target it with harmful "
                    "abilities or magical effects",
                    "charmer has advantage on social ability checks "
                    "against it"],
        "implies": [],
    },
    "deafened": {
        "effects": ["can't hear; auto-fails checks requiring hearing"],
        "implies": [],
    },
    "frightened": {
        "effects": ["disadvantage on ability checks and attack rolls while "
                    "the fear source is in line of sight",
                    "can't willingly move closer to the source"],
        "implies": [],
    },
    "grappled": {
        "effects": ["speed is 0 and can't benefit from speed bonuses",
                    "ends if the grappler is incapacitated",
                    "ends if an effect removes it from the grappler's reach"],
        "implies": [],
    },
    "incapacitated": {
        "effects": ["can't take actions or reactions"],
        "implies": [],
    },
    "invisible": {
        "effects": ["can't be seen without magic or special senses; heavily "
                    "obscured for hiding",
                    "attack rolls against it have disadvantage",
                    "its attack rolls have advantage"],
        "implies": [],
    },
    "paralyzed": {
        "effects": ["can't move or speak",
                    "auto-fails STR and DEX saving throws",
                    "attack rolls against it have advantage",
                    "any hit from within 5 ft is a critical hit"],
        "implies": ["incapacitated"],
    },
    "petrified": {
        "effects": ["transformed to solid substance; weight x10; not aging",
                    "can't move or speak; unaware of surroundings",
                    "auto-fails STR and DEX saving throws",
                    "attack rolls against it have advantage",
                    "resistance to all damage",
                    "immune to poison and disease (existing ones suspended)"],
        "implies": ["incapacitated"],
    },
    "poisoned": {
        "effects": ["disadvantage on attack rolls and ability checks"],
        "implies": [],
    },
    "prone": {
        "effects": ["may only crawl until it stands (costs half speed)",
                    "its attack rolls have disadvantage",
                    "attack rolls against it have advantage within 5 ft, "
                    "disadvantage beyond 5 ft"],
        "implies": [],
    },
    "restrained": {
        "effects": ["speed is 0 and can't benefit from speed bonuses",
                    "attack rolls against it have advantage",
                    "its attack rolls have disadvantage",
                    "disadvantage on DEX saving throws"],
        "implies": [],
    },
    "stunned": {
        "effects": ["can't move; can speak only falteringly",
                    "auto-fails STR and DEX saving throws",
                    "attack rolls against it have advantage"],
        "implies": ["incapacitated"],
    },
    "unconscious": {
        "effects": ["can't move or speak; unaware of surroundings",
                    "drops whatever it is holding and falls prone",
                    "auto-fails STR and DEX saving throws",
                    "attack rolls against it have advantage",
                    "any hit from within 5 ft is a critical hit"],
        "implies": ["incapacitated", "prone"],
    },
}

# --------------------------------------------------------------------------
# SUBCLASSES
#
# SRD 5.1 subclasses, levels 1-3, mech-distilled. Provenance: dnd5eapi,
# see git history of tools/srd/stage_classes.py for the extraction report.
# --------------------------------------------------------------------------

SUBCLASSES = {'berserker': {'class': 'barbarian',
               'level': 3,
               'features': {1: [],
                            2: [],
                            3: [{'name': 'Frenzy',
                                 'srd': True,
                                 'mech': {'kind': 'rider',
                                          'trigger': 'while frenzy-raging: '
                                                     'bonus action on each '
                                                     'turn after the one you '
                                                     'entered the rage',
                                          'effect': 'one melee weapon attack',
                                          'cost': 'one level of exhaustion '
                                                  'when the rage ends'}}]}},
 'lore': {'class': 'bard',
          'level': 3,
          'features': {1: [],
                       2: [],
                       3: [{'name': 'Bonus Proficiencies',
                            'srd': True,
                            'mech': {'kind': 'extra_prof',
                                     'what': 'three skills of your choice'}},
                           {'name': 'Cutting Words',
                            'srd': True,
                            'mech': {'kind': 'reaction',
                                     'trigger': 'creature you can see within '
                                                '60 ft makes an attack roll, '
                                                'ability check, or damage '
                                                'roll',
                                     'effect': 'expend one Bardic '
                                               'Inspiration use; roll the '
                                               'die and subtract it from the '
                                               "creature's roll",
                                     'dice': '1d6',
                                     'limits': 'declared after the roll, '
                                               'before the outcome; wasted '
                                               "if the target can't hear you "
                                               'or is charm-immune'}}]}},
 'life': {'class': 'cleric',
          'level': 1,
          'features': {1: [{'name': 'Domain Spells',
                            'srd': True,
                            'mech': {'kind': 'always_prepared',
                                     'spells': {1: ['bless', 'cure wounds'],
                                                3: ['lesser restoration',
                                                    'spiritual weapon']}}},
                           {'name': 'Bonus Proficiency',
                            'srd': True,
                            'mech': {'kind': 'extra_prof',
                                     'what': 'heavy armor'}},
                           {'name': 'Disciple of Life',
                            'srd': True,
                            'mech': {'kind': 'heal_bonus',
                                     'amount': '2 + spell level',
                                     'trigger': 'a level 1+ spell you cast '
                                                'restores HP to a '
                                                'creature'}}],
                       2: [{'name': 'Channel Divinity: Preserve Life',
                            'srd': True,
                            'mech': {'kind': 'resource',
                                     'pool': 'channel_divinity',
                                     'refresh': 'short',
                                     'amount': '5 x cleric level',
                                     'effect': 'action: restore that many HP '
                                               'split among creatures within '
                                               '30 ft, none above half its '
                                               'HP max; no undead or '
                                               'constructs'}}],
                       3: []}},
 'land': {'class': 'druid',
          'level': 2,
          'features': {1: [],
                       2: [{'name': 'Bonus Cantrip',
                            'srd': True,
                            'mech': {'kind': 'passive_bonus',
                                     'what': 'cantrips_known',
                                     'amount': 1}},
                           {'name': 'Natural Recovery',
                            'srd': True,
                            'mech': {'kind': 'resource',
                                     'pool': 'spell_slots',
                                     'refresh': 'long',
                                     'effect': 'once per day during a short '
                                               'rest, recover expended slots '
                                               'totaling <= half druid level '
                                               'rounded up (= one 1st-level '
                                               'slot at druid 2-3), none 6th '
                                               'level or higher'}}],
                       3: [{'name': 'Circle Spells (grassland)',
                            'srd': True,
                            'mech': {'kind': 'always_prepared',
                                     'spells': {3: ['invisibility',
                                                    'pass without '
                                                    'trace']}}}]}},
 'champion': {'class': 'fighter',
              'level': 3,
              'features': {1: [],
                           2: [],
                           3: [{'name': 'Improved Critical',
                                'srd': True,
                                'mech': {'kind': 'crit_range',
                                         'min_roll': 19}}]}},
 'open hand': {'class': 'monk',
               'level': 3,
               'features': {1: [],
                            2: [],
                            3: [{'name': 'Open Hand Technique',
                                 'srd': True,
                                 'mech': {'kind': 'rider',
                                          'trigger': 'each Flurry of Blows '
                                                     'hit; choose one effect',
                                          'dc': 'ki (8 + prof + WIS mod)',
                                          'options': [{'save': 'dex',
                                                       'effect': 'knocked '
                                                                 'prone'},
                                                      {'save': 'str',
                                                       'effect': 'pushed up '
                                                                 'to 15 ft '
                                                                 'away'},
                                                      {'save': None,
                                                       'effect': 'no '
                                                                 'reactions '
                                                                 'until the '
                                                                 'end of '
                                                                 'your next '
                                                                 'turn'}]}}]}},
 'devotion': {'class': 'paladin',
              'level': 3,
              'features': {1: [],
                           2: [],
                           3: [{'name': 'Oath Spells',
                                'srd': True,
                                'mech': {'kind': 'always_prepared',
                                         'spells': {3: ['protection from '
                                                        'evil and good',
                                                        'sanctuary']}}},
                               {'name': 'Channel Divinity: Sacred Weapon',
                                'srd': True,
                                'mech': {'kind': 'resource',
                                         'pool': 'channel_divinity',
                                         'refresh': 'short',
                                         'effect': 'action: one held weapon '
                                                   'gains +CHA mod (min +1) '
                                                   'to attack rolls for 1 '
                                                   'minute, glows (bright '
                                                   'light 20 ft), and counts '
                                                   'as magical'}},
                               {'name': 'Channel Divinity: Turn the Unholy',
                                'srd': True,
                                'mech': {'kind': 'resource',
                                         'pool': 'channel_divinity',
                                         'refresh': 'short',
                                         'save': 'wis',
                                         'effect': 'action: each fiend or '
                                                   'undead within 30 ft that '
                                                   'can see or hear you '
                                                   'saves or is turned '
                                                   '(flees, no reactions) '
                                                   'for 1 minute or until it '
                                                   'takes damage'}}]}},
 'hunter': {'class': 'ranger',
            'level': 3,
            'features': {1: [],
                         2: [],
                         3: [{'name': "Hunter's Prey: Colossus Slayer",
                              'srd': True,
                              'mech': {'kind': 'rider',
                                       'dice': '1d8',
                                       'type': 'weapon',
                                       'trigger': 'once per turn, weapon hit '
                                                  'against a creature below '
                                                  'its HP max'}},
                             {'name': "Hunter's Prey: Giant Killer",
                              'srd': True,
                              'mech': {'kind': 'reaction',
                                       'trigger': 'a Large or larger '
                                                  'creature within 5 ft hits '
                                                  'or misses you with an '
                                                  'attack',
                                       'effect': 'one weapon attack against '
                                                 'it'}},
                             {'name': "Hunter's Prey: Horde Breaker",
                              'srd': True,
                              'mech': {'kind': 'rider',
                                       'trigger': 'once per turn, on a '
                                                  'weapon attack',
                                       'effect': 'one more attack with the '
                                                 'same weapon against a '
                                                 'different creature within '
                                                 '5 ft of the original '
                                                 'target and in range'}}]}},
 'thief': {'class': 'rogue',
           'level': 3,
           'features': {1: [],
                        2: [],
                        3: [{'name': 'Fast Hands',
                             'srd': True,
                             'mech': {'kind': 'flavor',
                                      'note': 'Cunning Action bonus action '
                                              'may also Sleight of Hand, use '
                                              "thieves' tools, or take Use "
                                              'an Object'}},
                            {'name': 'Second-Story Work',
                             'srd': True,
                             'mech': {'kind': 'flavor',
                                      'note': 'climb at full speed; running '
                                              'jump distance +DEX mod '
                                              'ft'}}]}},
 'draconic bloodline': {'class': 'sorcerer',
                        'level': 1,
                        'features': {1: [{'name': 'Dragon Ancestor',
                                          'srd': True,
                                          'mech': {'kind': 'flavor',
                                                   'note': 'choose a dragon '
                                                           'type; its damage '
                                                           'type keys '
                                                           'features past '
                                                           'level 3 '
                                                           '(Elemental '
                                                           'Affinity at 6); '
                                                           'Draconic '
                                                           'language and '
                                                           'social perks'}},
                                         {'name': 'Draconic Resilience',
                                          'srd': True,
                                          'mech': {'kind': 'passive_bonus',
                                                   'what': 'max HP',
                                                   'amount': '+1 per '
                                                             'sorcerer level',
                                                   'also': 'unarmored AC = '
                                                           '13 + DEX mod'}}],
                                     2: [],
                                     3: []}},
 'fiend': {'class': 'warlock',
           'level': 1,
           'features': {1: [{'name': 'Expanded Spell List',
                             'srd': True,
                             'mech': {'kind': 'spell_list_extend',
                                      'spells': {1: ['burning hands',
                                                     'command'],
                                                 3: ['blindness/deafness',
                                                     'scorching ray']}}},
                            {'name': "Dark One's Blessing",
                             'srd': True,
                             'mech': {'kind': 'rider',
                                      'trigger': 'you reduce a hostile '
                                                 'creature to 0 HP',
                                      'effect': 'gain temp HP = CHA mod + '
                                                'warlock level (min 1)'}}],
                        2: [],
                        3: []}},
 'evocation': {'class': 'wizard',
               'level': 2,
               'features': {1: [],
                            2: [{'name': 'Evocation Savant',
                                 'srd': True,
                                 'mech': {'kind': 'flavor',
                                          'note': 'halved gold and time to '
                                                  'copy evocation spells '
                                                  'into the spellbook'}},
                                {'name': 'Sculpt Spells',
                                 'srd': True,
                                 'mech': {'kind': 'passive_bonus',
                                          'what': 'your evocation spells '
                                                  'spare chosen creatures: '
                                                  'auto-succeed on the save '
                                                  'and take no damage where '
                                                  'a success would halve it',
                                          'amount': '1 + spell level '
                                                    'creatures'}}],
                            3: []}}}

# --------------------------------------------------------------------------
# RACES
#
# SRD 5.1 races + subraces, mech-distilled (stage_races.py history).
# --------------------------------------------------------------------------

RACES = {'dwarf': {'srd': True,
           'asi': {'con': 2},
           'speed': 25,
           'traits': [{'name': 'Darkvision',
                       'srd': True,
                       'mech': {'kind': 'flavor',
                                'text': 'see 60 ft in dim light as if '
                                        'bright, and in darkness as in dim '
                                        '(gray only)'}},
                      {'name': 'Dwarven Resilience',
                       'srd': True,
                       'mech': {'kind': 'advantage',
                                'on': 'saving throws against poison',
                                'also': {'kind': 'resist',
                                         'dmg_type': 'poison'}}},
                      {'name': 'Stonecunning',
                       'srd': True,
                       'mech': {'kind': 'flavor',
                                'text': 'double proficiency on INT (History) '
                                        'checks about the origin of '
                                        'stonework'}},
                      {'name': 'Dwarven Combat Training',
                       'srd': True,
                       'mech': {'kind': 'passive_bonus',
                                'what': 'proficiencies',
                                'value': ['battleaxe',
                                          'handaxe',
                                          'light hammer',
                                          'warhammer'],
                                'note': 'handaxe and warhammer are in '
                                        'srd_data.WEAPONS; battleaxe and '
                                        'light hammer are not staged'}},
                      {'name': 'Tool Proficiency',
                       'srd': True,
                       'mech': {'kind': 'flavor',
                                'text': "artisan's tools: smith's, brewer's, "
                                        "or mason's (your pick)"}},
                      {'name': 'Languages',
                       'srd': True,
                       'mech': {'kind': 'flavor',
                                'text': 'Common and Dwarvish'}}],
           'subraces': {'hill dwarf': {'srd': True,
                                       'asi': {'wis': 1},
                                       'traits': [{'name': 'Dwarven '
                                                           'Toughness',
                                                   'srd': True,
                                                   'mech': {'kind': 'passive_bonus',
                                                            'what': 'hp',
                                                            'amount': '+1 '
                                                                      'per '
                                                                      'character '
                                                                      'level '
                                                                      '(including '
                                                                      '1st)'}}]}}},
 'elf': {'srd': True,
         'asi': {'dex': 2},
         'speed': 30,
         'traits': [{'name': 'Darkvision',
                     'srd': True,
                     'mech': {'kind': 'flavor',
                              'text': 'see 60 ft in dim light as if bright, '
                                      'and in darkness as in dim (gray '
                                      'only)'}},
                    {'name': 'Fey Ancestry',
                     'srd': True,
                     'mech': {'kind': 'advantage',
                              'on': 'saving throws against the charmed '
                                    'condition',
                              'also': {'kind': 'passive_bonus',
                                       'what': 'sleep immunity',
                                       'value': 'magic cannot put you to '
                                                "sleep (the sleep spell's HP "
                                                'pool skips you)'}}},
                    {'name': 'Trance',
                     'srd': True,
                     'mech': {'kind': 'flavor',
                              'text': 'no sleep; 4 hours of semiconscious '
                                      "meditation serve as a night's rest"}},
                    {'name': 'Keen Senses',
                     'srd': True,
                     'mech': {'kind': 'passive_bonus',
                              'what': 'skills',
                              'value': ['perception']}},
                    {'name': 'Languages',
                     'srd': True,
                     'mech': {'kind': 'flavor',
                              'text': 'Common and Elvish'}}],
         'subraces': {'high elf': {'srd': True,
                                   'asi': {'int': 1},
                                   'traits': [{'name': 'Elf Weapon Training',
                                               'srd': True,
                                               'mech': {'kind': 'passive_bonus',
                                                        'what': 'proficiencies',
                                                        'value': ['longsword',
                                                                  'shortsword',
                                                                  'shortbow',
                                                                  'longbow'],
                                                        'note': 'all four '
                                                                'are in '
                                                                'srd_data.WEAPONS '
                                                                'and the '
                                                                'engine '
                                                                'roster'}},
                                              {'name': 'High Elf Cantrip',
                                               'srd': True,
                                               'mech': {'kind': 'choice',
                                                        'when': 'creation',
                                                        'options': [{'label': 'acid '
                                                                              'splash',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['acid '
                                                                                'splash']},
                                                                    {'label': 'chill '
                                                                              'touch',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['chill '
                                                                                'touch']},
                                                                    {'label': 'dancing '
                                                                              'lights',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['dancing '
                                                                                'lights']},
                                                                    {'label': 'fire '
                                                                              'bolt',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['fire '
                                                                                'bolt']},
                                                                    {'label': 'light',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['light']},
                                                                    {'label': 'mage '
                                                                              'hand',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['mage '
                                                                                'hand']},
                                                                    {'label': 'mending',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['mending']},
                                                                    {'label': 'message',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['message']},
                                                                    {'label': 'minor '
                                                                              'illusion',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['minor '
                                                                                'illusion']},
                                                                    {'label': 'poison '
                                                                              'spray',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['poison '
                                                                                'spray']},
                                                                    {'label': 'prestidigitation',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['prestidigitation']},
                                                                    {'label': 'ray '
                                                                              'of '
                                                                              'frost',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['ray '
                                                                                'of '
                                                                                'frost']},
                                                                    {'label': 'shocking '
                                                                              'grasp',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['shocking '
                                                                                'grasp']},
                                                                    {'label': 'true '
                                                                              'strike',
                                                                     'kind': 'always_prepared',
                                                                     'spells': ['true '
                                                                                'strike']}]}},
                                              {'name': 'Extra Language',
                                               'srd': True,
                                               'mech': {'kind': 'flavor',
                                                        'text': 'one extra '
                                                                'language of '
                                                                'your '
                                                                'choice'}}]}}},
 'halfling': {'srd': True,
              'asi': {'dex': 2},
              'speed': 25,
              'traits': [{'name': 'Brave',
                          'srd': True,
                          'mech': {'kind': 'advantage',
                                   'on': 'saving throws against the '
                                         'frightened condition'}},
                         {'name': 'Halfling Nimbleness',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'move through the space of any '
                                           'creature one size larger than '
                                           'you (no move economy at our '
                                           'scope)'}},
                         {'name': 'Lucky',
                          'srd': True,
                          'mech': {'kind': 'reroll',
                                   'on': 'a natural 1 on an attack roll, '
                                         'ability check, or saving throw',
                                   'keep': 'the new roll',
                                   'note': 'dice-core flag on the d20 path '
                                           '(character2.md); reroll once, '
                                           'the new result stands even if it '
                                           'is another 1'}},
                         {'name': 'Languages',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'Common and Halfling'}}],
              'subraces': {'lightfoot halfling': {'srd': True,
                                                  'asi': {'cha': 1},
                                                  'traits': [{'name': 'Naturally '
                                                                      'Stealthy',
                                                              'srd': True,
                                                              'mech': {'kind': 'flavor',
                                                                       'text': 'you '
                                                                               'can '
                                                                               'hide '
                                                                               'behind '
                                                                               'creatures '
                                                                               'at '
                                                                               'least '
                                                                               'one '
                                                                               'size '
                                                                               'larger '
                                                                               'than '
                                                                               'you '
                                                                               '(positional; '
                                                                               'not '
                                                                               'in '
                                                                               'the '
                                                                               'design '
                                                                               "doc's "
                                                                               'mech '
                                                                               'list)'}}]}}},
 'human': {'srd': True,
           'asi': {'str': 1,
                   'dex': 1,
                   'con': 1,
                   'int': 1,
                   'wis': 1,
                   'cha': 1},
           'speed': 30,
           'traits': [{'name': 'Languages',
                       'srd': True,
                       'mech': {'kind': 'flavor',
                                'text': 'Common and one extra language of '
                                        'your choice'}}],
           'subraces': {}},
 'dragonborn': {'srd': True,
                'asi': {'str': 2, 'cha': 1},
                'speed': 30,
                'traits': [{'name': 'Draconic Ancestry',
                            'srd': True,
                            'mech': {'kind': 'choice',
                                     'when': 'creation',
                                     'options': [{'label': 'black',
                                                  'dmg_type': 'acid',
                                                  'shape': '5x30 ft line',
                                                  'save_ab': 'dex'},
                                                 {'label': 'blue',
                                                  'dmg_type': 'lightning',
                                                  'shape': '5x30 ft line',
                                                  'save_ab': 'dex'},
                                                 {'label': 'brass',
                                                  'dmg_type': 'fire',
                                                  'shape': '5x30 ft line',
                                                  'save_ab': 'dex'},
                                                 {'label': 'bronze',
                                                  'dmg_type': 'lightning',
                                                  'shape': '5x30 ft line',
                                                  'save_ab': 'dex'},
                                                 {'label': 'copper',
                                                  'dmg_type': 'acid',
                                                  'shape': '5x30 ft line',
                                                  'save_ab': 'dex'},
                                                 {'label': 'gold',
                                                  'dmg_type': 'fire',
                                                  'shape': '15 ft cone',
                                                  'save_ab': 'dex'},
                                                 {'label': 'green',
                                                  'dmg_type': 'poison',
                                                  'shape': '15 ft cone',
                                                  'save_ab': 'con'},
                                                 {'label': 'red',
                                                  'dmg_type': 'fire',
                                                  'shape': '15 ft cone',
                                                  'save_ab': 'dex'},
                                                 {'label': 'silver',
                                                  'dmg_type': 'cold',
                                                  'shape': '15 ft cone',
                                                  'save_ab': 'con'},
                                                 {'label': 'white',
                                                  'dmg_type': 'cold',
                                                  'shape': '15 ft cone',
                                                  'save_ab': 'con'}]}},
                           {'name': 'Breath Weapon',
                            'srd': True,
                            'mech': {'kind': 'breath',
                                     'action': 'action',
                                     'shape': 'per ancestry',
                                     'save_ab': 'per ancestry',
                                     'dc': '8+prof+con',
                                     'half_on_save': True,
                                     'dice': {1: '2d6', 2: '2d6', 3: '2d6'},
                                     'dmg_type': 'per ancestry',
                                     'uses': 1,
                                     'refresh': 'short',
                                     'note': 'SRD scaling is 2d6 through '
                                             'character 5 (3d6 at 6, 4d6 at '
                                             '11, 5d6 at 16 -- all beyond '
                                             'our level cap)'}},
                           {'name': 'Damage Resistance',
                            'srd': True,
                            'mech': {'kind': 'resist',
                                     'dmg_type': 'per ancestry'}},
                           {'name': 'Languages',
                            'srd': True,
                            'mech': {'kind': 'flavor',
                                     'text': 'Common and Draconic'}}],
                'subraces': {}},
 'gnome': {'srd': True,
           'asi': {'int': 2},
           'speed': 25,
           'traits': [{'name': 'Darkvision',
                       'srd': True,
                       'mech': {'kind': 'flavor',
                                'text': 'see 60 ft in dim light as if '
                                        'bright, and in darkness as in dim '
                                        '(gray only)'}},
                      {'name': 'Gnome Cunning',
                       'srd': True,
                       'mech': {'kind': 'advantage',
                                'on': 'INT, WIS, and CHA saving throws '
                                      'against magic'}},
                      {'name': 'Languages',
                       'srd': True,
                       'mech': {'kind': 'flavor',
                                'text': 'Common and Gnomish'}}],
           'subraces': {'rock gnome': {'srd': True,
                                       'asi': {'con': 1},
                                       'traits': [{'name': "Artificer's Lore",
                                                   'srd': True,
                                                   'mech': {'kind': 'flavor',
                                                            'text': 'double '
                                                                    'proficiency '
                                                                    'on INT '
                                                                    '(History) '
                                                                    'checks '
                                                                    'about '
                                                                    'magic '
                                                                    'items, '
                                                                    'alchemy, '
                                                                    'or '
                                                                    'technology'}},
                                                  {'name': 'Tinker',
                                                   'srd': True,
                                                   'mech': {'kind': 'flavor',
                                                            'text': "tinker's "
                                                                    'tools; '
                                                                    '1 hour '
                                                                    'and 10 '
                                                                    'gp '
                                                                    'builds '
                                                                    'a Tiny '
                                                                    'clockwork '
                                                                    'toy, '
                                                                    'fire '
                                                                    'starter, '
                                                                    'or '
                                                                    'music '
                                                                    'box '
                                                                    '(runs a '
                                                                    'day; '
                                                                    'three '
                                                                    'at a '
                                                                    'time)'}}]}}},
 'half-elf': {'srd': True,
              'asi': {'cha': 2},
              'asi_choice': {'kind': 'choice',
                             'when': 'creation',
                             'choose': 2,
                             'options': [{'str': 1},
                                         {'dex': 1},
                                         {'con': 1},
                                         {'int': 1},
                                         {'wis': 1}]},
              'speed': 30,
              'traits': [{'name': 'Darkvision',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'see 60 ft in dim light as if '
                                           'bright, and in darkness as in '
                                           'dim (gray only)'}},
                         {'name': 'Fey Ancestry',
                          'srd': True,
                          'mech': {'kind': 'advantage',
                                   'on': 'saving throws against the charmed '
                                         'condition',
                                   'also': {'kind': 'passive_bonus',
                                            'what': 'sleep immunity',
                                            'value': 'magic cannot put you '
                                                     'to sleep (the sleep '
                                                     "spell's HP pool skips "
                                                     'you)'}}},
                         {'name': 'Skill Versatility',
                          'srd': True,
                          'mech': {'kind': 'passive_bonus',
                                   'what': 'skills',
                                   'skill_choice': 2,
                                   'note': 'any two of the 18 skills, picked '
                                           'at creation'}},
                         {'name': 'Languages',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'Common, Elvish, and one extra '
                                           'language of your choice'}}],
              'subraces': {}},
 'half-orc': {'srd': True,
              'asi': {'str': 2, 'con': 1},
              'speed': 30,
              'traits': [{'name': 'Darkvision',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'see 60 ft in dim light as if '
                                           'bright, and in darkness as in '
                                           'dim (gray only)'}},
                         {'name': 'Menacing',
                          'srd': True,
                          'mech': {'kind': 'passive_bonus',
                                   'what': 'skills',
                                   'value': ['intimidation']}},
                         {'name': 'Relentless Endurance',
                          'srd': True,
                          'mech': {'kind': 'survive',
                                   'when': 'reduced to 0 HP but not killed '
                                           'outright',
                                   'instead': 'drop to 1 HP',
                                   'uses': 1,
                                   'refresh': 'long',
                                   'note': 'same r5_apply_damage hook as the '
                                           "boar's Relentless, without the "
                                           'damage-size cap'}},
                         {'name': 'Savage Attacks',
                          'srd': True,
                          'mech': {'kind': 'rider',
                                   'dice': 'one extra weapon damage die',
                                   'trigger': 'you score a critical hit with '
                                              'a melee weapon attack',
                                   'note': 'the crit adds one MORE weapon '
                                           'die on top of the usual doubled '
                                           'dice'}},
                         {'name': 'Languages',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'Common and Orc'}}],
              'subraces': {}},
 'tiefling': {'srd': True,
              'asi': {'int': 1, 'cha': 2},
              'speed': 30,
              'traits': [{'name': 'Darkvision',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'see 60 ft in dim light as if '
                                           'bright, and in darkness as in '
                                           'dim (gray only)'}},
                         {'name': 'Hellish Resistance',
                          'srd': True,
                          'mech': {'kind': 'resist', 'dmg_type': 'fire'}},
                         {'name': 'Infernal Legacy',
                          'srd': True,
                          'mech': {'kind': 'always_prepared',
                                   'spells': {3: ['hellish rebuke']},
                                   'uses': 1,
                                   'refresh': 'long',
                                   'cast_at_slot': 2,
                                   'note': 'hellish rebuke stages with the '
                                           'spell stone: reaction when '
                                           'damaged by a creature you can '
                                           'see within 60 ft; 3d10 fire at a '
                                           '2nd-level slot, DEX save for '
                                           'half',
                                   'also': {'kind': 'flavor',
                                            'text': 'thaumaturgy cantrip '
                                                    'from level 1: minor '
                                                    'infernal showmanship '
                                                    '(booming voice, '
                                                    'guttering flames); '
                                                    'flavor at our scope -- '
                                                    'no combat mechanics'}}},
                         {'name': 'Languages',
                          'srd': True,
                          'mech': {'kind': 'flavor',
                                   'text': 'Common and Infernal'}}],
              'subraces': {}}}

# --------------------------------------------------------------------------
# BACKGROUNDS
#
# Name + two skills + original blurb; only acolyte is SRD-extracted.
# --------------------------------------------------------------------------

BACKGROUNDS = {'acolyte': {'skills': ['insight', 'religion'],
             'srd': True,
             'blurb': 'Temple-raised; the rites still steady your hands.'},
 'criminal': {'skills': ['deception', 'stealth'],
              'srd': False,
              'blurb': 'You learned locks, lies, and exits, in that order.'},
 'sage': {'skills': ['arcana', 'history'],
          'srd': False,
          'blurb': 'You chased answers through the stacks the way others '
                   'chase gold.'},
 'soldier': {'skills': ['athletics', 'intimidation'],
             'srd': False,
             'blurb': 'You held a line once; part of you never left it.'},
 'entertainer': {'skills': ['acrobatics', 'performance'],
                 'srd': False,
                 'blurb': 'Any room is a stage if you enter it right.'},
 'folk hero': {'skills': ['animal handling', 'survival'],
               'srd': False,
               'blurb': 'Back home they still tell the story; you were '
                        'there.'},
 'noble': {'skills': ['history', 'persuasion'],
           'srd': False,
           'blurb': 'Born to a name that opens doors you never knocked on.'},
 'outlander': {'skills': ['athletics', 'survival'],
               'srd': False,
               'blurb': 'The maps end where you grew up.'},
 'urchin': {'skills': ['sleight of hand', 'stealth'],
            'srd': False,
            'blurb': 'The city raised you sideways: all shortcuts and sharp '
                     'eyes.'},
 'haunted one': {'skills': ['investigation', 'survival'],
                 'srd': False,
                 'blurb': 'Some mornings you wake knowing something used '
                          'your hands, and liked it.'}}

# --------------------------------------------------------------------------
# SKILLS
#
# The 18 SRD skills with governing abilities; alphabetical = bitmask order.
# --------------------------------------------------------------------------

SKILLS = {'acrobatics': {'ability': 'dex'},
 'animal handling': {'ability': 'wis'},
 'arcana': {'ability': 'int'},
 'athletics': {'ability': 'str'},
 'deception': {'ability': 'cha'},
 'history': {'ability': 'int'},
 'insight': {'ability': 'wis'},
 'intimidation': {'ability': 'cha'},
 'investigation': {'ability': 'int'},
 'medicine': {'ability': 'wis'},
 'nature': {'ability': 'int'},
 'perception': {'ability': 'wis'},
 'performance': {'ability': 'cha'},
 'persuasion': {'ability': 'cha'},
 'religion': {'ability': 'int'},
 'sleight of hand': {'ability': 'dex'},
 'stealth': {'ability': 'dex'},
 'survival': {'ability': 'wis'}}

# --------------------------------------------------------------------------
# CLASSES_NEW
#
# The six classes beyond the original roster (stage_classes.py history).
# Merged into CLASSES at import bottom; generator maps stay explicit.
# --------------------------------------------------------------------------

CLASSES_NEW = {'barbarian': {'hit_die': 12,
               'saves': ['str', 'con'],
               'prof_bonus': {1: 2, 2: 2, 3: 2},
               'features': {1: ['Rage', 'Unarmored Defense'],
                            2: ['Reckless Attack', 'Danger Sense'],
                            3: ['Primal Path']},
               'rages': {1: 2, 2: 2, 3: 3},
               'rage_bonus': {1: 2, 2: 2, 3: 2}},
 'druid': {'hit_die': 8,
           'saves': ['int', 'wis'],
           'prof_bonus': {1: 2, 2: 2, 3: 2},
           'features': {1: ['Druidic', 'Spellcasting'],
                        2: ['Wild Shape', 'Druid Circle'],
                        3: []},
           'spell_slots': {1: {1: 2}, 2: {1: 3}, 3: {1: 4, 2: 2}},
           'cantrips_known': {1: 2, 2: 2, 3: 2},
           'wild_shape': {1: None,
                          2: {'uses': 2,
                              'max_cr': '1/4',
                              'fly': False,
                              'swim': False},
                          3: {'uses': 2,
                              'max_cr': '1/4',
                              'fly': False,
                              'swim': False}}},
 'monk': {'hit_die': 8,
          'saves': ['str', 'dex'],
          'prof_bonus': {1: 2, 2: 2, 3: 2},
          'features': {1: ['Unarmored Defense', 'Martial Arts'],
                       2: ['Ki',
                           'Flurry of Blows',
                           'Patient Defense',
                           'Step of the Wind',
                           'Unarmored Movement (+10 ft)'],
                       3: ['Monastic Tradition', 'Deflect Missiles']},
          'martial_arts_die': {1: '1d4', 2: '1d4', 3: '1d4'},
          'ki_points': {1: 0, 2: 2, 3: 3}},
 'paladin': {'hit_die': 10,
             'saves': ['wis', 'cha'],
             'prof_bonus': {1: 2, 2: 2, 3: 2},
             'features': {1: ['Divine Sense', 'Lay on Hands'],
                          2: ['Fighting Style',
                              'Spellcasting',
                              'Divine Smite'],
                          3: ['Divine Health',
                              'Sacred Oath',
                              'Channel Divinity (1/rest)']},
             'spell_slots': {1: {}, 2: {1: 2}, 3: {1: 3}},
             'lay_on_hands': {1: 5, 2: 10, 3: 15}},
 'sorcerer': {'hit_die': 6,
              'saves': ['con', 'cha'],
              'prof_bonus': {1: 2, 2: 2, 3: 2},
              'features': {1: ['Spellcasting', 'Sorcerous Origin'],
                           2: ['Font of Magic'],
                           3: ['Metamagic']},
              'spell_slots': {1: {1: 2}, 2: {1: 3}, 3: {1: 4, 2: 2}},
              'cantrips_known': {1: 4, 2: 4, 3: 4},
              'spells_known': {1: 2, 2: 3, 3: 4},
              'sorcery_points': {1: 0, 2: 2, 3: 3}},
 'warlock': {'hit_die': 8,
             'saves': ['wis', 'cha'],
             'prof_bonus': {1: 2, 2: 2, 3: 2},
             'features': {1: ['Otherworldly Patron', 'Pact Magic'],
                          2: ['Eldritch Invocations'],
                          3: ['Pact Boon']},
             'pact_slots': {1: {1: 1}, 2: {1: 2}, 3: {2: 2}},
             'cantrips_known': {1: 2, 2: 2, 3: 2},
             'spells_known': {1: 2, 2: 3, 3: 4},
             'invocations_known': {1: 0, 2: 2, 3: 2}}}

CLASSES.update(CLASSES_NEW)   # twelve classes total

# --------------------------------------------------------------------------
# SPELLS_FULL
#
# All 83 combat/camp spells, levels 0-2 (80 extracted + 3 merge promotions;
# stage_spells.py history carries the 48-spell skip ledger).
# --------------------------------------------------------------------------

SPELLS_FULL = {'acid splash': {'name': 'acid splash',
                 'level': 0,
                 'school': 'conjuration',
                 'classes': ['sorcerer', 'wizard'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': False,
                 'ritual': False,
                 'kind': 'save',
                 'dice': '1d6',
                 'dmg_type': 'acid',
                 'save': ('dex', 'negates'),
                 'attack': None,
                 'condition': None,
                 'rounds': None,
                 'aoe': None,
                 'targets': 'all',
                 'range': 60,
                 'duration': 'instantaneous',
                 'upcast': None,
                 'cantrip_scaling': '2d6@5 3d6@11 4d6@17; levels 1-3 see '
                                    'base tier',
                 'srd': True,
                 'effect': 'one creature, or two within 5 ft of each other; '
                           'DEX save or take 1d6 acid'},
 'chill touch': {'name': 'chill touch',
                 'level': 0,
                 'school': 'necromancy',
                 'classes': ['sorcerer', 'warlock', 'wizard'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': False,
                 'ritual': False,
                 'kind': 'attack',
                 'dice': '1d8',
                 'dmg_type': 'necrotic',
                 'save': None,
                 'attack': 'ranged_spell',
                 'condition': "no-heal (can't regain hit points)",
                 'rounds': 1,
                 'aoe': None,
                 'targets': 'one',
                 'range': 120,
                 'duration': '1 round',
                 'upcast': None,
                 'cantrip_scaling': '2d8@5 3d8@11 4d8@17; levels 1-3 see '
                                    'base tier',
                 'srd': True,
                 'effect': 'ranged spell attack for 1d8 necrotic; target '
                           "can't regain HP until the start of your next "
                           'turn; undead hit also have disadvantage on '
                           'attacks vs you for the same window'},
 'eldritch blast': {'name': 'eldritch blast',
                    'level': 0,
                    'school': 'evocation',
                    'classes': ['warlock'],
                    'cast': 'action',
                    'action': 'action',
                    'concentration': False,
                    'ritual': False,
                    'kind': 'multi',
                    'dice': {'count': 1, 'dice': '1d10', 'plus': 0},
                    'dmg_type': 'force',
                    'save': None,
                    'attack': 'ranged_spell',
                    'condition': None,
                    'rounds': None,
                    'aoe': None,
                    'targets': 'one',
                    'range': 120,
                    'duration': 'instantaneous',
                    'upcast': None,
                    'cantrip_scaling': '2 beams@5 3@11 4@17, separate attack '
                                       'rolls, may split targets; levels 1-3 '
                                       'see one beam',
                    'srd': True,
                    'effect': 'ranged spell attack for 1d10 force; '
                              'Agonizing/Repelling invocations ride on this '
                              '(character2.md budget)'},
 'fire bolt': {'name': 'fire bolt',
               'level': 0,
               'school': 'evocation',
               'classes': ['sorcerer', 'wizard'],
               'cast': 'action',
               'action': 'action',
               'concentration': False,
               'ritual': False,
               'kind': 'attack',
               'dice': '1d10',
               'dmg_type': 'fire',
               'save': None,
               'attack': 'ranged_spell',
               'condition': None,
               'rounds': None,
               'aoe': None,
               'targets': 'one',
               'range': 120,
               'duration': 'instantaneous',
               'upcast': None,
               'cantrip_scaling': '2d10@5 3d10@11 4d10@17; levels 1-3 see '
                                  'base tier',
               'srd': True,
               'effect': 'ranged spell attack for 1d10 fire; ignites '
                         'unattended flammable objects'},
 'guidance': {'name': 'guidance',
              'level': 0,
              'school': 'divination',
              'classes': ['cleric', 'druid'],
              'cast': 'action',
              'action': 'action',
              'concentration': True,
              'ritual': False,
              'kind': 'buff',
              'dice': '1d4',
              'dmg_type': None,
              'save': None,
              'attack': None,
              'condition': None,
              'rounds': 10,
              'aoe': None,
              'targets': 'one',
              'range': 0,
              'duration': '1 minute',
              'upcast': None,
              'cantrip_scaling': None,
              'srd': True,
              'effect': 'touched ally adds 1d4 to one ability check of its '
                        'choice before the spell ends (dialog-check value), '
                        'then the spell ends'},
 'poison spray': {'name': 'poison spray',
                  'level': 0,
                  'school': 'conjuration',
                  'classes': ['druid', 'sorcerer', 'warlock', 'wizard'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'save',
                  'dice': '1d12',
                  'dmg_type': 'poison',
                  'save': ('con', 'negates'),
                  'attack': None,
                  'condition': None,
                  'rounds': None,
                  'aoe': None,
                  'targets': 'one',
                  'range': 10,
                  'duration': 'instantaneous',
                  'upcast': None,
                  'cantrip_scaling': '2d12@5 3d12@11 4d12@17; levels 1-3 see '
                                     'base tier',
                  'srd': True,
                  'effect': 'CON save or take 1d12 poison'},
 'produce flame': {'name': 'produce flame',
                   'level': 0,
                   'school': 'conjuration',
                   'classes': ['druid'],
                   'cast': 'action',
                   'action': 'action',
                   'concentration': False,
                   'ritual': False,
                   'kind': 'attack',
                   'dice': '1d8',
                   'dmg_type': 'fire',
                   'save': None,
                   'attack': 'ranged_spell',
                   'condition': None,
                   'rounds': None,
                   'aoe': None,
                   'targets': 'one',
                   'range': 30,
                   'duration': '10 minutes',
                   'upcast': None,
                   'cantrip_scaling': '2d8@5 3d8@11 4d8@17; levels 1-3 see '
                                      'base tier',
                   'srd': True,
                   'effect': 'flame held in hand for 10 minutes (light); '
                             'hurl it as the cast or a later action: ranged '
                             'spell attack for 1d8 fire, which ends the '
                             'spell'},
 'ray of frost': {'name': 'ray of frost',
                  'level': 0,
                  'school': 'evocation',
                  'classes': ['sorcerer', 'wizard'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'attack',
                  'dice': '1d8',
                  'dmg_type': 'cold',
                  'save': None,
                  'attack': 'ranged_spell',
                  'condition': None,
                  'rounds': 1,
                  'aoe': None,
                  'targets': 'one',
                  'range': 60,
                  'duration': '1 round',
                  'upcast': None,
                  'cantrip_scaling': '2d8@5 3d8@11 4d8@17; levels 1-3 see '
                                     'base tier',
                  'srd': True,
                  'effect': "ranged spell attack for 1d8 cold; target's "
                            'speed -10 ft until the start of your next turn'},
 'resistance': {'name': 'resistance',
                'level': 0,
                'school': 'abjuration',
                'classes': ['cleric', 'druid'],
                'cast': 'action',
                'action': 'action',
                'concentration': True,
                'ritual': False,
                'kind': 'buff',
                'dice': '1d4',
                'dmg_type': None,
                'save': None,
                'attack': None,
                'condition': None,
                'rounds': 10,
                'aoe': None,
                'targets': 'one',
                'range': 0,
                'duration': '1 minute',
                'upcast': None,
                'cantrip_scaling': None,
                'srd': True,
                'effect': 'touched ally adds 1d4 to one saving throw of its '
                          'choice before the spell ends, then the spell ends '
                          "(guidance's twin, for saves; maps onto the "
                          'R5F_BLESS d4 machinery)'},
 'sacred flame': {'name': 'sacred flame',
                  'level': 0,
                  'school': 'evocation',
                  'classes': ['cleric'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'save',
                  'dice': '1d8',
                  'dmg_type': 'radiant',
                  'save': ('dex', 'negates'),
                  'attack': None,
                  'condition': None,
                  'rounds': None,
                  'aoe': None,
                  'targets': 'one',
                  'range': 60,
                  'duration': 'instantaneous',
                  'upcast': None,
                  'cantrip_scaling': '2d8@5 3d8@11 4d8@17; levels 1-3 see '
                                     'base tier',
                  'srd': True,
                  'effect': 'DEX save or take 1d8 radiant; target gains no '
                            'benefit from cover on this save'},
 'shillelagh': {'name': 'shillelagh',
                'level': 0,
                'school': 'transmutation',
                'classes': ['druid'],
                'cast': 'bonus',
                'action': 'bonus',
                'concentration': False,
                'ritual': False,
                'kind': 'buff',
                'dice': '1d8',
                'dmg_type': None,
                'save': None,
                'attack': None,
                'condition': None,
                'rounds': 10,
                'aoe': None,
                'targets': 'self',
                'range': 0,
                'duration': '1 minute',
                'upcast': None,
                'cantrip_scaling': None,
                'srd': True,
                'effect': 'your club or quarterstaff attacks use your '
                          'spellcasting ability instead of STR and its '
                          'damage die becomes 1d8; the weapon counts as '
                          'magical'},
 'shocking grasp': {'name': 'shocking grasp',
                    'level': 0,
                    'school': 'evocation',
                    'classes': ['sorcerer', 'wizard'],
                    'cast': 'action',
                    'action': 'action',
                    'concentration': False,
                    'ritual': False,
                    'kind': 'attack',
                    'dice': '1d8',
                    'dmg_type': 'lightning',
                    'save': None,
                    'attack': 'melee_spell',
                    'condition': None,
                    'rounds': 1,
                    'aoe': None,
                    'targets': 'one',
                    'range': 0,
                    'duration': 'instantaneous',
                    'upcast': None,
                    'cantrip_scaling': '2d8@5 3d8@11 4d8@17; levels 1-3 see '
                                       'base tier',
                    'srd': True,
                    'effect': 'melee spell attack (advantage if the target '
                              'wears metal armor) for 1d8 lightning; target '
                              "can't take reactions until the start of its "
                              'next turn'},
 'true strike': {'name': 'true strike',
                 'level': 0,
                 'school': 'divination',
                 'classes': ['bard', 'sorcerer', 'warlock', 'wizard'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': True,
                 'ritual': False,
                 'kind': 'buff',
                 'dice': None,
                 'dmg_type': None,
                 'save': None,
                 'attack': None,
                 'condition': None,
                 'rounds': 1,
                 'aoe': None,
                 'targets': 'one',
                 'range': 30,
                 'duration': '1 round',
                 'upcast': None,
                 'cantrip_scaling': None,
                 'srd': True,
                 'effect': 'advantage (R5F_ADV) on your first attack roll '
                           'against the target on your NEXT turn -- costs '
                           "this turn's action"},
 'vicious mockery': {'name': 'vicious mockery',
                     'level': 0,
                     'school': 'enchantment',
                     'classes': ['bard'],
                     'cast': 'action',
                     'action': 'action',
                     'concentration': False,
                     'ritual': False,
                     'kind': 'save',
                     'dice': '1d4',
                     'dmg_type': 'psychic',
                     'save': ('wis', 'negates'),
                     'attack': None,
                     'condition': None,
                     'rounds': 1,
                     'aoe': None,
                     'targets': 'one',
                     'range': 60,
                     'duration': 'instantaneous',
                     'upcast': None,
                     'cantrip_scaling': '2d4@5 3d4@11 4d4@17; levels 1-3 see '
                                        'base tier',
                     'srd': True,
                     'effect': 'WIS save or take 1d4 psychic and have '
                               'disadvantage on its next attack roll before '
                               'the end of its next turn'},
 'animal friendship': {'name': 'animal friendship',
                       'level': 1,
                       'school': 'enchantment',
                       'classes': ['bard', 'druid', 'ranger'],
                       'cast': 'action',
                       'action': 'action',
                       'concentration': False,
                       'ritual': False,
                       'kind': 'condition',
                       'dice': None,
                       'dmg_type': None,
                       'save': ('wis', 'negates'),
                       'attack': None,
                       'condition': 'charmed',
                       'rounds': 14400,
                       'aoe': None,
                       'targets': 'one',
                       'range': 30,
                       'duration': '24 hours',
                       'upcast': '+1 beast per slot above 1st (dnd5eapi '
                                 'omits this)',
                       'srd': True,
                       'effect': 'one beast with INT < 4: WIS save or '
                                 'charmed for the duration; ends if you or '
                                 'companions harm it (pacify the boar, not '
                                 'the imp)'},
 'bane': {'name': 'bane',
          'level': 1,
          'school': 'enchantment',
          'classes': ['bard', 'cleric'],
          'cast': 'action',
          'action': 'action',
          'concentration': True,
          'ritual': False,
          'kind': 'condition',
          'dice': '1d4',
          'dmg_type': None,
          'save': ('cha', 'negates'),
          'attack': None,
          'condition': 'baned (-1d4 on attack rolls and saves)',
          'rounds': 10,
          'aoe': None,
          'targets': 'all',
          'range': 30,
          'duration': '1 minute',
          'upcast': '+1 target per slot above 1st',
          'srd': True,
          'effect': 'up to 3 creatures: CHA save or subtract 1d4 from attack '
                    'rolls and saving throws for the duration (anti-bless; '
                    'engine can mirror the R5F_BLESS d4 as a penalty)'},
 'bless': {'name': 'bless',
           'level': 1,
           'school': 'enchantment',
           'classes': ['cleric', 'paladin'],
           'cast': 'action',
           'action': 'action',
           'concentration': True,
           'ritual': False,
           'kind': 'buff',
           'dice': '1d4',
           'dmg_type': None,
           'save': None,
           'attack': None,
           'condition': None,
           'rounds': 10,
           'aoe': None,
           'targets': 'party',
           'range': 30,
           'duration': '1 minute',
           'upcast': '+1 target per slot above 1st',
           'srd': True,
           'effect': 'up to 3 creatures add 1d4 to attack rolls and saving '
                     'throws for the duration; +1 target per slot above 1'},
 'burning hands': {'name': 'burning hands',
                   'level': 1,
                   'school': 'evocation',
                   'classes': ['sorcerer', 'wizard'],
                   'cast': 'action',
                   'action': 'action',
                   'concentration': False,
                   'ritual': False,
                   'kind': 'save',
                   'dice': '3d6',
                   'dmg_type': 'fire',
                   'save': ('dex', 'half'),
                   'attack': None,
                   'condition': None,
                   'rounds': None,
                   'aoe': {'shape': 'cone', 'size_ft': 15},
                   'targets': 'all',
                   'range': 0,
                   'duration': 'instantaneous',
                   'upcast': '+1d6 per slot above 1st',
                   'srd': True,
                   'effect': '15-ft cone; DEX save for half of 3d6 fire; '
                             'ignites unattended flammables'},
 'charm person': {'name': 'charm person',
                  'level': 1,
                  'school': 'enchantment',
                  'classes': ['bard',
                              'druid',
                              'sorcerer',
                              'warlock',
                              'wizard'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'condition',
                  'dice': None,
                  'dmg_type': None,
                  'save': ('wis', 'negates'),
                  'attack': None,
                  'condition': 'charmed',
                  'rounds': 600,
                  'aoe': None,
                  'targets': 'one',
                  'range': 30,
                  'duration': '1 hour',
                  'upcast': '+1 target per slot above 1st (within 30 ft of '
                            'each other)',
                  'srd': True,
                  'effect': 'one humanoid: WIS save (advantage if you or '
                            'companions are fighting it) or charmed -- '
                            'regards you as a friendly acquaintance; ends if '
                            'you harm it; knows afterward'},
 'color spray': {'name': 'color spray',
                 'level': 1,
                 'school': 'illusion',
                 'classes': ['sorcerer', 'wizard'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': False,
                 'ritual': False,
                 'kind': 'pool',
                 'dice': '6d10',
                 'dmg_type': None,
                 'save': None,
                 'attack': None,
                 'condition': 'blinded',
                 'rounds': 1,
                 'aoe': {'shape': 'cone', 'size_ft': 15},
                 'targets': 'all',
                 'range': 0,
                 'duration': '1 round',
                 'upcast': '+2d10 to the pool per slot above 1st',
                 'srd': True,
                 'effect': 'roll 6d10 for an HP pool; creatures in the cone '
                           'are blinded until end of your next turn in '
                           'ascending order of current HP, no save, each '
                           "subtracting its HP from the pool (sleep's "
                           'machinery, blinded instead of unconscious; skips '
                           'unconscious and sightless creatures)'},
 'command': {'name': 'command',
             'level': 1,
             'school': 'enchantment',
             'classes': ['cleric', 'paladin'],
             'cast': 'action',
             'action': 'action',
             'concentration': False,
             'ritual': False,
             'kind': 'condition',
             'dice': None,
             'dmg_type': None,
             'save': ('wis', 'negates'),
             'attack': None,
             'condition': 'commanded (grovel=prone / halt=lose turn / flee / '
                          'approach / drop)',
             'rounds': 1,
             'aoe': None,
             'targets': 'one',
             'range': 60,
             'duration': '1 round',
             'upcast': '+1 target per slot above 1st (within 30 ft of each '
                       'other)',
             'srd': True,
             'effect': 'WIS save or obey a one-word command on its next '
                       "turn; no effect on undead or if it can't understand "
                       'you'},
 'cure wounds': {'name': 'cure wounds',
                 'level': 1,
                 'school': 'evocation',
                 'classes': ['bard', 'cleric', 'druid', 'paladin', 'ranger'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': False,
                 'ritual': False,
                 'kind': 'heal',
                 'dice': '1d8',
                 'plus_mod': True,
                 'dmg_type': None,
                 'save': None,
                 'attack': None,
                 'condition': None,
                 'rounds': None,
                 'aoe': None,
                 'targets': 'one',
                 'range': 0,
                 'duration': 'instantaneous',
                 'upcast': '+1d8 per slot above 1st',
                 'srd': True,
                 'effect': 'touched creature regains 1d8 + spellcasting mod '
                           'HP; no effect on undead or constructs'},
 'divine favor': {'name': 'divine favor',
                  'level': 1,
                  'school': 'evocation',
                  'classes': ['paladin'],
                  'cast': 'bonus',
                  'action': 'bonus',
                  'concentration': True,
                  'ritual': False,
                  'kind': 'buff',
                  'dice': '1d4',
                  'dmg_type': 'radiant',
                  'save': None,
                  'attack': None,
                  'condition': None,
                  'rounds': 10,
                  'aoe': None,
                  'targets': 'self',
                  'range': 0,
                  'duration': '1 minute',
                  'upcast': None,
                  'srd': True,
                  'effect': 'your weapon attacks deal +1d4 radiant on hit '
                            "for the duration (hunter's-mark-shaped rider, "
                            'R5F_MARK pattern)'},
 'entangle': {'name': 'entangle',
              'level': 1,
              'school': 'conjuration',
              'classes': ['druid'],
              'cast': 'action',
              'action': 'action',
              'concentration': True,
              'ritual': False,
              'kind': 'condition',
              'dice': None,
              'dmg_type': None,
              'save': ('str', 'negates'),
              'attack': None,
              'condition': 'restrained',
              'rounds': 10,
              'aoe': {'shape': 'cube', 'size_ft': 20},
              'targets': 'all',
              'range': 90,
              'duration': '1 minute',
              'upcast': None,
              'srd': True,
              'effect': 'creatures in the area on cast: STR save or '
                        'restrained by vines; escape = action, STR check vs '
                        'spell DC; area is difficult terrain for the '
                        'duration'},
 'expeditious retreat': {'name': 'expeditious retreat',
                         'level': 1,
                         'school': 'transmutation',
                         'classes': ['sorcerer', 'warlock', 'wizard'],
                         'cast': 'bonus',
                         'action': 'bonus',
                         'concentration': True,
                         'ritual': False,
                         'kind': 'buff',
                         'dice': None,
                         'dmg_type': None,
                         'save': None,
                         'attack': None,
                         'condition': None,
                         'rounds': 100,
                         'aoe': None,
                         'targets': 'self',
                         'range': 0,
                         'duration': '10 minutes',
                         'upcast': None,
                         'srd': True,
                         'effect': 'Dash as a bonus action on cast and on '
                                   'each of your turns for the duration'},
 'faerie fire': {'name': 'faerie fire',
                 'level': 1,
                 'school': 'evocation',
                 'classes': ['druid'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': True,
                 'ritual': False,
                 'kind': 'condition',
                 'dice': None,
                 'dmg_type': None,
                 'save': ('dex', 'negates'),
                 'attack': None,
                 'condition': "revealed (attacks vs it have advantage; can't "
                              'be invisible)',
                 'rounds': 10,
                 'aoe': {'shape': 'cube', 'size_ft': 20},
                 'targets': 'all',
                 'range': 60,
                 'duration': '1 minute',
                 'upcast': None,
                 'srd': True,
                 'effect': 'creatures in the cube: DEX save or outlined in '
                           'light for the duration -- attack rolls against '
                           'them have advantage and invisibility does not '
                           'help (counters the imp)'},
 'false life': {'name': 'false life',
                'level': 1,
                'school': 'necromancy',
                'classes': ['sorcerer', 'wizard'],
                'cast': 'action',
                'action': 'action',
                'concentration': False,
                'ritual': False,
                'kind': 'buff',
                'dice': '1d4',
                'dmg_type': None,
                'save': None,
                'attack': None,
                'condition': None,
                'rounds': 600,
                'aoe': None,
                'targets': 'self',
                'range': 0,
                'duration': '1 hour',
                'upcast': '+5 temp HP per slot above 1st',
                'srd': True,
                'effect': 'gain 1d4 + 4 temporary hit points for the '
                          'duration (temp_hp field on R5Creature; '
                          'camp-castable pre-fight)'},
 'fog cloud': {'name': 'fog cloud',
               'level': 1,
               'school': 'conjuration',
               'classes': ['druid', 'ranger', 'sorcerer', 'wizard'],
               'cast': 'action',
               'action': 'action',
               'concentration': True,
               'ritual': False,
               'kind': 'utility',
               'dice': None,
               'dmg_type': None,
               'save': None,
               'attack': None,
               'condition': 'blinded (everyone, while inside the cloud)',
               'rounds': 600,
               'aoe': {'shape': 'sphere', 'size_ft': 20},
               'targets': 'all',
               'range': 120,
               'duration': '1 hour',
               'upcast': '+20 ft radius per slot above 1st',
               'srd': True,
               'effect': '20-ft-radius sphere is heavily obscured: attacks '
                         'into, out of, or within it are blind-vs-unseen '
                         '(adv+dis soup); defensive screen the engine can '
                         'flag-model'},
 'goodberry': {'name': 'goodberry',
               'level': 1,
               'school': 'transmutation',
               'classes': ['druid', 'ranger'],
               'cast': 'action',
               'action': 'action',
               'concentration': False,
               'ritual': False,
               'kind': 'heal',
               'dice': None,
               'dmg_type': None,
               'save': None,
               'attack': None,
               'condition': None,
               'rounds': None,
               'aoe': None,
               'targets': 'party',
               'range': 0,
               'duration': 'instantaneous',
               'upcast': None,
               'srd': True,
               'effect': '10 berries; eating one (an action) restores 1 HP '
                         'and feeds a creature for a day; berries expire in '
                         '24 h -- camp item: a slot becomes 10 HP of '
                         'out-of-combat healing'},
 'grease': {'name': 'grease',
            'level': 1,
            'school': 'conjuration',
            'classes': ['wizard'],
            'cast': 'action',
            'action': 'action',
            'concentration': False,
            'ritual': False,
            'kind': 'condition',
            'dice': None,
            'dmg_type': None,
            'save': ('dex', 'negates'),
            'attack': None,
            'condition': 'prone',
            'rounds': 10,
            'aoe': {'shape': 'cube', 'size_ft': 10},
            'targets': 'all',
            'range': 60,
            'duration': '1 minute',
            'upcast': None,
            'srd': True,
            'effect': 'creatures in the area on cast (and any entering or '
                      'ending a turn there): DEX save or fall prone; area is '
                      'difficult terrain'},
 'guiding bolt': {'name': 'guiding bolt',
                  'level': 1,
                  'school': 'evocation',
                  'classes': ['cleric'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'attack',
                  'dice': '4d6',
                  'dmg_type': 'radiant',
                  'save': None,
                  'attack': 'ranged_spell',
                  'condition': None,
                  'rounds': 1,
                  'aoe': None,
                  'targets': 'one',
                  'range': 120,
                  'duration': '1 round',
                  'upcast': '+1d6 per slot above 1st',
                  'srd': True,
                  'effect': 'ranged spell attack for 4d6 radiant; next '
                            'attack roll against the target before the end '
                            'of your next turn has advantage'},
 'healing word': {'name': 'healing word',
                  'level': 1,
                  'school': 'evocation',
                  'classes': ['bard', 'cleric', 'druid'],
                  'cast': 'bonus',
                  'action': 'bonus',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'heal',
                  'dice': '1d4',
                  'plus_mod': True,
                  'dmg_type': None,
                  'save': None,
                  'attack': None,
                  'condition': None,
                  'rounds': None,
                  'aoe': None,
                  'targets': 'one',
                  'range': 60,
                  'duration': 'instantaneous',
                  'upcast': '+1d4 per slot above 1st',
                  'srd': True,
                  'effect': 'one creature in range regains 1d4 + '
                            'spellcasting mod HP; no effect on undead or '
                            'constructs; the wake-the-fallen bonus action '
                            '(r5_heal wakes unconscious PCs)'},
 'hellish rebuke': {'name': 'hellish rebuke',
                    'level': 1,
                    'school': 'evocation',
                    'classes': ['warlock'],
                    'cast': 'reaction',
                    'action': 'reaction',
                    'concentration': False,
                    'ritual': False,
                    'kind': 'save',
                    'dice': '2d10',
                    'dmg_type': 'fire',
                    'save': ('dex', 'half'),
                    'attack': None,
                    'condition': None,
                    'rounds': None,
                    'aoe': None,
                    'targets': 'one',
                    'range': 60,
                    'duration': 'instantaneous',
                    'upcast': '+1d10 per slot above 1st',
                    'srd': True,
                    'effect': 'reaction when damaged by a creature you can '
                              'see: it makes a DEX save or takes 2d10 fire, '
                              'half on success (pointless vs the fire-immune '
                              'imp and cambion -- flavor)'},
 'heroism': {'name': 'heroism',
             'level': 1,
             'school': 'enchantment',
             'classes': ['bard', 'paladin'],
             'cast': 'action',
             'action': 'action',
             'concentration': True,
             'ritual': False,
             'kind': 'buff',
             'dice': None,
             'plus_mod': True,
             'dmg_type': None,
             'save': None,
             'attack': None,
             'condition': None,
             'rounds': 10,
             'aoe': None,
             'targets': 'one',
             'range': 0,
             'duration': '1 minute',
             'upcast': '+1 target per slot above 1st (dnd5eapi omits this)',
             'srd': True,
             'effect': 'touched ally is immune to frightened and gains temp '
                       'HP equal to your spellcasting mod at the start of '
                       'each of its turns; temp HP vanish when the spell '
                       'ends'},
 'hideous laughter': {'name': 'hideous laughter',
                      'level': 1,
                      'school': 'enchantment',
                      'classes': ['bard', 'wizard'],
                      'cast': 'action',
                      'action': 'action',
                      'concentration': True,
                      'ritual': False,
                      'kind': 'condition',
                      'dice': None,
                      'dmg_type': None,
                      'save': ('wis', 'negates'),
                      'attack': None,
                      'condition': 'incapacitated+prone',
                      'rounds': 10,
                      'aoe': None,
                      'targets': 'one',
                      'range': 30,
                      'duration': '1 minute',
                      'upcast': None,
                      'srd': True,
                      'effect': 'one creature with INT > 4: WIS save or '
                                'collapse laughing -- prone, incapacitated, '
                                "can't stand; re-saves at end of each of its "
                                'turns and when damaged (advantage if '
                                'damage-triggered)'},
 "hunter's mark": {'name': "hunter's mark",
                   'level': 1,
                   'school': 'divination',
                   'classes': ['ranger'],
                   'cast': 'bonus',
                   'action': 'bonus',
                   'concentration': True,
                   'ritual': False,
                   'kind': 'buff',
                   'dice': '1d6',
                   'dmg_type': None,
                   'save': None,
                   'attack': None,
                   'condition': None,
                   'rounds': 600,
                   'aoe': None,
                   'targets': 'one',
                   'range': 90,
                   'duration': '1 hour',
                   'upcast': None,
                   'srd': True,
                   'effect': 'mark one creature; your weapon attacks that '
                             'hit it deal +1d6 (R5F_MARK); bonus action to '
                             're-mark on kill'},
 'inflict wounds': {'name': 'inflict wounds',
                    'level': 1,
                    'school': 'necromancy',
                    'classes': ['cleric'],
                    'cast': 'action',
                    'action': 'action',
                    'concentration': False,
                    'ritual': False,
                    'kind': 'attack',
                    'dice': '3d10',
                    'dmg_type': 'necrotic',
                    'save': None,
                    'attack': 'melee_spell',
                    'condition': None,
                    'rounds': None,
                    'aoe': None,
                    'targets': 'one',
                    'range': 0,
                    'duration': 'instantaneous',
                    'upcast': '+1d10 per slot above 1st',
                    'srd': True,
                    'effect': 'melee spell attack for 3d10 necrotic -- the '
                              'biggest level-1 single hit in the game'},
 'longstrider': {'name': 'longstrider',
                 'level': 1,
                 'school': 'transmutation',
                 'classes': ['bard', 'druid', 'ranger', 'wizard'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': False,
                 'ritual': False,
                 'kind': 'buff',
                 'dice': None,
                 'dmg_type': None,
                 'save': None,
                 'attack': None,
                 'condition': None,
                 'rounds': 600,
                 'aoe': None,
                 'targets': 'one',
                 'range': 0,
                 'duration': '1 hour',
                 'upcast': '+1 target per slot above 1st',
                 'srd': True,
                 'effect': "touched creature's speed +10 ft for 1 hour"},
 'mage armor': {'name': 'mage armor',
                'level': 1,
                'school': 'abjuration',
                'classes': ['sorcerer', 'wizard'],
                'cast': 'action',
                'action': 'action',
                'concentration': False,
                'ritual': False,
                'kind': 'buff',
                'dice': None,
                'dmg_type': None,
                'save': None,
                'attack': None,
                'condition': None,
                'rounds': 4800,
                'aoe': None,
                'targets': 'one',
                'range': 0,
                'duration': '8 hours',
                'upcast': None,
                'srd': True,
                'effect': "willing unarmored creature's base AC becomes 13 + "
                          'DEX mod for 8 hours; ends if it dons armor (the '
                          'classic cast-at-camp wizard skin)'},
 'magic missile': {'name': 'magic missile',
                   'level': 1,
                   'school': 'evocation',
                   'classes': ['sorcerer', 'wizard'],
                   'cast': 'action',
                   'action': 'action',
                   'concentration': False,
                   'ritual': False,
                   'kind': 'multi',
                   'dice': {'count': 3, 'dice': '1d4', 'plus': 1},
                   'dmg_type': 'force',
                   'save': None,
                   'attack': None,
                   'condition': None,
                   'rounds': None,
                   'aoe': None,
                   'targets': 'all',
                   'range': 120,
                   'duration': 'instantaneous',
                   'upcast': '+1 dart per slot above 1st',
                   'srd': True,
                   'effect': '3 darts, each auto-hits for 1d4+1 force; darts '
                             'strike simultaneously and may split among '
                             'visible targets'},
 'protection from evil and good': {'name': 'protection from evil and good',
                                   'level': 1,
                                   'school': 'abjuration',
                                   'classes': ['cleric',
                                               'paladin',
                                               'warlock',
                                               'wizard'],
                                   'cast': 'action',
                                   'action': 'action',
                                   'concentration': True,
                                   'ritual': False,
                                   'kind': 'buff',
                                   'dice': None,
                                   'dmg_type': None,
                                   'save': None,
                                   'attack': None,
                                   'condition': None,
                                   'rounds': 100,
                                   'aoe': None,
                                   'targets': 'one',
                                   'range': 0,
                                   'duration': '10 minutes',
                                   'upcast': None,
                                   'srd': True,
                                   'effect': 'touched willing creature: '
                                             'aberrations, celestials, '
                                             'elementals, fey, fiends, and '
                                             'undead have disadvantage to '
                                             "hit it, and it can't be "
                                             'charmed/frightened/possessed '
                                             'by them -- on the nautiloid '
                                             'that is EVERY enemy (imp, '
                                             'devourer, cambion, mind '
                                             'flayer)'},
 'sanctuary': {'name': 'sanctuary',
               'level': 1,
               'school': 'abjuration',
               'classes': ['cleric'],
               'cast': 'bonus',
               'action': 'bonus',
               'concentration': False,
               'ritual': False,
               'kind': 'buff',
               'dice': None,
               'dmg_type': None,
               'save': ('wis', 'negates'),
               'attack': None,
               'condition': None,
               'rounds': 10,
               'aoe': None,
               'targets': 'one',
               'range': 30,
               'duration': '1 minute',
               'upcast': None,
               'srd': True,
               'effect': 'ward one creature: any enemy targeting it with an '
                         'attack or harmful spell first makes a WIS save or '
                         'must pick a new target / lose the action; ward '
                         'breaks if the warded creature attacks or casts '
                         'against an enemy; no help vs area effects'},
 'shield': {'name': 'shield',
            'level': 1,
            'school': 'abjuration',
            'classes': ['sorcerer', 'wizard'],
            'cast': 'reaction',
            'action': 'reaction',
            'concentration': False,
            'ritual': False,
            'kind': 'buff',
            'dice': None,
            'dmg_type': None,
            'save': None,
            'attack': None,
            'condition': None,
            'rounds': 1,
            'aoe': None,
            'targets': 'self',
            'range': 0,
            'duration': '1 round',
            'upcast': None,
            'srd': True,
            'effect': 'reaction when hit by an attack or targeted by magic '
                      'missile: +5 AC until the start of your next turn, '
                      'including vs the trigger, and magic missile deals you '
                      'nothing'},
 'shield of faith': {'name': 'shield of faith',
                     'level': 1,
                     'school': 'abjuration',
                     'classes': ['cleric', 'paladin'],
                     'cast': 'bonus',
                     'action': 'bonus',
                     'concentration': True,
                     'ritual': False,
                     'kind': 'buff',
                     'dice': None,
                     'dmg_type': None,
                     'save': None,
                     'attack': None,
                     'condition': None,
                     'rounds': 100,
                     'aoe': None,
                     'targets': 'one',
                     'range': 60,
                     'duration': '10 minutes',
                     'upcast': None,
                     'srd': True,
                     'effect': 'one creature in range gains +2 AC for the '
                               'duration'},
 'sleep': {'name': 'sleep',
           'level': 1,
           'school': 'enchantment',
           'classes': ['bard', 'sorcerer', 'wizard'],
           'cast': 'action',
           'action': 'action',
           'concentration': False,
           'ritual': False,
           'kind': 'pool',
           'dice': '5d8',
           'dmg_type': None,
           'save': None,
           'attack': None,
           'condition': 'unconscious',
           'rounds': 10,
           'aoe': {'shape': 'sphere', 'size_ft': 20},
           'targets': 'all',
           'range': 90,
           'duration': '1 minute',
           'upcast': '+2d8 to the pool per slot above 1st',
           'srd': True,
           'effect': 'roll 5d8 for an HP pool; creatures within 20 ft of the '
                     'point fall unconscious in ascending order of current '
                     "HP, no save, subtracting each sleeper's HP from the "
                     'pool; skip a creature the remaining pool cannot cover; '
                     'ends on damage or shake-awake (action); undead and '
                     'charm-immune creatures unaffected'},
 'thunderwave': {'name': 'thunderwave',
                 'level': 1,
                 'school': 'evocation',
                 'classes': ['bard', 'druid', 'sorcerer', 'wizard'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': False,
                 'ritual': False,
                 'kind': 'save',
                 'dice': '2d8',
                 'dmg_type': 'thunder',
                 'save': ('con', 'half'),
                 'attack': None,
                 'condition': None,
                 'rounds': None,
                 'aoe': {'shape': 'cube', 'size_ft': 15},
                 'targets': 'all',
                 'range': 0,
                 'duration': 'instantaneous',
                 'upcast': '+1d8 per slot above 1st',
                 'srd': True,
                 'effect': '15-ft cube from self; CON save or take 2d8 '
                           'thunder and be pushed 10 ft away (half damage, '
                           'no push, on success); audible 300 ft'},
 'ensnaring strike': {'name': 'ensnaring strike',
                      'level': 1,
                      'school': 'conjuration',
                      'classes': ['ranger'],
                      'cast': 'bonus',
                      'action': 'bonus',
                      'concentration': True,
                      'ritual': False,
                      'kind': 'condition',
                      'dice': '1d6',
                      'dmg_type': 'piercing',
                      'save': ('str', 'negates'),
                      'attack': None,
                      'condition': 'restrained',
                      'rounds': 10,
                      'aoe': None,
                      'targets': 'one',
                      'range': 0,
                      'duration': '1 minute',
                      'upcast': '+1d6 per slot above 1st',
                      'srd': True,
                      'effect': 'on a weapon hit, target makes a STR save '
                                '(advantage if Large or bigger) or is '
                                'restrained by vines and takes 1d6 piercing '
                                'at the start of each of its turns; escape = '
                                'action, STR (Athletics) vs spell DC'},
 'acid arrow': {'name': 'acid arrow',
                'level': 2,
                'school': 'evocation',
                'classes': ['wizard'],
                'cast': 'action',
                'action': 'action',
                'concentration': False,
                'ritual': False,
                'kind': 'attack',
                'dice': '4d4',
                'dmg_type': 'acid',
                'save': None,
                'attack': 'ranged_spell',
                'condition': None,
                'rounds': 1,
                'aoe': None,
                'targets': 'one',
                'range': 90,
                'duration': 'instantaneous',
                'upcast': None,
                'srd': True,
                'delayed': {'dice': '2d4', 'dmg_type': 'acid'},
                'effect': 'ranged spell attack: 4d4 acid on hit plus 2d4 '
                          "acid at the end of the target's next turn; on a "
                          'MISS still half the initial damage and no delayed '
                          'damage (unique miss-for-half attack)'},
 'aid': {'name': 'aid',
         'level': 2,
         'school': 'abjuration',
         'classes': ['cleric', 'paladin'],
         'cast': 'action',
         'action': 'action',
         'concentration': False,
         'ritual': False,
         'kind': 'buff',
         'dice': None,
         'dmg_type': None,
         'save': None,
         'attack': None,
         'condition': None,
         'rounds': 4800,
         'aoe': None,
         'targets': 'party',
         'range': 30,
         'duration': '8 hours',
         'upcast': None,
         'srd': True,
         'effect': 'up to 3 creatures get +5 max HP AND +5 current HP for 8 '
                   'hours -- premier camp buff; not temp HP, real ceiling'},
 'barkskin': {'name': 'barkskin',
              'level': 2,
              'school': 'transmutation',
              'classes': ['druid', 'ranger'],
              'cast': 'action',
              'action': 'action',
              'concentration': True,
              'ritual': False,
              'kind': 'buff',
              'dice': None,
              'dmg_type': None,
              'save': None,
              'attack': None,
              'condition': None,
              'rounds': 600,
              'aoe': None,
              'targets': 'one',
              'range': 0,
              'duration': '1 hour',
              'upcast': None,
              'srd': True,
              'effect': "touched willing creature's AC can't be below 16 for "
                        'the duration (floor, not bonus -- big for '
                        'wild-shape beasts and unarmored druids)'},
 'blindness/deafness': {'name': 'blindness/deafness',
                        'level': 2,
                        'school': 'necromancy',
                        'classes': ['bard', 'cleric', 'sorcerer', 'wizard'],
                        'cast': 'action',
                        'action': 'action',
                        'concentration': False,
                        'ritual': False,
                        'kind': 'condition',
                        'dice': None,
                        'dmg_type': None,
                        'save': ('con', 'negates'),
                        'attack': None,
                        'condition': 'blinded',
                        'rounds': 10,
                        'aoe': None,
                        'targets': 'one',
                        'range': 30,
                        'duration': '1 minute',
                        'upcast': None,
                        'srd': True,
                        'effect': 'CON save or blinded (or deafened, '
                                  "caster's choice) for 1 minute; re-save at "
                                  'the end of each of its turns; the '
                                  'no-concentration debuff'},
 'blur': {'name': 'blur',
          'level': 2,
          'school': 'illusion',
          'classes': ['sorcerer', 'wizard'],
          'cast': 'action',
          'action': 'action',
          'concentration': True,
          'ritual': False,
          'kind': 'buff',
          'dice': None,
          'dmg_type': None,
          'save': None,
          'attack': None,
          'condition': None,
          'rounds': 10,
          'aoe': None,
          'targets': 'self',
          'range': 0,
          'duration': '1 minute',
          'upcast': None,
          'srd': True,
          'effect': 'attack rolls against you have disadvantage for the '
                    "duration (attackers that don't rely on sight ignore "
                    'it)'},
 'branding smite': {'name': 'branding smite',
                    'level': 2,
                    'school': 'evocation',
                    'classes': ['paladin'],
                    'cast': 'bonus',
                    'action': 'bonus',
                    'concentration': True,
                    'ritual': False,
                    'kind': 'buff',
                    'dice': '2d6',
                    'dmg_type': 'radiant',
                    'save': None,
                    'attack': None,
                    'condition': None,
                    'rounds': 10,
                    'aoe': None,
                    'targets': 'self',
                    'range': 0,
                    'duration': '1 minute',
                    'upcast': None,
                    'srd': True,
                    'effect': 'your next weapon hit before the spell ends '
                              'deals +2d6 radiant, the target becomes '
                              "visible if invisible and can't turn invisible "
                              'while the spell lasts (smite-rider pattern; '
                              'the anti-imp smite)'},
 'calm emotions': {'name': 'calm emotions',
                   'level': 2,
                   'school': 'enchantment',
                   'classes': ['bard', 'cleric'],
                   'cast': 'action',
                   'action': 'action',
                   'concentration': True,
                   'ritual': False,
                   'kind': 'condition',
                   'dice': None,
                   'dmg_type': None,
                   'save': ('cha', 'negates'),
                   'attack': None,
                   'condition': 'pacified (or: suppresses charmed/frightened '
                                'on allies)',
                   'rounds': 10,
                   'aoe': {'shape': 'sphere', 'size_ft': 20},
                   'targets': 'all',
                   'range': 60,
                   'duration': '1 minute',
                   'upcast': None,
                   'srd': True,
                   'effect': 'humanoids in the sphere: CHA save (may fail on '
                             'purpose) or either lose charmed/frightened for '
                             "the duration (cast on allies vs the cambion's "
                             'Fiendish Charm) or turn indifferent to chosen '
                             'enemies until harmed'},
 'darkness': {'name': 'darkness',
              'level': 2,
              'school': 'evocation',
              'classes': ['sorcerer', 'warlock', 'wizard'],
              'cast': 'action',
              'action': 'action',
              'concentration': True,
              'ritual': False,
              'kind': 'utility',
              'dice': None,
              'dmg_type': None,
              'save': None,
              'attack': None,
              'condition': "blinded (everyone inside without Devil's Sight)",
              'rounds': 100,
              'aoe': {'shape': 'sphere', 'size_ft': 15},
              'targets': 'all',
              'range': 60,
              'duration': '10 minutes',
              'upcast': None,
              'srd': True,
              'effect': "15-ft-radius magical darkness: darkvision can't "
                        'pierce it, nonmagical light dies in it; mutual '
                        "blindness soup like fog cloud -- but note the imp's "
                        "Devil's Sight sees through it just fine"},
 'enhance ability': {'name': 'enhance ability',
                     'level': 2,
                     'school': 'transmutation',
                     'classes': ['bard', 'cleric', 'druid', 'sorcerer'],
                     'cast': 'action',
                     'action': 'action',
                     'concentration': True,
                     'ritual': False,
                     'kind': 'buff',
                     'dice': '2d6',
                     'dmg_type': None,
                     'save': None,
                     'attack': None,
                     'condition': None,
                     'rounds': 600,
                     'aoe': None,
                     'targets': 'one',
                     'range': 0,
                     'duration': '1 hour',
                     'upcast': None,
                     'srd': True,
                     'effect': 'touched creature gets advantage on one '
                               "ability's checks for 1 hour (choose animal: "
                               'bear also grants 2d6 temp HP) -- '
                               'dialog-check and camp value'},
 'enlarge/reduce': {'name': 'enlarge/reduce',
                    'level': 2,
                    'school': 'transmutation',
                    'classes': ['sorcerer', 'wizard'],
                    'cast': 'action',
                    'action': 'action',
                    'concentration': True,
                    'ritual': False,
                    'kind': 'buff',
                    'dice': '1d4',
                    'dmg_type': None,
                    'save': ('con', 'negates'),
                    'attack': None,
                    'condition': None,
                    'rounds': 10,
                    'aoe': None,
                    'targets': 'one',
                    'range': 30,
                    'duration': '1 minute',
                    'upcast': None,
                    'srd': True,
                    'effect': 'enlarge: +1d4 weapon damage, advantage on STR '
                              'checks and saves, size up; reduce (enemy, CON '
                              'save negates): -1d4 weapon damage, '
                              'disadvantage on STR checks and saves, size '
                              'down'},
 'flame blade': {'name': 'flame blade',
                 'level': 2,
                 'school': 'evocation',
                 'classes': ['druid'],
                 'cast': 'bonus',
                 'action': 'bonus',
                 'concentration': True,
                 'ritual': False,
                 'kind': 'attack',
                 'dice': '3d6',
                 'dmg_type': 'fire',
                 'save': None,
                 'attack': 'melee_spell',
                 'condition': None,
                 'rounds': 100,
                 'aoe': None,
                 'targets': 'one',
                 'range': 0,
                 'duration': '10 minutes',
                 'upcast': None,
                 'srd': True,
                 'effect': 'bonus action: fiery scimitar in your free hand '
                           'for 10 minutes; each ACTION you may melee spell '
                           'attack with it for 3d6 fire; sheds light; '
                           're-evoke as a bonus action if dropped'},
 'flaming sphere': {'name': 'flaming sphere',
                    'level': 2,
                    'school': 'conjuration',
                    'classes': ['druid', 'wizard'],
                    'cast': 'action',
                    'action': 'action',
                    'concentration': True,
                    'ritual': False,
                    'kind': 'save',
                    'dice': '2d6',
                    'dmg_type': 'fire',
                    'save': ('dex', 'half'),
                    'attack': None,
                    'condition': None,
                    'rounds': 10,
                    'aoe': None,
                    'targets': 'one',
                    'range': 60,
                    'duration': '1 minute',
                    'upcast': None,
                    'srd': True,
                    'effect': 'summon a 5-ft fire sphere: creatures ending '
                              'their turn within 5 ft of it save DEX for '
                              'half of 2d6 fire; bonus action each turn to '
                              'move it 30 ft and ram someone (a pet damage '
                              'engine on a bonus action)'},
 'heat metal': {'name': 'heat metal',
                'level': 2,
                'school': 'transmutation',
                'classes': ['bard', 'druid'],
                'cast': 'action',
                'action': 'action',
                'concentration': True,
                'ritual': False,
                'kind': 'save',
                'dice': '2d8',
                'dmg_type': 'fire',
                'save': ('con', 'negates'),
                'attack': None,
                'condition': None,
                'rounds': 10,
                'aoe': None,
                'targets': 'one',
                'range': 60,
                'duration': '1 minute',
                'upcast': None,
                'srd': True,
                'save_vs_damage': False,
                'effect': 'target a metal weapon or armor: its holder/wearer '
                          'takes 2d8 fire, NO save vs damage; bonus action '
                          'each turn to repeat; CON save or drop the object '
                          "(can't = disadvantage on attacks and checks until "
                          'your next turn); Commander Zhalk is a man in '
                          'metal holding metal'},
 'hold person': {'name': 'hold person',
                 'level': 2,
                 'school': 'enchantment',
                 'classes': ['bard',
                             'cleric',
                             'druid',
                             'sorcerer',
                             'warlock',
                             'wizard'],
                 'cast': 'action',
                 'action': 'action',
                 'concentration': True,
                 'ritual': False,
                 'kind': 'condition',
                 'dice': None,
                 'dmg_type': None,
                 'save': ('wis', 'negates'),
                 'attack': None,
                 'condition': 'paralyzed',
                 'rounds': 10,
                 'aoe': None,
                 'targets': 'one',
                 'range': 60,
                 'duration': '1 minute',
                 'upcast': None,
                 'srd': True,
                 'effect': 'one HUMANOID: WIS save or paralyzed; re-save at '
                           'end of each of its turns; paralyzed = auto-crit '
                           'within 5 ft (R5F_AUTOCRIT) -- works on thralls '
                           'and cultists, not fiends or aberrations'},
 'invisibility': {'name': 'invisibility',
                  'level': 2,
                  'school': 'illusion',
                  'classes': ['bard', 'sorcerer', 'warlock', 'wizard'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': True,
                  'ritual': False,
                  'kind': 'buff',
                  'dice': None,
                  'dmg_type': None,
                  'save': None,
                  'attack': None,
                  'condition': 'invisible',
                  'rounds': 600,
                  'aoe': None,
                  'targets': 'one',
                  'range': 0,
                  'duration': '1 hour',
                  'upcast': None,
                  'srd': True,
                  'effect': 'touched creature is invisible until it attacks '
                            'or casts a spell: attacks against it have '
                            'disadvantage, its attacks have advantage (the '
                            "imp's own trick, engine condition already "
                            'exists)'},
 'lesser restoration': {'name': 'lesser restoration',
                        'level': 2,
                        'school': 'abjuration',
                        'classes': ['bard',
                                    'cleric',
                                    'druid',
                                    'paladin',
                                    'ranger'],
                        'cast': 'action',
                        'action': 'action',
                        'concentration': False,
                        'ritual': False,
                        'kind': 'heal',
                        'dice': None,
                        'dmg_type': None,
                        'save': None,
                        'attack': None,
                        'condition': None,
                        'rounds': None,
                        'aoe': None,
                        'targets': 'one',
                        'range': 0,
                        'duration': 'instantaneous',
                        'upcast': None,
                        'srd': True,
                        'effect': 'end one disease or one of: blinded, '
                                  'deafened, paralyzed, poisoned on the '
                                  'touched creature (all four are engine '
                                  'conditions)'},
 'levitate': {'name': 'levitate',
              'level': 2,
              'school': 'transmutation',
              'classes': ['sorcerer', 'wizard'],
              'cast': 'action',
              'action': 'action',
              'concentration': True,
              'ritual': False,
              'kind': 'condition',
              'dice': None,
              'dmg_type': None,
              'save': ('con', 'negates'),
              'attack': None,
              'condition': 'levitated (floats 20 ft up: speed 0, melee-only '
                           "creatures can't reach or be reached)",
              'rounds': 100,
              'aoe': None,
              'targets': 'one',
              'range': 60,
              'duration': '10 minutes',
              'upcast': None,
              'srd': True,
              'effect': 'one creature up to 500 lb: CON save (unwilling '
                        'only) or float 20 ft up for the duration -- a melee '
                        'monster becomes a pinata for archers; also castable '
                        'on an ally to escape melee'},
 'magic weapon': {'name': 'magic weapon',
                  'level': 2,
                  'school': 'transmutation',
                  'classes': ['paladin', 'wizard'],
                  'cast': 'bonus',
                  'action': 'bonus',
                  'concentration': True,
                  'ritual': False,
                  'kind': 'buff',
                  'dice': None,
                  'dmg_type': None,
                  'save': None,
                  'attack': None,
                  'condition': None,
                  'rounds': 600,
                  'aoe': None,
                  'targets': 'one',
                  'range': 0,
                  'duration': '1 hour',
                  'upcast': None,
                  'srd': True,
                  'effect': 'touched nonmagical weapon becomes magical with '
                            '+1 to attack and damage rolls for 1 hour (beats '
                            "the imp's resistance to nonmagical, nonsilvered "
                            'weapons)'},
 'mirror image': {'name': 'mirror image',
                  'level': 2,
                  'school': 'illusion',
                  'classes': ['sorcerer', 'warlock', 'wizard'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'buff',
                  'dice': None,
                  'dmg_type': None,
                  'save': None,
                  'attack': None,
                  'condition': None,
                  'rounds': 10,
                  'aoe': None,
                  'targets': 'self',
                  'range': 0,
                  'duration': '1 minute',
                  'upcast': None,
                  'srd': True,
                  'effect': '3 duplicates; each incoming attack rolls a d20: '
                            '6+ / 8+ / 11+ (3/2/1 dupes left) redirects it '
                            'to a duplicate with AC 10 + your DEX mod, '
                            'destroying it on a hit -- sight-based attackers '
                            'only'},
 'misty step': {'name': 'misty step',
                'level': 2,
                'school': 'conjuration',
                'classes': ['sorcerer', 'warlock', 'wizard'],
                'cast': 'bonus',
                'action': 'bonus',
                'concentration': False,
                'ritual': False,
                'kind': 'utility',
                'dice': None,
                'dmg_type': None,
                'save': None,
                'attack': None,
                'condition': None,
                'rounds': None,
                'aoe': None,
                'targets': 'self',
                'range': 0,
                'duration': 'instantaneous',
                'upcast': None,
                'srd': True,
                'effect': 'bonus action: teleport up to 30 ft to a spot you '
                          'can see; escapes grapples, '
                          'restraints-by-position, and melee lockdown '
                          'without provoking'},
 'moonbeam': {'name': 'moonbeam',
              'level': 2,
              'school': 'evocation',
              'classes': ['druid'],
              'cast': 'action',
              'action': 'action',
              'concentration': True,
              'ritual': False,
              'kind': 'save',
              'dice': '2d10',
              'dmg_type': 'radiant',
              'save': ('con', 'half'),
              'attack': None,
              'condition': None,
              'rounds': 10,
              'aoe': {'shape': 'sphere', 'size_ft': 5},
              'targets': 'all',
              'range': 120,
              'duration': '1 minute',
              'upcast': None,
              'srd': True,
              'effect': '5-ft-radius beam: a creature entering it or '
                        'starting its turn there saves CON for half of 2d10 '
                        'radiant; shapechangers save at disadvantage and '
                        'revert on a fail (the anti-imp zone); action each '
                        'turn to move the beam 60 ft'},
 'prayer of healing': {'name': 'prayer of healing',
                       'level': 2,
                       'school': 'evocation',
                       'classes': ['cleric'],
                       'cast': 'action',
                       'action': 'action',
                       'concentration': False,
                       'ritual': False,
                       'kind': 'heal',
                       'dice': '2d8',
                       'plus_mod': True,
                       'dmg_type': None,
                       'save': None,
                       'attack': None,
                       'condition': None,
                       'rounds': None,
                       'aoe': None,
                       'targets': 'party',
                       'range': 30,
                       'duration': 'instantaneous',
                       'upcast': None,
                       'srd': True,
                       'effect': '10-minute cast (CAMP ONLY): up to six '
                                 'creatures each regain 2d8 + spellcasting '
                                 'mod HP; no effect on undead or constructs '
                                 '-- one slot, whole party topped up'},
 'protection from poison': {'name': 'protection from poison',
                            'level': 2,
                            'school': 'abjuration',
                            'classes': ['cleric',
                                        'druid',
                                        'paladin',
                                        'ranger'],
                            'cast': 'action',
                            'action': 'action',
                            'concentration': False,
                            'ritual': False,
                            'kind': 'buff',
                            'dice': None,
                            'dmg_type': None,
                            'save': None,
                            'attack': None,
                            'condition': None,
                            'rounds': 600,
                            'aoe': None,
                            'targets': 'one',
                            'range': 0,
                            'duration': '1 hour',
                            'upcast': None,
                            'srd': True,
                            'effect': 'touched creature: neutralize one '
                                      'poison afflicting it; for 1 hour it '
                                      'has resistance to poison damage and '
                                      'advantage on saves vs poisoned (the '
                                      "imp's sting loses its teeth)"},
 'ray of enfeeblement': {'name': 'ray of enfeeblement',
                         'level': 2,
                         'school': 'necromancy',
                         'classes': ['warlock', 'wizard'],
                         'cast': 'action',
                         'action': 'action',
                         'concentration': True,
                         'ritual': False,
                         'kind': 'condition',
                         'dice': None,
                         'dmg_type': None,
                         'save': ('con', 'negates'),
                         'attack': 'ranged_spell',
                         'condition': 'enfeebled (STR-based weapon damage '
                                      'halved)',
                         'rounds': 10,
                         'aoe': None,
                         'targets': 'one',
                         'range': 60,
                         'duration': '1 minute',
                         'upcast': None,
                         'srd': True,
                         'effect': 'ranged spell attack: on hit the target '
                                   'deals half damage with STR weapon '
                                   'attacks; CON save at the end of each of '
                                   "ITS turns ends it (halves Zhalk's "
                                   'greatsword)'},
 'scorching ray': {'name': 'scorching ray',
                   'level': 2,
                   'school': 'evocation',
                   'classes': ['sorcerer', 'wizard'],
                   'cast': 'action',
                   'action': 'action',
                   'concentration': False,
                   'ritual': False,
                   'kind': 'multi',
                   'dice': {'count': 3, 'dice': '2d6', 'plus': 0},
                   'dmg_type': 'fire',
                   'save': None,
                   'attack': 'ranged_spell',
                   'condition': None,
                   'rounds': None,
                   'aoe': None,
                   'targets': 'all',
                   'range': 120,
                   'duration': 'instantaneous',
                   'upcast': None,
                   'srd': True,
                   'effect': '3 rays, a separate ranged spell attack each '
                             'for 2d6 fire, split among targets freely '
                             "(magic missile's swingier big sibling)"},
 'see invisibility': {'name': 'see invisibility',
                      'level': 2,
                      'school': 'divination',
                      'classes': ['bard', 'sorcerer', 'wizard'],
                      'cast': 'action',
                      'action': 'action',
                      'concentration': False,
                      'ritual': False,
                      'kind': 'buff',
                      'dice': None,
                      'dmg_type': None,
                      'save': None,
                      'attack': None,
                      'condition': None,
                      'rounds': 600,
                      'aoe': None,
                      'targets': 'self',
                      'range': 0,
                      'duration': '1 hour',
                      'upcast': None,
                      'srd': True,
                      'effect': 'for 1 hour you see invisible creatures and '
                                "objects (and into the Ethereal): the imp's "
                                'Invisibility action stops working on you -- '
                                "kept precisely because the prologue's "
                                'signature enemy vanishes'},
 'shatter': {'name': 'shatter',
             'level': 2,
             'school': 'evocation',
             'classes': ['bard', 'sorcerer', 'warlock', 'wizard'],
             'cast': 'action',
             'action': 'action',
             'concentration': False,
             'ritual': False,
             'kind': 'save',
             'dice': '3d8',
             'dmg_type': 'thunder',
             'save': ('con', 'half'),
             'attack': None,
             'condition': None,
             'rounds': None,
             'aoe': {'shape': 'sphere', 'size_ft': 10},
             'targets': 'all',
             'range': 60,
             'duration': 'instantaneous',
             'upcast': None,
             'srd': True,
             'effect': '10-ft-radius boom at a point in range: CON save for '
                       'half of 3d8 thunder; inorganic creatures save at '
                       "disadvantage; the tier's fireball-before-fireball"},
 'silence': {'name': 'silence',
             'level': 2,
             'school': 'illusion',
             'classes': ['bard', 'cleric', 'ranger'],
             'cast': 'action',
             'action': 'action',
             'concentration': True,
             'ritual': True,
             'kind': 'utility',
             'dice': None,
             'dmg_type': None,
             'save': None,
             'attack': None,
             'condition': 'deafened (while fully inside; also '
                          'thunder-immune)',
             'rounds': 100,
             'aoe': {'shape': 'sphere', 'size_ft': 20},
             'targets': 'all',
             'range': 120,
             'duration': '10 minutes',
             'upcast': None,
             'srd': True,
             'effect': '20-ft-radius soundless zone: creatures fully inside '
                       'are deafened and immune to thunder damage; verbal '
                       'spell-casting is impossible there (mind flayer '
                       'psionics need no components -- it will not care; '
                       'cultist casters will)'},
 'spike growth': {'name': 'spike growth',
                  'level': 2,
                  'school': 'transmutation',
                  'classes': ['druid', 'ranger'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': True,
                  'ritual': False,
                  'kind': 'utility',
                  'dice': '2d4',
                  'dmg_type': 'piercing',
                  'save': None,
                  'attack': None,
                  'condition': None,
                  'rounds': 100,
                  'aoe': {'shape': 'sphere', 'size_ft': 20},
                  'targets': 'all',
                  'range': 150,
                  'duration': '10 minutes',
                  'upcast': None,
                  'srd': True,
                  'effect': '20-ft-radius ground sprouts spikes, camouflaged '
                            'and difficult terrain: 2d4 piercing per 5 ft a '
                            'creature moves through it (engine: charge '
                            'damage tiers when an enemy closes through the '
                            'zone)'},
 'spiritual weapon': {'name': 'spiritual weapon',
                      'level': 2,
                      'school': 'evocation',
                      'classes': ['cleric'],
                      'cast': 'bonus',
                      'action': 'bonus',
                      'concentration': False,
                      'ritual': False,
                      'kind': 'attack',
                      'dice': '1d8',
                      'plus_mod': True,
                      'dmg_type': 'force',
                      'save': None,
                      'attack': 'melee_spell',
                      'condition': None,
                      'rounds': 10,
                      'aoe': None,
                      'targets': 'one',
                      'range': 60,
                      'duration': '1 minute',
                      'upcast': None,
                      'srd': True,
                      'effect': 'bonus action: spectral weapon appears and '
                                'swings -- melee spell attack for 1d8 + '
                                'spellcasting mod force; every later turn, '
                                'bonus action to move it 20 ft and swing '
                                "again; no concentration = the cleric's "
                                'whole-fight bonus-action engine'},
 'suggestion': {'name': 'suggestion',
                'level': 2,
                'school': 'enchantment',
                'classes': ['bard', 'sorcerer', 'warlock', 'wizard'],
                'cast': 'action',
                'action': 'action',
                'concentration': True,
                'ritual': False,
                'kind': 'condition',
                'dice': None,
                'dmg_type': None,
                'save': ('wis', 'negates'),
                'attack': None,
                'condition': 'charmed',
                'rounds': 4800,
                'aoe': None,
                'targets': 'one',
                'range': 30,
                'duration': '8 hours',
                'upcast': None,
                'srd': True,
                'effect': 'suggest a reasonable-sounding course of action (a '
                          'sentence or two) to a creature that hears and '
                          'understands you: WIS save or it pursues the '
                          'suggestion; charm-immune creatures unaffected; '
                          "ends if you or companions damage it ('walk away' "
                          '= one enemy exits the encounter)'},
 'warding bond': {'name': 'warding bond',
                  'level': 2,
                  'school': 'abjuration',
                  'classes': ['cleric'],
                  'cast': 'action',
                  'action': 'action',
                  'concentration': False,
                  'ritual': False,
                  'kind': 'buff',
                  'dice': None,
                  'dmg_type': None,
                  'save': None,
                  'attack': None,
                  'condition': None,
                  'rounds': 600,
                  'aoe': None,
                  'targets': 'one',
                  'range': 0,
                  'duration': '1 hour',
                  'upcast': None,
                  'srd': True,
                  'effect': 'bond with a touched ally for 1 hour: it gains '
                            '+1 AC, +1 to saves, and resistance to ALL '
                            'damage -- and the caster takes matching damage '
                            'each time it is hurt; ends if the caster drops '
                            'to 0 HP (tank-links the cleric to the squishy)'},
 'web': {'name': 'web',
         'level': 2,
         'school': 'conjuration',
         'classes': ['sorcerer', 'wizard'],
         'cast': 'action',
         'action': 'action',
         'concentration': True,
         'ritual': False,
         'kind': 'condition',
         'dice': None,
         'dmg_type': None,
         'save': ('dex', 'negates'),
         'attack': None,
         'condition': 'restrained',
         'rounds': 600,
         'aoe': {'shape': 'cube', 'size_ft': 20},
         'targets': 'all',
         'range': 60,
         'duration': '1 hour',
         'upcast': None,
         'srd': True,
         'effect': '20-ft cube of webs (needs anchoring surfaces): a '
                   'creature starting its turn in or entering them saves DEX '
                   'or is restrained; escape = action, STR check vs spell '
                   'DC; webs are difficult terrain and flammable (fire burns '
                   'a 5-ft cube per round for 2d4 fire)'},
 'pass without trace': {'name': 'pass without trace',
                        'level': 2,
                        'school': 'abjuration',
                        'classes': ['druid', 'ranger'],
                        'concentration': True,
                        'ritual': False,
                        'action': 'action',
                        'kind': 'buff',
                        'buff': {'what': 'party stealth',
                                 'amount': '+10 stealth; halves patrol '
                                           'detection radius (field)'},
                        'aoe': None,
                        'upcast': '',
                        'srd': True,
                        'note': 'promoted at merge: the skills/patrol system '
                                'gives it mechanics; Masks kit'},
 'silent image': {'name': 'silent image',
                  'level': 1,
                  'school': 'illusion',
                  'classes': ['bard', 'sorcerer', 'wizard'],
                  'concentration': True,
                  'ritual': False,
                  'action': 'action',
                  'kind': 'utility',
                  'aoe': None,
                  'upcast': '',
                  'srd': True,
                  'note': 'promoted at merge: field-flavor illusion for the '
                          'Masks writ'},
 'detect thoughts': {'name': 'detect thoughts',
                     'level': 2,
                     'school': 'divination',
                     'classes': ['bard', 'sorcerer', 'wizard'],
                     'concentration': True,
                     'ritual': False,
                     'action': 'action',
                     'kind': 'utility',
                     'aoe': None,
                     'upcast': '',
                     'srd': True,
                     'note': 'promoted at merge: field/dialog beat '
                             '(psychic-tadpole game); Voice kit'}}

SPELLS = dict(SPELLS, **SPELLS_FULL)   # superset; legacy 13 verified identical
