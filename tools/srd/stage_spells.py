# stage_spells.py -- staged SRD 5.1 spell extraction, levels 0-2, full caster set.
#
# STAGING FILE: feeds review/merge into srd_data.py SPELLS; nothing imports it
# yet.  Plain Python literals only: no imports, no dataclasses (srd_data.py
# discipline).
#
# Provenance: dnd5eapi /api/2014/spells?level=0..2 (== SRD 5.1), fetched
# 2026-07-06; every per-spell detail record pulled and distilled by hand.
# 5.1 is canonical; SRD 5.2.1 mechanical divergences are noted inline where
# known.  "srd": True on every record -- this table is pure SRD, no homebrew.
# Sole exception to the 5.1 baseline: ensnaring strike is absent from SRD 5.1
# and is encoded from SRD 5.2.1, exactly matching the existing srd_data.py
# entry (the game already ships it).
#
# Scope: every SRD 5.1 spell of levels 0-2 on the bard / cleric / druid /
# paladin / ranger / sorcerer / warlock / wizard lists that is usable in
# combat or camp.  Pure-exploration utilities a room-based GBA prologue can
# never express are skipped (ledger below).  79 of 127 API spells kept, plus
# ensnaring strike = 80 records: 14 cantrips, 33 level 1, 33 level 2.
#
# Schema: superset of the srd_data.py SPELLS record style.  Existing fields
# keep their exact meaning (level, school, cast, concentration, dice,
# dmg_type, plus_mod, save, attack, effect, targets, range, duration).
# Cross-checked against the 13 existing SPELLS entries: NO disagreements;
# ensnaring strike carried over verbatim on 5.2.1 provenance.
#
# New / extended fields:
#   name       - == the dict key (redundant on purpose; codegen can assert).
#   classes    - SRD 5.1 class lists per dnd5eapi.  Where the SRD 5.1 "Spell
#                Lists" chapter disagrees with the API, the API value is kept
#                and the difference is inline-commented (faerie fire).
#   ritual     - bool.
#   action     - "action" | "bonus" | "reaction"; "cast" is kept as a legacy
#                alias with the identical value ("reaction" is a new value
#                for both fields).
#   kind       - "attack" (spell attack roll) | "save" (save-gated damage) |
#                "heal" | "buff" (incl. camp buffs and cleanses' cousins) |
#                "condition" (save applies a condition) | "multi" (separate
#                simultaneous hits: darts/rays/beams) | "pool" (sleep-style
#                HP pool) | "utility" (zones, mobility).
#   dice       - "NdS" or {"count", "dice", "plus"} for kind "multi", as in
#                the existing magic missile entry.  For "heal" it is the
#                healing roll; for "pool" the pool roll.
#   condition  - condition applied (engine names where they exist; custom
#                strings like "revealed" / "baned" are commented) or None.
#   rounds     - machine duration in 6-second rounds (1 min = 10, 10 min =
#                100, 1 h = 600, 8 h = 4800, 24 h = 14400); None when
#                instantaneous or not meaningfully timed.
#   aoe        - None | {"shape": "line|cone|sphere|cube", "size_ft": n}.
#                True cylinders are recorded as spheres of the same radius
#                with the real shape in a comment (engine approximates all
#                shapes as target-count tiers anyway).
#   upcast     - level-1 spells: what one slot level buys (1st->2nd is the
#                only upcast reachable at character levels 1-3).  Level-2
#                spells and cantrips: None.
#   cantrip_scaling - cantrips only: the 5/11/17 tiers, recorded but NOT
#                modeled; levels 1-3 only ever see the base tier.
#   targets    - existing "one" | "all" | "party" plus new "self".
#   range      - feet; 0 = touch or self (existing convention).
#   save_vs_damage - only present (False) when a spell's save does NOT gate
#                its damage (heat metal).
#   srd        - True (see provenance).
#
# --------------------------------------------------------------------------
# SKIPPED (48): pure exploration/social utilities out of reach of a
# room-based prologue.  One-word reasons.
#
# Level 0 (10 of 24 skipped):
#   dancing lights (light)     druidcraft (flavor)      light (light)
#   mage hand (telekinesis)    mending (repair)         message (whisper)
#   minor illusion (illusion)  prestidigitation (flavor)
#   spare the dying (death-saves: engine has no death saving throws --
#     unconscious-until-healed makes "stable" meaningless)
#   thaumaturgy (flavor)
#
# Level 1 (17 of 49 skipped):
#   alarm (ward)               comprehend languages (language)
#   create or destroy water (water)
#   detect evil and good (detection)   detect magic (detection)
#   detect poison and disease (detection)
#   disguise self (disguise)   feather fall (falling)
#   find familiar (familiar)   floating disk (hauling)
#   identify (appraisal)       illusory script (forgery)
#   jump (jumping)             purify food and drink (provisioning)
#   silent image (illusion)    speak with animals (parley)
#   unseen servant (servant)
#
# Level 2 (21 of 54 skipped):
#   alter self (disguise)      animal messenger (courier)
#   arcane lock (lock)         arcanist's magic aura (forgery)
#   augury (divination)        continual flame (light)
#   darkvision (vision)        detect thoughts (telepathy)
#   enthrall (distraction)     find steed (mount)
#   find traps (detection)     gentle repose (mortuary)
#   gust of wind (push-only: engine has no forced-movement concept; unlike
#     thunderwave there is no damage to ride on)
#   knock (lock)               locate animals or plants (detection)
#   locate object (detection)  magic mouth (recording)
#   pass without trace (stealth)       rope trick (hideout)
#   spider climb (climbing)    zone of truth (interrogation)
# --------------------------------------------------------------------------

