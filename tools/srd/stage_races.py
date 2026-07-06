# stage_races.py -- STAGED SRD extraction: the nine SRD 5.1 races (with
# their four SRD subraces), one homebrew race, ten backgrounds, and the
# 18-skill table.  Review, then merge into srd_data.py (character2.md,
# "Races, backgrounds, stats, skills").
#
# Plain Python literals only: no imports, no dataclasses.  This file feeds
# the same C code generator as srd_data.py.
#
# Provenance (see tools/srd/ATTRIBUTION.md for legal text):
#   RACES       - SRD 5.1 (dnd5eapi /api/2014/races/* and /api/2014/
#                 subraces/*, trait text from /api/2014/traits/*, fetched
#                 2026-07-06).  The draconic ancestry table is from the
#                 ten /api/2014/traits/draconic-ancestry-* records.
#   RACES_HB    - homebrew under the piscodemon doctrine (docs/
#                 character2.md): the race NAME is used plainly per the
#                 design doc; every stat, trait name, and line of text is
#                 ours -- nothing from MToF or BG3.
#   BACKGROUNDS - acolyte is the ONLY background in SRD 5.1, extracted
#                 from /api/2014/backgrounds/acolyte.  The other nine use
#                 generic English names and skill pairings matching
#                 common table expectations (pairings are facts, not
#                 protectable expression); every blurb is original prose.
#   SKILLS      - SRD 5.1 (/api/2014/skills), each skill's governing
#                 ability verified record-by-record.  Identical in 5.2.1.
#
# THE 5.2.1 DIVERGENCE THAT MATTERS (noted once, NOT adopted): the 2024
# rules remove ability score increases from races entirely -- backgrounds
# carry +2/+1 (or +1/+1/+1) instead -- and rename races "species".  This
# game keeps fixed 5.1 racial ASIs (docs/character2.md; BG3's floating
# +2/+1 is a Tasha's-ism we likewise do not adopt).  SRD 5.2.1 also drops
# the half-elf and half-orc entirely (goliath and orc replace them); both
# stay here on their 5.1 stats.  Smaller deltas are inline at each race.
#
# Mech records reuse the stage_homebrew.py vocabulary (passive_bonus /
# advantage / impose_dis / rider / resource / temp_hp / crit_range /
# always_prepared / flavor, plus the "choice" wrapper -- a new "when":
# "creation" value marks picks made once at character creation).  Four
# kinds are NEW here:
#   reroll   dice-core flag (halfling Lucky): reroll the named natural
#            roll once and keep the new result
#   survive  drop to 1 HP instead of 0 (r5_apply_damage hook), per-rest
#   breath   save-gated AoE exhalation: shape / save ability / dice by
#            character level; a per-rest resource
#   resist   permanent damage-type resistance (R5Creature.resist bit)
# Darkvision, languages, and movement speed are flavor at our scope (no
# light, language, or move economy); speeds are recorded for the sheet.
# Skill proficiencies use passive_bonus what="skills": a fixed list in
# "value", or "skill_choice": N for player picks -> the u32 skill mask.

# --------------------------------------------------------------------------
# RACES (SRD 5.1)
#
# name -> srd, asi (fixed 5.1 increases, lowercase ability keys), speed
# (recorded; flavor at our scope), traits (feature records in API order),
# subraces.  Half-elf adds "asi_choice" (+1 to each of two abilities of
# the player's choice) -- the only 5.1 race with a floating ASI part.
# Sizes are not modeled (halfling and gnome are Small; the engine has no
# size economy).
# --------------------------------------------------------------------------

