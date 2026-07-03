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
#   MONSTERS   - imp and boar from SRD 5.1 (dnd5eapi /api/2014/monsters).
#                intellect devourer, cambion, and mind flayer appear in NO SRD
#                (neither 5.1 nor 5.2.1); their game statistics are reconstructed
#                from the 2014 Monster Manual and are marked "srd": False.
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