SPELLS_FULL = {

    # ---------------------------------------------------------- cantrips --

    "acid splash": {
        "name": "acid splash",
        "level": 0, "school": "conjuration",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "save",
        "dice": "1d6", "dmg_type": "acid",
        "save": ("dex", "negates"), "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,   # not an area: one creature, or two within 5 ft
        "targets": "all", "range": 60, "duration": "instantaneous",
        "upcast": None,
        "cantrip_scaling": "2d6@5 3d6@11 4d6@17; levels 1-3 see base tier",
        "srd": True,
        "effect": "one creature, or two within 5 ft of each other; DEX save "
                  "or take 1d6 acid",
    },
    "chill touch": {
        "name": "chill touch",
        "level": 0, "school": "necromancy",
        "classes": ["sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "1d8", "dmg_type": "necrotic",
        "save": None, "attack": "ranged_spell",
        # custom condition: healing lockout, not one of the 14
        "condition": "no-heal (can't regain hit points)", "rounds": 1,
        "aoe": None,
        "targets": "one", "range": 120, "duration": "1 round",
        "upcast": None,
        "cantrip_scaling": "2d8@5 3d8@11 4d8@17; levels 1-3 see base tier",
        "srd": True,
        # SRD 5.2.1 reworks chill touch entirely (touch range, 1d10).
        "effect": "ranged spell attack for 1d8 necrotic; target can't regain "
                  "HP until the start of your next turn; undead hit also "
                  "have disadvantage on attacks vs you for the same window",
    },
    "eldritch blast": {
        "name": "eldritch blast",
        "level": 0, "school": "evocation",
        "classes": ["warlock"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "multi",   # 1 beam at this tier; beams scale by char level
        "dice": {"count": 1, "dice": "1d10", "plus": 0}, "dmg_type": "force",
        "save": None, "attack": "ranged_spell",
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 120, "duration": "instantaneous",
        "upcast": None,
        "cantrip_scaling": "2 beams@5 3@11 4@17, separate attack rolls, may "
                           "split targets; levels 1-3 see one beam",
        "srd": True,
        "effect": "ranged spell attack for 1d10 force; Agonizing/Repelling "
                  "invocations ride on this (character2.md budget)",
    },
    "fire bolt": {
        "name": "fire bolt",
        "level": 0, "school": "evocation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "1d10", "dmg_type": "fire",
        "save": None, "attack": "ranged_spell",
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 120, "duration": "instantaneous",
        "upcast": None,
        "cantrip_scaling": "2d10@5 3d10@11 4d10@17; levels 1-3 see base tier",
        "srd": True,
        "effect": "ranged spell attack for 1d10 fire; ignites unattended "
                  "flammable objects",   # matches existing SPELLS entry
    },
    "guidance": {
        "name": "guidance",
        "level": 0, "school": "divination",
        "classes": ["cleric", "druid"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": "1d4", "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 minute",
        "upcast": None,
        "cantrip_scaling": None,
        "srd": True,
        "effect": "touched ally adds 1d4 to one ability check of its choice "
                  "before the spell ends (dialog-check value), then the "
                  "spell ends",
    },
    "poison spray": {
        "name": "poison spray",
        "level": 0, "school": "conjuration",
        "classes": ["druid", "sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "save",
        "dice": "1d12", "dmg_type": "poison",
        "save": ("con", "negates"), "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 10, "duration": "instantaneous",
        "upcast": None,
        "cantrip_scaling": "2d12@5 3d12@11 4d12@17; levels 1-3 see base tier",
        "srd": True,
        # note: useless vs the imp (poison immune) -- good teaching moment
        "effect": "CON save or take 1d12 poison",
    },
    "produce flame": {
        "name": "produce flame",
        "level": 0, "school": "conjuration",
        "classes": ["druid"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "1d8", "dmg_type": "fire",
        "save": None, "attack": "ranged_spell",
        "condition": None, "rounds": None,
        "aoe": None,
        # range Self to hold (sheds light 10 ft); hurling it reaches 30 ft
        "targets": "one", "range": 30, "duration": "10 minutes",
        "upcast": None,
        "cantrip_scaling": "2d8@5 3d8@11 4d8@17; levels 1-3 see base tier",
        "srd": True,
        "effect": "flame held in hand for 10 minutes (light); hurl it as the "
                  "cast or a later action: ranged spell attack for 1d8 fire, "
                  "which ends the spell",
    },
    "ray of frost": {
        "name": "ray of frost",
        "level": 0, "school": "evocation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "1d8", "dmg_type": "cold",
        "save": None, "attack": "ranged_spell",
        "condition": None, "rounds": 1,   # the speed tax
        "aoe": None,
        "targets": "one", "range": 60, "duration": "1 round",
        "upcast": None,
        "cantrip_scaling": "2d8@5 3d8@11 4d8@17; levels 1-3 see base tier",
        "srd": True,
        "effect": "ranged spell attack for 1d8 cold; target's speed -10 ft "
                  "until the start of your next turn",
    },
    "resistance": {
        "name": "resistance",
        "level": 0, "school": "abjuration",
        "classes": ["cleric", "druid"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": "1d4", "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 minute",
        "upcast": None,
        "cantrip_scaling": None,
        "srd": True,
        "effect": "touched ally adds 1d4 to one saving throw of its choice "
                  "before the spell ends, then the spell ends (guidance's "
                  "twin, for saves; maps onto the R5F_BLESS d4 machinery)",
    },
    "sacred flame": {
        "name": "sacred flame",
        "level": 0, "school": "evocation",
        "classes": ["cleric"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "save",
        "dice": "1d8", "dmg_type": "radiant",
        "save": ("dex", "negates"), "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "instantaneous",
        "upcast": None,
        "cantrip_scaling": "2d8@5 3d8@11 4d8@17; levels 1-3 see base tier",
        "srd": True,
        "effect": "DEX save or take 1d8 radiant; target gains no benefit "
                  "from cover on this save",
    },
    "shillelagh": {
        "name": "shillelagh",
        "level": 0, "school": "transmutation",
        "classes": ["druid"],
        "cast": "bonus", "action": "bonus",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": "1d8", "dmg_type": None,   # the weapon's new damage die
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 minute",
        "upcast": None,
        "cantrip_scaling": None,
        "srd": True,
        "effect": "your club or quarterstaff attacks use your spellcasting "
                  "ability instead of STR and its damage die becomes 1d8; "
                  "the weapon counts as magical",
    },
    "shocking grasp": {
        "name": "shocking grasp",
        "level": 0, "school": "evocation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "1d8", "dmg_type": "lightning",
        "save": None, "attack": "melee_spell",
        "condition": None, "rounds": 1,   # the no-reactions rider
        "aoe": None,
        "targets": "one", "range": 0, "duration": "instantaneous",
        "upcast": None,
        "cantrip_scaling": "2d8@5 3d8@11 4d8@17; levels 1-3 see base tier",
        "srd": True,
        "effect": "melee spell attack (advantage if the target wears metal "
                  "armor) for 1d8 lightning; target can't take reactions "
                  "until the start of its next turn",
    },
    "true strike": {
        "name": "true strike",
        "level": 0, "school": "divination",
        "classes": ["bard", "sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 1,
        "aoe": None,
        "targets": "one", "range": 30, "duration": "1 round",
        "upcast": None,
        "cantrip_scaling": None,
        "srd": True,
        # SRD 5.2.1 redesigns true strike completely (attack with the weapon
        # using your spellcasting ability, +radiant at higher levels); the
        # famously weak 5.1 version below is canonical here.
        "effect": "advantage (R5F_ADV) on your first attack roll against the "
                  "target on your NEXT turn -- costs this turn's action",
    },
    "vicious mockery": {
        "name": "vicious mockery",
        "level": 0, "school": "enchantment",
        "classes": ["bard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "save",
        # SRD 5.2.1 raises this to 1d6; 5.1 baseline 1d4 kept, matching the
        # existing srd_data.py entry.
        "dice": "1d4", "dmg_type": "psychic",
        "save": ("wis", "negates"), "attack": None,
        "condition": None, "rounds": 1,   # the disadvantage rider
        "aoe": None,
        "targets": "one", "range": 60, "duration": "instantaneous",
        "upcast": None,
        "cantrip_scaling": "2d4@5 3d4@11 4d4@17; levels 1-3 see base tier",
        "srd": True,
        "effect": "WIS save or take 1d4 psychic and have disadvantage on its "
                  "next attack roll before the end of its next turn",
    },

    # ----------------------------------------------------------- level 1 --

    "animal friendship": {
        "name": "animal friendship",
        "level": 1, "school": "enchantment",
        "classes": ["bard", "druid", "ranger"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("wis", "negates"), "attack": None,
        "condition": "charmed", "rounds": 14400,   # 24 hours
        "aoe": None,
        "targets": "one", "range": 30, "duration": "24 hours",
        # PHB/SRD 5.1 print text has "+1 beast per slot above 1st";
        # dnd5eapi omits the At Higher Levels block.  Recorded per print.
        "upcast": "+1 beast per slot above 1st (dnd5eapi omits this)",
        "srd": True,
        "effect": "one beast with INT < 4: WIS save or charmed for the "
                  "duration; ends if you or companions harm it (pacify the "
                  "boar, not the imp)",
    },
    "bane": {
        "name": "bane",
        "level": 1, "school": "enchantment",
        "classes": ["bard", "cleric"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": "1d4", "dmg_type": None,
        "save": ("cha", "negates"), "attack": None,
        # custom condition: bless's mirror image, a -1d4 flag
        "condition": "baned (-1d4 on attack rolls and saves)", "rounds": 10,
        "aoe": None,
        "targets": "all", "range": 30, "duration": "1 minute",
        "upcast": "+1 target per slot above 1st",
        "srd": True,
        "effect": "up to 3 creatures: CHA save or subtract 1d4 from attack "
                  "rolls and saving throws for the duration (anti-bless; "
                  "engine can mirror the R5F_BLESS d4 as a penalty)",
    },
    "bless": {
        "name": "bless",
        "level": 1, "school": "enchantment",
        "classes": ["cleric", "paladin"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": "1d4", "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "party", "range": 30, "duration": "1 minute",
        "upcast": "+1 target per slot above 1st",
        "srd": True,
        "effect": "up to 3 creatures add 1d4 to attack rolls and saving "
                  "throws for the duration; +1 target per slot above 1",
    },   # matches existing SPELLS entry exactly
    "burning hands": {
        "name": "burning hands",
        "level": 1, "school": "evocation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "save",
        "dice": "3d6", "dmg_type": "fire",
        "save": ("dex", "half"), "attack": None,
        "condition": None, "rounds": None,
        "aoe": {"shape": "cone", "size_ft": 15},
        "targets": "all", "range": 0, "duration": "instantaneous",
        "upcast": "+1d6 per slot above 1st",
        "srd": True,
        "effect": "15-ft cone; DEX save for half of 3d6 fire; ignites "
                  "unattended flammables",   # matches existing entry
    },
    "charm person": {
        "name": "charm person",
        "level": 1, "school": "enchantment",
        "classes": ["bard", "druid", "sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("wis", "negates"), "attack": None,
        "condition": "charmed", "rounds": 600,   # 1 hour
        "aoe": None,
        "targets": "one", "range": 30, "duration": "1 hour",
        "upcast": "+1 target per slot above 1st (within 30 ft of each other)",
        "srd": True,
        "effect": "one humanoid: WIS save (advantage if you or companions "
                  "are fighting it) or charmed -- regards you as a friendly "
                  "acquaintance; ends if you harm it; knows afterward",
    },
    "color spray": {
        "name": "color spray",
        "level": 1, "school": "illusion",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "pool",
        "dice": "6d10", "dmg_type": None,
        "save": None, "attack": None,
        "condition": "blinded", "rounds": 1,
        "aoe": {"shape": "cone", "size_ft": 15},
        "targets": "all", "range": 0, "duration": "1 round",
        "upcast": "+2d10 to the pool per slot above 1st",
        "srd": True,
        "effect": "roll 6d10 for an HP pool; creatures in the cone are "
                  "blinded until end of your next turn in ascending order "
                  "of current HP, no save, each subtracting its HP from the "
                  "pool (sleep's machinery, blinded instead of unconscious; "
                  "skips unconscious and sightless creatures)",
    },
    "command": {
        "name": "command",
        "level": 1, "school": "enchantment",
        "classes": ["cleric", "paladin"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("wis", "negates"), "attack": None,
        # one-word command; Grovel maps to C_PRONE, Halt to a lost turn
        "condition": "commanded (grovel=prone / halt=lose turn / flee / "
                     "approach / drop)", "rounds": 1,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "1 round",
        "upcast": "+1 target per slot above 1st (within 30 ft of each other)",
        "srd": True,
        "effect": "WIS save or obey a one-word command on its next turn; no "
                  "effect on undead or if it can't understand you",
    },
    "cure wounds": {
        "name": "cure wounds",
        "level": 1, "school": "evocation",
        "classes": ["bard", "cleric", "druid", "paladin", "ranger"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "heal",
        "dice": "1d8", "plus_mod": True, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "instantaneous",
        "upcast": "+1d8 per slot above 1st",
        "srd": True,
        # SRD 5.2.1: school abjuration, 2d8 + mod; 5.1 baseline kept,
        # matching the existing srd_data.py entry.
        "effect": "touched creature regains 1d8 + spellcasting mod HP; no "
                  "effect on undead or constructs",
    },
    "divine favor": {
        "name": "divine favor",
        "level": 1, "school": "evocation",
        "classes": ["paladin"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": "1d4", "dmg_type": "radiant",
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 minute",
        "upcast": None,   # no At Higher Levels in 5.1
        "srd": True,
        # SRD 5.2.1 drops concentration; 5.1 (concentration) kept.
        "effect": "your weapon attacks deal +1d4 radiant on hit for the "
                  "duration (hunter's-mark-shaped rider, R5F_MARK pattern)",
    },
    "entangle": {
        "name": "entangle",
        "level": 1, "school": "conjuration",
        "classes": ["druid"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("str", "negates"), "attack": None,
        "condition": "restrained", "rounds": 10,
        "aoe": {"shape": "cube", "size_ft": 20},   # RAW: 20-ft square
        "targets": "all", "range": 90, "duration": "1 minute",
        "upcast": None,   # no At Higher Levels in 5.1
        "srd": True,
        "effect": "creatures in the area on cast: STR save or restrained by "
                  "vines; escape = action, STR check vs spell DC; area is "
                  "difficult terrain for the duration",
    },
    "expeditious retreat": {
        "name": "expeditious retreat",
        "level": 1, "school": "transmutation",
        "classes": ["sorcerer", "warlock", "wizard"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 100,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "10 minutes",
        "upcast": None,
        "srd": True,
        # kept (in-combat mobility the battle layer owns); the passive
        # travel spells (jump, feather fall) are skipped instead
        "effect": "Dash as a bonus action on cast and on each of your turns "
                  "for the duration",
    },
    "faerie fire": {
        "name": "faerie fire",
        "level": 1, "school": "evocation",
        # dnd5eapi lists druid only; the SRD 5.1 Spell Lists chapter also
        # puts faerie fire on the BARD list.  API kept as canonical -- the
        # merger may want to add "bard" here.
        "classes": ["druid"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("dex", "negates"), "attack": None,
        # custom condition: attack-advantage beacon, cancels invisibility
        "condition": "revealed (attacks vs it have advantage; can't be "
                     "invisible)", "rounds": 10,
        "aoe": {"shape": "cube", "size_ft": 20},
        "targets": "all", "range": 60, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "creatures in the cube: DEX save or outlined in light for "
                  "the duration -- attack rolls against them have advantage "
                  "and invisibility does not help (counters the imp)",
    },
    "false life": {
        "name": "false life",
        "level": 1, "school": "necromancy",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": "1d4", "dmg_type": None,   # 1d4 + 4 temp HP
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 hour",
        "upcast": "+5 temp HP per slot above 1st",
        "srd": True,
        "effect": "gain 1d4 + 4 temporary hit points for the duration "
                  "(temp_hp field on R5Creature; camp-castable pre-fight)",
    },
    "fog cloud": {
        "name": "fog cloud",
        "level": 1, "school": "conjuration",
        "classes": ["druid", "ranger", "sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "utility",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        # heavy obscurement == mutual blindness inside the cloud
        "condition": "blinded (everyone, while inside the cloud)",
        "rounds": 600,
        "aoe": {"shape": "sphere", "size_ft": 20},
        "targets": "all", "range": 120, "duration": "1 hour",
        "upcast": "+20 ft radius per slot above 1st",
        "srd": True,
        "effect": "20-ft-radius sphere is heavily obscured: attacks into, "
                  "out of, or within it are blind-vs-unseen (adv+dis soup); "
                  "defensive screen the engine can flag-model",
    },
    "goodberry": {
        "name": "goodberry",
        "level": 1, "school": "transmutation",
        "classes": ["druid", "ranger"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "heal",
        "dice": None, "dmg_type": None,   # flat 1 HP per berry, 10 berries
        "save": None, "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "party", "range": 0, "duration": "instantaneous",
        "upcast": None,
        "srd": True,
        "effect": "10 berries; eating one (an action) restores 1 HP and "
                  "feeds a creature for a day; berries expire in 24 h -- "
                  "camp item: a slot becomes 10 HP of out-of-combat healing",
    },

    "grease": {
        "name": "grease",
        "level": 1, "school": "conjuration",
        "classes": ["wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("dex", "negates"), "attack": None,
        "condition": "prone", "rounds": 10,   # zone lasts 1 minute
        "aoe": {"shape": "cube", "size_ft": 10},   # RAW: 10-ft square
        "targets": "all", "range": 60, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "creatures in the area on cast (and any entering or "
                  "ending a turn there): DEX save or fall prone; area is "
                  "difficult terrain",
    },
    "guiding bolt": {
        "name": "guiding bolt",
        "level": 1, "school": "evocation",
        "classes": ["cleric"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "4d6", "dmg_type": "radiant",
        "save": None, "attack": "ranged_spell",
        "condition": None, "rounds": 1,   # the advantage beacon
        "aoe": None,
        "targets": "one", "range": 120, "duration": "1 round",
        "upcast": "+1d6 per slot above 1st",
        "srd": True,
        "effect": "ranged spell attack for 4d6 radiant; next attack roll "
                  "against the target before the end of your next turn has "
                  "advantage",   # matches existing SPELLS entry
    },
    "healing word": {
        "name": "healing word",
        "level": 1, "school": "evocation",
        "classes": ["bard", "cleric", "druid"],
        "cast": "bonus", "action": "bonus",
        "concentration": False, "ritual": False,
        "kind": "heal",
        "dice": "1d4", "plus_mod": True, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "instantaneous",
        "upcast": "+1d4 per slot above 1st",
        "srd": True,
        # SRD 5.2.1: school abjuration, 2d4 + mod; 5.1 baseline kept,
        # matching the existing srd_data.py entry.
        "effect": "one creature in range regains 1d4 + spellcasting mod HP; "
                  "no effect on undead or constructs; the wake-the-fallen "
                  "bonus action (r5_heal wakes unconscious PCs)",
    },
    "hellish rebuke": {
        "name": "hellish rebuke",
        "level": 1, "school": "evocation",
        "classes": ["warlock"],
        "cast": "reaction", "action": "reaction",
        "concentration": False, "ritual": False,
        "kind": "save",
        "dice": "2d10", "dmg_type": "fire",
        "save": ("dex", "half"), "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "instantaneous",
        "upcast": "+1d10 per slot above 1st",
        "srd": True,
        "effect": "reaction when damaged by a creature you can see: it "
                  "makes a DEX save or takes 2d10 fire, half on success "
                  "(pointless vs the fire-immune imp and cambion -- flavor)",
    },
    "heroism": {
        "name": "heroism",
        "level": 1, "school": "enchantment",
        "classes": ["bard", "paladin"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "plus_mod": True, "dmg_type": None,   # temp HP = mod
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 minute",
        # PHB/SRD 5.1 print text has "+1 creature per slot above 1st";
        # dnd5eapi omits the At Higher Levels block.  Recorded per print.
        "upcast": "+1 target per slot above 1st (dnd5eapi omits this)",
        "srd": True,
        "effect": "touched ally is immune to frightened and gains temp HP "
                  "equal to your spellcasting mod at the start of each of "
                  "its turns; temp HP vanish when the spell ends",
    },
    "hideous laughter": {
        "name": "hideous laughter",
        "level": 1, "school": "enchantment",
        "classes": ["bard", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("wis", "negates"), "attack": None,
        "condition": "incapacitated+prone", "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 30, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "one creature with INT > 4: WIS save or collapse laughing "
                  "-- prone, incapacitated, can't stand; re-saves at end of "
                  "each of its turns and when damaged (advantage if damage-"
                  "triggered)",
    },
    "hunter's mark": {
        "name": "hunter's mark",
        "level": 1, "school": "divination",
        "classes": ["ranger"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": "1d6", "dmg_type": None,   # weapon's type, per 5.1
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "one", "range": 90, "duration": "1 hour",
        "upcast": None,   # 1st->2nd buys nothing (duration jumps at 3rd)
        "srd": True,
        # SRD 5.2.1 makes the extra damage force and triggers on any attack
        # roll; 5.1 baseline (weapon hits, weapon's type) kept, matching
        # the existing srd_data.py entry.
        "effect": "mark one creature; your weapon attacks that hit it deal "
                  "+1d6 (R5F_MARK); bonus action to re-mark on kill",
    },
    "inflict wounds": {
        "name": "inflict wounds",
        "level": 1, "school": "necromancy",
        "classes": ["cleric"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "3d10", "dmg_type": "necrotic",
        "save": None, "attack": "melee_spell",
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "instantaneous",
        "upcast": "+1d10 per slot above 1st",
        "srd": True,
        "effect": "melee spell attack for 3d10 necrotic -- the biggest "
                  "level-1 single hit in the game",
    },
    "longstrider": {
        "name": "longstrider",
        "level": 1, "school": "transmutation",
        "classes": ["bard", "druid", "ranger", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 hour",
        "upcast": "+1 target per slot above 1st",
        "srd": True,
        # kept as a no-concentration camp buff (speed exists in the battle
        # layer); jump and feather fall are skipped -- rooms have neither
        # long jumps nor falls
        "effect": "touched creature's speed +10 ft for 1 hour",
    },
    "mage armor": {
        "name": "mage armor",
        "level": 1, "school": "abjuration",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 4800,   # 8 hours
        "aoe": None,
        "targets": "one", "range": 0, "duration": "8 hours",
        "upcast": None,
        "srd": True,
        "effect": "willing unarmored creature's base AC becomes 13 + DEX "
                  "mod for 8 hours; ends if it dons armor (the classic "
                  "cast-at-camp wizard skin)",
    },
    "magic missile": {
        "name": "magic missile",
        "level": 1, "school": "evocation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "multi",
        "dice": {"count": 3, "dice": "1d4", "plus": 1}, "dmg_type": "force",
        "save": None, "attack": None,   # auto-hit
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "all", "range": 120, "duration": "instantaneous",
        "upcast": "+1 dart per slot above 1st",
        "srd": True,
        "effect": "3 darts, each auto-hits for 1d4+1 force; darts strike "
                  "simultaneously and may split among visible targets",
    },   # matches existing SPELLS entry exactly
    "protection from evil and good": {
        "name": "protection from evil and good",
        "level": 1, "school": "abjuration",
        "classes": ["cleric", "paladin", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 100,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "10 minutes",
        "upcast": None,
        "srd": True,
        "effect": "touched willing creature: aberrations, celestials, "
                  "elementals, fey, fiends, and undead have disadvantage to "
                  "hit it, and it can't be charmed/frightened/possessed by "
                  "them -- on the nautiloid that is EVERY enemy (imp, "
                  "devourer, cambion, mind flayer)",
    },
    "sanctuary": {
        "name": "sanctuary",
        "level": 1, "school": "abjuration",
        "classes": ["cleric"],
        "cast": "bonus", "action": "bonus",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        # the ATTACKER saves, not the warded creature
        "save": ("wis", "negates"), "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 30, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "ward one creature: any enemy targeting it with an attack "
                  "or harmful spell first makes a WIS save or must pick a "
                  "new target / lose the action; ward breaks if the warded "
                  "creature attacks or casts against an enemy; no help vs "
                  "area effects",
    },
    "shield": {
        "name": "shield",
        "level": 1, "school": "abjuration",
        "classes": ["sorcerer", "wizard"],
        "cast": "reaction", "action": "reaction",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 1,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 round",
        "upcast": None,
        "srd": True,
        "effect": "reaction when hit by an attack or targeted by magic "
                  "missile: +5 AC until the start of your next turn, "
                  "including vs the trigger, and magic missile deals you "
                  "nothing",
    },
    "shield of faith": {
        "name": "shield of faith",
        "level": 1, "school": "abjuration",
        "classes": ["cleric", "paladin"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 100,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "10 minutes",
        "upcast": None,
        "srd": True,
        "effect": "one creature in range gains +2 AC for the duration",
    },   # matches existing SPELLS entry exactly
    "sleep": {
        "name": "sleep",
        "level": 1, "school": "enchantment",
        "classes": ["bard", "sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "pool",
        "dice": "5d8", "dmg_type": None,
        "save": None, "attack": None,
        "condition": "unconscious", "rounds": 10,
        "aoe": {"shape": "sphere", "size_ft": 20},
        "targets": "all", "range": 90, "duration": "1 minute",
        "upcast": "+2d8 to the pool per slot above 1st",
        "srd": True,
        # SRD 5.2.1 redesigned sleep entirely (5-ft sphere, WIS save gate,
        # concentration); the classic 5.1 HP-pool mechanic below is what
        # this game implements -- matches the existing srd_data.py entry.
        "effect": "roll 5d8 for an HP pool; creatures within 20 ft of the "
                  "point fall unconscious in ascending order of current HP, "
                  "no save, subtracting each sleeper's HP from the pool; "
                  "skip a creature the remaining pool cannot cover; ends on "
                  "damage or shake-awake (action); undead and charm-immune "
                  "creatures unaffected",
    },
    "thunderwave": {
        "name": "thunderwave",
        "level": 1, "school": "evocation",
        "classes": ["bard", "druid", "sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "save",
        "dice": "2d8", "dmg_type": "thunder",
        "save": ("con", "half"), "attack": None,
        "condition": None, "rounds": None,
        "aoe": {"shape": "cube", "size_ft": 15},
        "targets": "all", "range": 0, "duration": "instantaneous",
        "upcast": "+1d8 per slot above 1st",
        "srd": True,
        "effect": "15-ft cube from self; CON save or take 2d8 thunder and "
                  "be pushed 10 ft away (half damage, no push, on success); "
                  "audible 300 ft",   # matches existing SPELLS entry
    },
    # Not in SRD 5.1 at all; encoded from SRD 5.2.1 exactly as the game
    # already ships it (see srd_data.py, which carries the same note).  The
    # 2014 PHB version was instead cast before the hit and triggered on your
    # next weapon hit.
    "ensnaring strike": {
        "name": "ensnaring strike",
        "level": 1, "school": "conjuration",
        "classes": ["ranger"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": "1d6", "dmg_type": "piercing",
        "save": ("str", "negates"), "attack": None,
        "condition": "restrained", "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 minute",
        "upcast": "+1d6 per slot above 1st",
        "srd": True,   # SRD 5.2.1 (not 5.1)
        "effect": "on a weapon hit, target makes a STR save (advantage if "
                  "Large or bigger) or is restrained by vines and takes 1d6 "
                  "piercing at the start of each of its turns; escape = "
                  "action, STR (Athletics) vs spell DC",
    },

    # ----------------------------------------------------------- level 2 --
    # (2nd-level slots arrive at character level 3; no upcast is reachable,
    # so "upcast" is None throughout this section.)

    "acid arrow": {
        "name": "acid arrow",
        "level": 2, "school": "evocation",
        "classes": ["wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "attack",
        "dice": "4d4", "dmg_type": "acid",
        "save": None, "attack": "ranged_spell",
        "condition": None, "rounds": 1,   # the delayed splash
        "aoe": None,
        "targets": "one", "range": 90, "duration": "instantaneous",
        "upcast": None,
        "srd": True,
        # structured rider for the codegen: the end-of-next-turn tick
        "delayed": {"dice": "2d4", "dmg_type": "acid"},
        "effect": "ranged spell attack: 4d4 acid on hit plus 2d4 acid at "
                  "the end of the target's next turn; on a MISS still half "
                  "the initial damage and no delayed damage (unique "
                  "miss-for-half attack)",
    },
    "aid": {
        "name": "aid",
        "level": 2, "school": "abjuration",
        "classes": ["cleric", "paladin"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,   # flat +5, no roll
        "save": None, "attack": None,
        "condition": None, "rounds": 4800,   # 8 hours
        "aoe": None,
        "targets": "party", "range": 30, "duration": "8 hours",
        "upcast": None,
        "srd": True,
        "effect": "up to 3 creatures get +5 max HP AND +5 current HP for 8 "
                  "hours -- premier camp buff; not temp HP, real ceiling",
    },
    "barkskin": {
        "name": "barkskin",
        "level": 2, "school": "transmutation",
        "classes": ["druid", "ranger"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 hour",
        "upcast": None,
        "srd": True,
        # SRD 5.2.1: bonus action, no concentration, AC floor 17; the 5.1
        # action-cast concentration AC-16 floor is canonical here.
        "effect": "touched willing creature's AC can't be below 16 for the "
                  "duration (floor, not bonus -- big for wild-shape beasts "
                  "and unarmored druids)",
    },
    "blindness/deafness": {
        "name": "blindness/deafness",
        "level": 2, "school": "necromancy",
        "classes": ["bard", "cleric", "sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,   # no concentration!
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("con", "negates"), "attack": None,
        # caster picks blinded or deafened; blinded is the combat pick and
        # what the engine should default to
        "condition": "blinded", "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 30, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "CON save or blinded (or deafened, caster's choice) for 1 "
                  "minute; re-save at the end of each of its turns; the "
                  "no-concentration debuff",
    },
    "blur": {
        "name": "blur",
        "level": 2, "school": "illusion",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "attack rolls against you have disadvantage for the "
                  "duration (attackers that don't rely on sight ignore it)",
    },
    "branding smite": {
        "name": "branding smite",
        "level": 2, "school": "evocation",
        "classes": ["paladin"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": "2d6", "dmg_type": "radiant",
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "your next weapon hit before the spell ends deals +2d6 "
                  "radiant, the target becomes visible if invisible and "
                  "can't turn invisible while the spell lasts (smite-rider "
                  "pattern; the anti-imp smite)",
    },
    "calm emotions": {
        "name": "calm emotions",
        "level": 2, "school": "enchantment",
        "classes": ["bard", "cleric"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("cha", "negates"), "attack": None,   # allies may fail freely
        # custom condition: hostile targets stand down; on allies it instead
        # SUPPRESSES charmed/frightened for the duration
        "condition": "pacified (or: suppresses charmed/frightened on "
                     "allies)", "rounds": 10,
        "aoe": {"shape": "sphere", "size_ft": 20},
        "targets": "all", "range": 60, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "humanoids in the sphere: CHA save (may fail on purpose) "
                  "or either lose charmed/frightened for the duration (cast "
                  "on allies vs the cambion's Fiendish Charm) or turn "
                  "indifferent to chosen enemies until harmed",
    },
    "darkness": {
        "name": "darkness",
        "level": 2, "school": "evocation",
        "classes": ["sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "utility",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": "blinded (everyone inside without Devil's Sight)",
        "rounds": 100,
        "aoe": {"shape": "sphere", "size_ft": 15},
        "targets": "all", "range": 60, "duration": "10 minutes",
        "upcast": None,
        "srd": True,
        "effect": "15-ft-radius magical darkness: darkvision can't pierce "
                  "it, nonmagical light dies in it; mutual blindness soup "
                  "like fog cloud -- but note the imp's Devil's Sight sees "
                  "through it just fine",
    },
    "enhance ability": {
        "name": "enhance ability",
        "level": 2, "school": "transmutation",
        "classes": ["bard", "cleric", "druid", "sorcerer"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": "2d6", "dmg_type": None,   # Bear's Endurance temp HP
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 hour",
        "upcast": None,
        "srd": True,
        "effect": "touched creature gets advantage on one ability's checks "
                  "for 1 hour (choose animal: bear also grants 2d6 temp HP) "
                  "-- dialog-check and camp value",
    },
    "enlarge/reduce": {
        "name": "enlarge/reduce",
        "level": 2, "school": "transmutation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",   # buff on an ally, debuff (with save) on an enemy
        "dice": "1d4", "dmg_type": None,   # weapon damage swing, +/-
        "save": ("con", "negates"), "attack": None,   # unwilling targets only
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 30, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "enlarge: +1d4 weapon damage, advantage on STR checks and "
                  "saves, size up; reduce (enemy, CON save negates): -1d4 "
                  "weapon damage, disadvantage on STR checks and saves, "
                  "size down",
    },
    "flame blade": {
        "name": "flame blade",
        "level": 2, "school": "evocation",
        "classes": ["druid"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "attack",
        "dice": "3d6", "dmg_type": "fire",
        "save": None, "attack": "melee_spell",
        "condition": None, "rounds": 100,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "10 minutes",
        "upcast": None,   # damage first scales at 4th-level slots anyway
        "srd": True,
        "effect": "bonus action: fiery scimitar in your free hand for 10 "
                  "minutes; each ACTION you may melee spell attack with it "
                  "for 3d6 fire; sheds light; re-evoke as a bonus action if "
                  "dropped",
    },
    "flaming sphere": {
        "name": "flaming sphere",
        "level": 2, "school": "conjuration",
        "classes": ["druid", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "save",
        "dice": "2d6", "dmg_type": "fire",
        "save": ("dex", "half"), "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,   # a 5-ft rolling hazard, not a burst
        "targets": "one", "range": 60, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "summon a 5-ft fire sphere: creatures ending their turn "
                  "within 5 ft of it save DEX for half of 2d6 fire; bonus "
                  "action each turn to move it 30 ft and ram someone (a "
                  "pet damage engine on a bonus action)",
    },
    "heat metal": {
        "name": "heat metal",
        "level": 2, "school": "transmutation",
        "classes": ["bard", "druid"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "save",
        "dice": "2d8", "dmg_type": "fire",
        "save": ("con", "negates"), "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        # The 2d8 fire is AUTOMATIC -- no roll avoids it.  The CON save
        # (dnd5eapi dc_success: "other") gates only the drop-your-weapon /
        # disadvantage rider.  Codegen must not halve/negate the damage.
        "save_vs_damage": False,
        "effect": "target a metal weapon or armor: its holder/wearer takes "
                  "2d8 fire, NO save vs damage; bonus action each turn to "
                  "repeat; CON save or drop the object (can't = "
                  "disadvantage on attacks and checks until your next "
                  "turn); Commander Zhalk is a man in metal holding metal",
    },
    "hold person": {
        "name": "hold person",
        "level": 2, "school": "enchantment",
        "classes": ["bard", "cleric", "druid", "sorcerer", "warlock",
                    "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("wis", "negates"), "attack": None,
        "condition": "paralyzed", "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "one HUMANOID: WIS save or paralyzed; re-save at end of "
                  "each of its turns; paralyzed = auto-crit within 5 ft "
                  "(R5F_AUTOCRIT) -- works on thralls and cultists, not "
                  "fiends or aberrations",
    },
    "invisibility": {
        "name": "invisibility",
        "level": 2, "school": "illusion",
        "classes": ["bard", "sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": "invisible", "rounds": 600,   # C_INVISIBLE, on an ally
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 hour",
        "upcast": None,
        "srd": True,
        "effect": "touched creature is invisible until it attacks or casts "
                  "a spell: attacks against it have disadvantage, its "
                  "attacks have advantage (the imp's own trick, engine "
                  "condition already exists)",
    },
    "lesser restoration": {
        "name": "lesser restoration",
        "level": 2, "school": "abjuration",
        "classes": ["bard", "cleric", "druid", "paladin", "ranger"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "heal",   # a cleanse: no HP, removes a condition
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "instantaneous",
        "upcast": None,
        "srd": True,
        "effect": "end one disease or one of: blinded, deafened, paralyzed, "
                  "poisoned on the touched creature (all four are engine "
                  "conditions)",
    },
    "levitate": {
        "name": "levitate",
        "level": 2, "school": "transmutation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("con", "negates"), "attack": None,
        # custom condition: melee-lockout; nearest engine analog is
        # restrained-without-the-save-penalty (speed 0, can't approach)
        "condition": "levitated (floats 20 ft up: speed 0, melee-only "
                     "creatures can't reach or be reached)", "rounds": 100,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "10 minutes",
        "upcast": None,
        "srd": True,
        "effect": "one creature up to 500 lb: CON save (unwilling only) or "
                  "float 20 ft up for the duration -- a melee monster "
                  "becomes a pinata for archers; also castable on an ally "
                  "to escape melee",
    },

    "magic weapon": {
        "name": "magic weapon",
        "level": 2, "school": "transmutation",
        "classes": ["paladin", "wizard"],
        "cast": "bonus", "action": "bonus",
        "concentration": True, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 hour",
        "upcast": None,   # +2 needs a 4th-level slot, out of reach
        "srd": True,
        "effect": "touched nonmagical weapon becomes magical with +1 to "
                  "attack and damage rolls for 1 hour (beats the imp's "
                  "resistance to nonmagical, nonsilvered weapons)",
    },
    "mirror image": {
        "name": "mirror image",
        "level": 2, "school": "illusion",
        "classes": ["sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,   # no concentration!
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "3 duplicates; each incoming attack rolls a d20: 6+ / 8+ "
                  "/ 11+ (3/2/1 dupes left) redirects it to a duplicate "
                  "with AC 10 + your DEX mod, destroying it on a hit -- "
                  "sight-based attackers only",
    },
    "misty step": {
        "name": "misty step",
        "level": 2, "school": "conjuration",
        "classes": ["sorcerer", "warlock", "wizard"],
        "cast": "bonus", "action": "bonus",
        "concentration": False, "ritual": False,
        "kind": "utility",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "instantaneous",
        "upcast": None,
        "srd": True,
        # kept on battle-layer faith: positioning lives there (rules.h);
        # in menu terms this is "escape melee / cross the room free"
        "effect": "bonus action: teleport up to 30 ft to a spot you can "
                  "see; escapes grapples, restraints-by-position, and "
                  "melee lockdown without provoking",
    },
    "moonbeam": {
        "name": "moonbeam",
        "level": 2, "school": "evocation",
        "classes": ["druid"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "save",
        "dice": "2d10", "dmg_type": "radiant",
        "save": ("con", "half"), "attack": None,
        "condition": None, "rounds": 10,
        # true shape: 5-ft-radius, 40-ft-high CYLINDER (schema allows only
        # line/cone/sphere/cube; sphere of the same radius recorded)
        "aoe": {"shape": "sphere", "size_ft": 5},
        "targets": "all", "range": 120, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "5-ft-radius beam: a creature entering it or starting "
                  "its turn there saves CON for half of 2d10 radiant; "
                  "shapechangers save at disadvantage and revert on a fail "
                  "(the anti-imp zone); action each turn to move the beam "
                  "60 ft",
    },
    "prayer of healing": {
        "name": "prayer of healing",
        "level": 2, "school": "evocation",
        "classes": ["cleric"],
        # RAW casting time is 10 MINUTES -- camp/downtime only, never
        # mid-fight; "action" is the closest schema value
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "heal",
        "dice": "2d8", "plus_mod": True, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "party", "range": 30, "duration": "instantaneous",
        "upcast": None,
        "srd": True,
        # SRD 5.2.1 additionally grants each target a short rest's benefits.
        "effect": "10-minute cast (CAMP ONLY): up to six creatures each "
                  "regain 2d8 + spellcasting mod HP; no effect on undead "
                  "or constructs -- one slot, whole party topped up",
    },
    "protection from poison": {
        "name": "protection from poison",
        "level": 2, "school": "abjuration",
        "classes": ["cleric", "druid", "paladin", "ranger"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 hour",
        "upcast": None,
        "srd": True,
        "effect": "touched creature: neutralize one poison afflicting it; "
                  "for 1 hour it has resistance to poison damage and "
                  "advantage on saves vs poisoned (the imp's sting loses "
                  "its teeth)",
    },
    "ray of enfeeblement": {
        "name": "ray of enfeeblement",
        "level": 2, "school": "necromancy",
        "classes": ["warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        # spell attack to land it, then CON re-saves to shake it
        "save": ("con", "negates"), "attack": "ranged_spell",
        "condition": "enfeebled (STR-based weapon damage halved)",
        "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "1 minute",
        "upcast": None,
        "srd": True,
        "effect": "ranged spell attack: on hit the target deals half "
                  "damage with STR weapon attacks; CON save at the end of "
                  "each of ITS turns ends it (halves Zhalk's greatsword)",
    },
    "scorching ray": {
        "name": "scorching ray",
        "level": 2, "school": "evocation",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "multi",
        "dice": {"count": 3, "dice": "2d6", "plus": 0}, "dmg_type": "fire",
        "save": None, "attack": "ranged_spell",   # separate roll per ray
        "condition": None, "rounds": None,
        "aoe": None,
        "targets": "all", "range": 120, "duration": "instantaneous",
        "upcast": None,
        "srd": True,
        "effect": "3 rays, a separate ranged spell attack each for 2d6 "
                  "fire, split among targets freely (magic missile's "
                  "swingier big sibling)",
    },
    "see invisibility": {
        "name": "see invisibility",
        "level": 2, "school": "divination",
        "classes": ["bard", "sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "self", "range": 0, "duration": "1 hour",
        "upcast": None,
        "srd": True,
        "effect": "for 1 hour you see invisible creatures and objects "
                  "(and into the Ethereal): the imp's Invisibility action "
                  "stops working on you -- kept precisely because the "
                  "prologue's signature enemy vanishes",
    },
    "shatter": {
        "name": "shatter",
        "level": 2, "school": "evocation",
        "classes": ["bard", "sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "save",
        "dice": "3d8", "dmg_type": "thunder",
        "save": ("con", "half"), "attack": None,
        "condition": None, "rounds": None,
        "aoe": {"shape": "sphere", "size_ft": 10},
        "targets": "all", "range": 60, "duration": "instantaneous",
        "upcast": None,
        "srd": True,
        "effect": "10-ft-radius boom at a point in range: CON save for "
                  "half of 3d8 thunder; inorganic creatures save at "
                  "disadvantage; the tier's fireball-before-fireball",
    },
    "silence": {
        "name": "silence",
        "level": 2, "school": "illusion",
        "classes": ["bard", "cleric", "ranger"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": True,
        "kind": "utility",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": "deafened (while fully inside; also thunder-immune)",
        "rounds": 100,
        "aoe": {"shape": "sphere", "size_ft": 20},
        "targets": "all", "range": 120, "duration": "10 minutes",
        "upcast": None,
        "srd": True,
        "effect": "20-ft-radius soundless zone: creatures fully inside are "
                  "deafened and immune to thunder damage; verbal spell-"
                  "casting is impossible there (mind flayer psionics need "
                  "no components -- it will not care; cultist casters "
                  "will)",
    },
    "spike growth": {
        "name": "spike growth",
        "level": 2, "school": "transmutation",
        "classes": ["druid", "ranger"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "utility",   # zone damage: no attack roll, no save
        "dice": "2d4", "dmg_type": "piercing",
        "save": None, "attack": None,
        "condition": None, "rounds": 100,
        # true shape: 20-ft-radius CYLINDER (ground area); sphere recorded
        "aoe": {"shape": "sphere", "size_ft": 20},
        "targets": "all", "range": 150, "duration": "10 minutes",
        "upcast": None,
        "srd": True,
        "effect": "20-ft-radius ground sprouts spikes, camouflaged and "
                  "difficult terrain: 2d4 piercing per 5 ft a creature "
                  "moves through it (engine: charge damage tiers when an "
                  "enemy closes through the zone)",
    },
    "spiritual weapon": {
        "name": "spiritual weapon",
        "level": 2, "school": "evocation",
        "classes": ["cleric"],
        "cast": "bonus", "action": "bonus",
        "concentration": False, "ritual": False,   # no concentration!
        "kind": "attack",
        "dice": "1d8", "plus_mod": True, "dmg_type": "force",
        "save": None, "attack": "melee_spell",
        "condition": None, "rounds": 10,
        "aoe": None,
        "targets": "one", "range": 60, "duration": "1 minute",
        "upcast": None,   # +1d8 needs a 4th-level slot (every TWO levels)
        "srd": True,
        "effect": "bonus action: spectral weapon appears and swings -- "
                  "melee spell attack for 1d8 + spellcasting mod force; "
                  "every later turn, bonus action to move it 20 ft and "
                  "swing again; no concentration = the cleric's whole-"
                  "fight bonus-action engine",
    },
    "suggestion": {
        "name": "suggestion",
        "level": 2, "school": "enchantment",
        "classes": ["bard", "sorcerer", "warlock", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("wis", "negates"), "attack": None,
        # RAW this is compulsion, not the charmed condition; charm-immunity
        # gates it, and "out of the fight" is engine-charmed in practice
        "condition": "charmed", "rounds": 4800,   # up to 8 hours
        "aoe": None,
        "targets": "one", "range": 30, "duration": "8 hours",
        "upcast": None,
        "srd": True,
        "effect": "suggest a reasonable-sounding course of action (a "
                  "sentence or two) to a creature that hears and "
                  "understands you: WIS save or it pursues the suggestion; "
                  "charm-immune creatures unaffected; ends if you or "
                  "companions damage it ('walk away' = one enemy exits "
                  "the encounter)",
    },
    "warding bond": {
        "name": "warding bond",
        "level": 2, "school": "abjuration",
        "classes": ["cleric"],
        "cast": "action", "action": "action",
        "concentration": False, "ritual": False,
        "kind": "buff",
        "dice": None, "dmg_type": None,
        "save": None, "attack": None,
        "condition": None, "rounds": 600,
        "aoe": None,
        "targets": "one", "range": 0, "duration": "1 hour",
        "upcast": None,
        "srd": True,
        "effect": "bond with a touched ally for 1 hour: it gains +1 AC, "
                  "+1 to saves, and resistance to ALL damage -- and the "
                  "caster takes matching damage each time it is hurt; "
                  "ends if the caster drops to 0 HP (tank-links the "
                  "cleric to the squishy)",
    },
    "web": {
        "name": "web",
        "level": 2, "school": "conjuration",
        "classes": ["sorcerer", "wizard"],
        "cast": "action", "action": "action",
        "concentration": True, "ritual": False,
        "kind": "condition",
        "dice": None, "dmg_type": None,
        "save": ("dex", "negates"), "attack": None,
        "condition": "restrained", "rounds": 600,
        "aoe": {"shape": "cube", "size_ft": 20},
        "targets": "all", "range": 60, "duration": "1 hour",
        "upcast": None,
        "srd": True,
        "effect": "20-ft cube of webs (needs anchoring surfaces): a "
                  "creature starting its turn in or entering them saves "
                  "DEX or is restrained; escape = action, STR check vs "
                  "spell DC; webs are difficult terrain and flammable "
                  "(fire burns a 5-ft cube per round for 2d4 fire)",
    },
}