RACES = {
    # 5.2.1: speed 30, darkvision 120 ft, ALL dwarves gain Dwarven
    # Toughness, subraces gone, Stonecunning becomes a tremorsense rite,
    # and Dwarven Combat Training is dropped.  None adopted.
    "dwarf": {
        "srd": True,
        "asi": {"con": 2},
        "speed": 25,
        "traits": [
            {"name": "Darkvision", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "see 60 ft in dim light as if bright, and "
                              "in darkness as in dim (gray only)"}},
            {"name": "Dwarven Resilience", "srd": True,
             "mech": {"kind": "advantage",
                      "on": "saving throws against poison",
                      "also": {"kind": "resist", "dmg_type": "poison"}}},
            {"name": "Stonecunning", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "double proficiency on INT (History) "
                              "checks about the origin of stonework"}},
            {"name": "Dwarven Combat Training", "srd": True,
             "mech": {"kind": "passive_bonus", "what": "proficiencies",
                      "value": ["battleaxe", "handaxe", "light hammer",
                                "warhammer"],
                      "note": "handaxe and warhammer are in srd_data."
                              "WEAPONS; battleaxe and light hammer are "
                              "not staged"}},
            {"name": "Tool Proficiency", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "artisan's tools: smith's, brewer's, or "
                              "mason's (your pick)"}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor", "text": "Common and Dwarvish"}},
        ],
        "subraces": {
            "hill dwarf": {
                "srd": True,
                "asi": {"wis": 1},
                "traits": [
                    {"name": "Dwarven Toughness", "srd": True,
                     "mech": {"kind": "passive_bonus", "what": "hp",
                              "amount": "+1 per character level "
                                        "(including 1st)"}},
                ],
            },
        },
    },

    # 5.2.1: three lineages (drow/high/wood) replace subraces; Keen
    # Senses becomes a pick of Insight/Perception/Survival; the magic-
    # sleep immunity moves from Fey Ancestry into Trance; the high-elf
    # cantrip becomes prestidigitation (re-pickable per long rest).
    "elf": {
        "srd": True,
        "asi": {"dex": 2},
        "speed": 30,
        "traits": [
            {"name": "Darkvision", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "see 60 ft in dim light as if bright, and "
                              "in darkness as in dim (gray only)"}},
            {"name": "Fey Ancestry", "srd": True,
             "mech": {"kind": "advantage",
                      "on": "saving throws against the charmed condition",
                      "also": {"kind": "passive_bonus",
                               "what": "sleep immunity",
                               "value": "magic cannot put you to sleep "
                                        "(the sleep spell's HP pool "
                                        "skips you)"}}},
            {"name": "Trance", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "no sleep; 4 hours of semiconscious "
                              "meditation serve as a night's rest"}},
            {"name": "Keen Senses", "srd": True,
             "mech": {"kind": "passive_bonus", "what": "skills",
                      "value": ["perception"]}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor", "text": "Common and Elvish"}},
        ],
        "subraces": {
            "high elf": {
                "srd": True,
                "asi": {"int": 1},
                "traits": [
                    {"name": "Elf Weapon Training", "srd": True,
                     "mech": {"kind": "passive_bonus",
                              "what": "proficiencies",
                              "value": ["longsword", "shortsword",
                                        "shortbow", "longbow"],
                              "note": "all four are in srd_data.WEAPONS "
                                      "and the engine roster"}},
                    # SRD: any wizard cantrip, INT is its casting
                    # ability.  All 14 SRD 5.1 wizard cantrips are
                    # encoded; the generator keeps only those present in
                    # the staged spell set.
                    {"name": "High Elf Cantrip", "srd": True,
                     "mech": {"kind": "choice", "when": "creation",
                              "options": [
                         {"label": "acid splash",
                          "kind": "always_prepared",
                          "spells": ["acid splash"]},
                         {"label": "chill touch",
                          "kind": "always_prepared",
                          "spells": ["chill touch"]},
                         {"label": "dancing lights",
                          "kind": "always_prepared",
                          "spells": ["dancing lights"]},
                         {"label": "fire bolt",
                          "kind": "always_prepared",
                          "spells": ["fire bolt"]},
                         {"label": "light",
                          "kind": "always_prepared",
                          "spells": ["light"]},
                         {"label": "mage hand",
                          "kind": "always_prepared",
                          "spells": ["mage hand"]},
                         {"label": "mending",
                          "kind": "always_prepared",
                          "spells": ["mending"]},
                         {"label": "message",
                          "kind": "always_prepared",
                          "spells": ["message"]},
                         {"label": "minor illusion",
                          "kind": "always_prepared",
                          "spells": ["minor illusion"]},
                         {"label": "poison spray",
                          "kind": "always_prepared",
                          "spells": ["poison spray"]},
                         {"label": "prestidigitation",
                          "kind": "always_prepared",
                          "spells": ["prestidigitation"]},
                         {"label": "ray of frost",
                          "kind": "always_prepared",
                          "spells": ["ray of frost"]},
                         {"label": "shocking grasp",
                          "kind": "always_prepared",
                          "spells": ["shocking grasp"]},
                         {"label": "true strike",
                          "kind": "always_prepared",
                          "spells": ["true strike"]},
                     ]}},
                    {"name": "Extra Language", "srd": True,
                     "mech": {"kind": "flavor",
                              "text": "one extra language of your "
                                      "choice"}},
                ],
            },
        },
    },

    # Small (no size economy).  5.2.1: speed 30, Lucky is renamed Luck
    # and covers any D20 Test, Naturally Stealthy is baseline for all
    # halflings, subraces gone.
    "halfling": {
        "srd": True,
        "asi": {"dex": 2},
        "speed": 25,
        "traits": [
            {"name": "Brave", "srd": True,
             "mech": {"kind": "advantage",
                      "on": "saving throws against the frightened "
                            "condition"}},
            {"name": "Halfling Nimbleness", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "move through the space of any creature "
                              "one size larger than you (no move "
                              "economy at our scope)"}},
            {"name": "Lucky", "srd": True,
             "mech": {"kind": "reroll",
                      "on": "a natural 1 on an attack roll, ability "
                            "check, or saving throw",
                      "keep": "the new roll",
                      "note": "dice-core flag on the d20 path "
                              "(character2.md); reroll once, the new "
                              "result stands even if it is another 1"}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor", "text": "Common and Halfling"}},
        ],
        "subraces": {
            "lightfoot halfling": {
                "srd": True,
                "asi": {"cha": 1},
                "traits": [
                    {"name": "Naturally Stealthy", "srd": True,
                     "mech": {"kind": "flavor",
                              "text": "you can hide behind creatures at "
                                      "least one size larger than you "
                                      "(positional; not in the design "
                                      "doc's mech list)"}},
                ],
            },
        },
    },

    # 5.2.1: the six +1s are gone entirely; the 2024 human instead gets
    # Resourceful (Heroic Inspiration per long rest), Skillful (one
    # skill), and Versatile (an origin feat).  Not adopted.
    "human": {
        "srd": True,
        "asi": {"str": 1, "dex": 1, "con": 1,
                "int": 1, "wis": 1, "cha": 1},
        "speed": 30,
        "traits": [
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "Common and one extra language of your "
                              "choice"}},
        ],
        "subraces": {},
    },

    # 5.2.1: gains darkvision 60; breath weapon replaces one attack in
    # the Attack action, shape chosen per use (15-ft cone or 30-ft
    # line), the save is always DEX, 1d10 scaling by level, and uses =
    # prof bonus per long rest; Draconic Flight arrives at character 5.
    # The ancestry damage-type map below is unchanged in 5.2.1.
    "dragonborn": {
        "srd": True,
        "asi": {"str": 2, "cha": 1},
        "speed": 30,
        "traits": [
            # The one place options are NOT primitive mechs: each
            # ancestry is a parameter record that sets dmg_type / shape /
            # save_ab for the two traits below.  Our engine has no
            # geometry -- breath resolves as targets "all" (the enemy
            # side) and shape is kept for text/UI; the live knobs are
            # the damage type and the save ability.
            {"name": "Draconic Ancestry", "srd": True,
             "mech": {"kind": "choice", "when": "creation",
                      "options": [
                 {"label": "black", "dmg_type": "acid",
                  "shape": "5x30 ft line", "save_ab": "dex"},
                 {"label": "blue", "dmg_type": "lightning",
                  "shape": "5x30 ft line", "save_ab": "dex"},
                 {"label": "brass", "dmg_type": "fire",
                  "shape": "5x30 ft line", "save_ab": "dex"},
                 {"label": "bronze", "dmg_type": "lightning",
                  "shape": "5x30 ft line", "save_ab": "dex"},
                 {"label": "copper", "dmg_type": "acid",
                  "shape": "5x30 ft line", "save_ab": "dex"},
                 {"label": "gold", "dmg_type": "fire",
                  "shape": "15 ft cone", "save_ab": "dex"},
                 {"label": "green", "dmg_type": "poison",
                  "shape": "15 ft cone", "save_ab": "con"},
                 {"label": "red", "dmg_type": "fire",
                  "shape": "15 ft cone", "save_ab": "dex"},
                 {"label": "silver", "dmg_type": "cold",
                  "shape": "15 ft cone", "save_ab": "con"},
                 {"label": "white", "dmg_type": "cold",
                  "shape": "15 ft cone", "save_ab": "con"},
             ]}},
            {"name": "Breath Weapon", "srd": True,
             "mech": {"kind": "breath", "action": "action",
                      "shape": "per ancestry",
                      "save_ab": "per ancestry", "dc": "8+prof+con",
                      "half_on_save": True,
                      "dice": {1: "2d6", 2: "2d6", 3: "2d6"},
                      "dmg_type": "per ancestry",
                      "uses": 1, "refresh": "short",
                      "note": "SRD scaling is 2d6 through character 5 "
                              "(3d6 at 6, 4d6 at 11, 5d6 at 16 -- all "
                              "beyond our level cap)"}},
            {"name": "Damage Resistance", "srd": True,
             "mech": {"kind": "resist", "dmg_type": "per ancestry"}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor", "text": "Common and Draconic"}},
        ],
        "subraces": {},
    },

    # Small (no size economy).  5.2.1: speed 30; Gnomish Cunning drops
    # the against-magic rider (advantage on ALL INT/WIS/CHA saves);
    # lineages (forest/rock) replace subraces, and the 2024 rock lineage
    # grants mending + a prestidigitation-style tinker cantrip.
    "gnome": {
        "srd": True,
        "asi": {"int": 2},
        "speed": 25,
        "traits": [
            {"name": "Darkvision", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "see 60 ft in dim light as if bright, and "
                              "in darkness as in dim (gray only)"}},
            {"name": "Gnome Cunning", "srd": True,
             "mech": {"kind": "advantage",
                      "on": "INT, WIS, and CHA saving throws against "
                            "magic"}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor", "text": "Common and Gnomish"}},
        ],
        "subraces": {
            "rock gnome": {
                "srd": True,
                "asi": {"con": 1},
                "traits": [
                    {"name": "Artificer's Lore", "srd": True,
                     "mech": {"kind": "flavor",
                              "text": "double proficiency on INT "
                                      "(History) checks about magic "
                                      "items, alchemy, or technology"}},
                    {"name": "Tinker", "srd": True,
                     "mech": {"kind": "flavor",
                              "text": "tinker's tools; 1 hour and 10 gp "
                                      "builds a Tiny clockwork toy, "
                                      "fire starter, or music box "
                                      "(runs a day; three at a time)"}},
                ],
            },
        },
    },

    # Absent from SRD 5.2.1 entirely (no half-elf species in the 2024
    # rules); kept here on SRD 5.1 stats.
    "half-elf": {
        "srd": True,
        "asi": {"cha": 2},
        # +1 to each of two different abilities of the player's choice
        # (CHA excluded -- it already has the +2).
        "asi_choice": {"kind": "choice", "when": "creation", "choose": 2,
                       "options": [{"str": 1}, {"dex": 1}, {"con": 1},
                                   {"int": 1}, {"wis": 1}]},
        "speed": 30,
        "traits": [
            {"name": "Darkvision", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "see 60 ft in dim light as if bright, and "
                              "in darkness as in dim (gray only)"}},
            {"name": "Fey Ancestry", "srd": True,
             "mech": {"kind": "advantage",
                      "on": "saving throws against the charmed condition",
                      "also": {"kind": "passive_bonus",
                               "what": "sleep immunity",
                               "value": "magic cannot put you to sleep "
                                        "(the sleep spell's HP pool "
                                        "skips you)"}}},
            {"name": "Skill Versatility", "srd": True,
             "mech": {"kind": "passive_bonus", "what": "skills",
                      "skill_choice": 2,
                      "note": "any two of the 18 skills, picked at "
                              "creation"}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "Common, Elvish, and one extra language "
                              "of your choice"}},
        ],
        "subraces": {},
    },

    # Absent from SRD 5.2.1 entirely (the 2024 rules ship an orc species
    # with a different kit); kept here on SRD 5.1 stats.
    "half-orc": {
        "srd": True,
        "asi": {"str": 2, "con": 1},
        "speed": 30,
        "traits": [
            {"name": "Darkvision", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "see 60 ft in dim light as if bright, and "
                              "in darkness as in dim (gray only)"}},
            {"name": "Menacing", "srd": True,
             "mech": {"kind": "passive_bonus", "what": "skills",
                      "value": ["intimidation"]}},
            {"name": "Relentless Endurance", "srd": True,
             "mech": {"kind": "survive",
                      "when": "reduced to 0 HP but not killed outright",
                      "instead": "drop to 1 HP",
                      "uses": 1, "refresh": "long",
                      "note": "same r5_apply_damage hook as the boar's "
                              "Relentless, without the damage-size cap"}},
            {"name": "Savage Attacks", "srd": True,
             "mech": {"kind": "rider",
                      "dice": "one extra weapon damage die",
                      "trigger": "you score a critical hit with a melee "
                                 "weapon attack",
                      "note": "the crit adds one MORE weapon die on top "
                              "of the usual doubled dice"}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor", "text": "Common and Orc"}},
        ],
        "subraces": {},
    },

    # 5.2.1: tieflings choose one of three fiendish legacies (abyssal /
    # chthonic / infernal); the infernal legacy keeps fire resistance,
    # thaumaturgy, hellish rebuke at character 3, and darkness at 5, with
    # the casting ability picked from INT/WIS/CHA.  Structure kept 5.1.
    "tiefling": {
        "srd": True,
        "asi": {"int": 1, "cha": 2},
        "speed": 30,
        "traits": [
            {"name": "Darkvision", "srd": True,
             "mech": {"kind": "flavor",
                      "text": "see 60 ft in dim light as if bright, and "
                              "in darkness as in dim (gray only)"}},
            {"name": "Hellish Resistance", "srd": True,
             "mech": {"kind": "resist", "dmg_type": "fire"}},
            # SRD: thaumaturgy cantrip at 1; hellish rebuke once per
            # long rest as a 2nd-level spell from character level 3;
            # darkness from character level 5 (beyond our cap).  CHA is
            # the casting ability.  Keys here are CHARACTER levels.
            {"name": "Infernal Legacy", "srd": True,
             "mech": {"kind": "always_prepared",
                      "spells": {3: ["hellish rebuke"]},
                      "uses": 1, "refresh": "long", "cast_at_slot": 2,
                      "note": "hellish rebuke stages with the spell "
                              "stone: reaction when damaged by a "
                              "creature you can see within 60 ft; 3d10 "
                              "fire at a 2nd-level slot, DEX save for "
                              "half",
                      "also": {"kind": "flavor",
                               "text": "thaumaturgy cantrip from level "
                                       "1: minor infernal showmanship "
                                       "(booming voice, guttering "
                                       "flames); flavor at our scope -- "
                                       "no combat mechanics"}}},
            {"name": "Languages", "srd": True,
             "mech": {"kind": "flavor", "text": "Common and Infernal"}},
        ],
        "subraces": {},
    },
}

# --------------------------------------------------------------------------
# RACES_HB -- homebrew races, piscodemon doctrine (original mechanics,
# original text; "srd": False, overrides.py convention).
# --------------------------------------------------------------------------

RACES_HB = {
    "githyanki": {
        "srd": False,
        "blurb": "Sword-drilled exiles of a stolen people: they trust "
                 "steel, discipline, and very little else.",
        # OUR spread, not an SRD fact: +2 STR for the sword-first martial
        # culture, +1 INT for the psionic schooling -- the pair any
        # blade-and-mind culture suggests, and the right fit for origin
        # Lae'zel (fighter first, book-learning second).
        "asi": {"str": 2, "int": 1},
        "speed": 30,
        "traits": [
            {"name": "Warrior-Caste Schooling", "srd": False,
             "mech": {"kind": "passive_bonus", "what": "proficiencies",
                      "value": ["light armor", "medium armor",
                                "shortsword", "longsword",
                                "greatsword"]}},
            # Mage-hand-flavored minor psionics, kept modest: one field
            # check per long rest, not a combat mechanic.
            {"name": "Silver Reach", "srd": False,
             "mech": {"kind": "advantage",
                      "on": "one Sleight of Hand or Investigation "
                            "check, declared before rolling",
                      "uses": 1, "refresh": "long",
                      "note": "an unseen hand of will does the fine "
                              "work at ten paces (R5F_ADV on a "
                              "field_check)"}},
            {"name": "Languages", "srd": False,
             "mech": {"kind": "flavor", "text": "Common and Gith"}},
        ],
        "subraces": {},
    },
}

# --------------------------------------------------------------------------
# BACKGROUNDS
#
# At our scope a background IS two skill proficiencies plus one line of
# texture (character2.md).  Acolyte is the only SRD 5.1 background and is
# extracted (skills verbatim from the API record); the other nine carry
# common-expectation skill pairings under generic English names, with
# original blurbs.  Haunted One is the Dark Urge default and uses OUR
# fixed pairing.  5.2.1 note (not adopted): 2024 backgrounds carry the
# ASIs (+2/+1 among three fixed abilities), an origin feat, and a tool --
# e.g. the 5.2.1 acolyte keeps Insight+Religion but adds INT/WIS/CHA
# increases and Magic Initiate (Cleric).
# --------------------------------------------------------------------------

BACKGROUNDS = {
    "acolyte": {
        "skills": ["insight", "religion"], "srd": True,
        "blurb": "Temple-raised; the rites still steady your hands.",
    },
    "criminal": {
        "skills": ["deception", "stealth"], "srd": False,
        "blurb": "You learned locks, lies, and exits, in that order.",
    },
    "sage": {
        "skills": ["arcana", "history"], "srd": False,
        "blurb": "You chased answers through the stacks the way others "
                 "chase gold.",
    },
    "soldier": {
        "skills": ["athletics", "intimidation"], "srd": False,
        "blurb": "You held a line once; part of you never left it.",
    },
    "entertainer": {
        "skills": ["acrobatics", "performance"], "srd": False,
        "blurb": "Any room is a stage if you enter it right.",
    },
    "folk hero": {
        "skills": ["animal handling", "survival"], "srd": False,
        "blurb": "Back home they still tell the story; you were there.",
    },
    "noble": {
        "skills": ["history", "persuasion"], "srd": False,
        "blurb": "Born to a name that opens doors you never knocked on.",
    },
    "outlander": {
        "skills": ["athletics", "survival"], "srd": False,
        "blurb": "The maps end where you grew up.",
    },
    "urchin": {
        "skills": ["sleight of hand", "stealth"], "srd": False,
        "blurb": "The city raised you sideways: all shortcuts and sharp "
                 "eyes.",
    },
    # The Dark Urge's default background (docs/character2.md).
    "haunted one": {
        "skills": ["investigation", "survival"], "srd": False,
        "blurb": "Some mornings you wake knowing something used your "
                 "hands, and liked it.",
    },
}

# --------------------------------------------------------------------------
# SKILLS -- the canonical 18, each with its governing ability (SRD fact,
# verified per /api/2014/skills/*; the 5.2.1 list and abilities are
# identical).  The u32 proficiency bitmask uses THIS dict's (alphabetical)
# order.  Sources of proficiency: class picks (choice vector), background
# (2 fixed), race bits (passive_bonus what="skills").
# --------------------------------------------------------------------------

SKILLS = {
    "acrobatics":      {"ability": "dex"},
    "animal handling": {"ability": "wis"},
    "arcana":          {"ability": "int"},
    "athletics":       {"ability": "str"},
    "deception":       {"ability": "cha"},
    "history":         {"ability": "int"},
    "insight":         {"ability": "wis"},
    "intimidation":    {"ability": "cha"},
    "investigation":   {"ability": "int"},
    "medicine":        {"ability": "wis"},
    "nature":          {"ability": "int"},
    "perception":      {"ability": "wis"},
    "performance":     {"ability": "cha"},
    "persuasion":      {"ability": "cha"},
    "religion":        {"ability": "int"},
    "sleight of hand": {"ability": "dex"},
    "stealth":         {"ability": "dex"},
    "survival":        {"ability": "wis"},
}
