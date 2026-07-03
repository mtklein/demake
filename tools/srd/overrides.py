# Prologue monster variants — OUR game-design numbers, not SRD and not MM.
#
# Two reasons an entry lives here instead of srd_data.py:
#  1. Licensing: intellect devourer, cambion, and mind flayer appear in no
#     SRD (see ATTRIBUTION.md). The stat blocks below are original homebrew
#     tuned for this game, in the spirit of BG3's own "Lesser" variants.
#  2. Balance: SRD monsters at book strength would end a level-1 party
#     (three 10 HP imps with +5 stings and 3d6 poison is a TPK, and MM
#     cambions have AC 19). BG3 solved this with "Lesser" variants; so do we.
#
# Schema matches srd_data.MONSTERS entries.

MONSTERS = {
    "lesser imp": {
        "display": "Imp", "srd": False, "ac": 11, "hp": 6,
        "abilities": {"str": 6, "dex": 14, "con": 11, "int": 9, "wis": 10, "cha": 10},
        "cr": 0.25,
        "attacks": [
            {"name": "Sting", "to_hit": 4, "dice": "1d4", "plus": 2,
             "dmg_type": "piercing",
             "extra": {"save": ("con", 10), "dice": "1d6",
                       "dmg_type": "poison", "half_on_save": True}},
        ],
        "resistances": ["cold"], "immunities": ["fire", "poison"],
    },
    "lesser hellsboar": {
        "display": "Boar", "srd": False, "ac": 11, "hp": 9,
        "abilities": {"str": 13, "dex": 10, "con": 12, "int": 2, "wis": 9, "cha": 5},
        "cr": 0.25,
        "attacks": [
            {"name": "Gore", "to_hit": 3, "dice": "1d6", "plus": 1,
             "dmg_type": "slashing"},
        ],
        "resistances": ["poison"], "immunities": [],
    },
    "intellect devourer": {
        "display": "Devourer", "srd": False, "ac": 12, "hp": 21,
        "abilities": {"str": 6, "dex": 14, "con": 13, "int": 12, "wis": 11, "cha": 10},
        "cr": 1,
        "attacks": [
            {"name": "Claws", "to_hit": 4, "dice": "1d6", "plus": 2,
             "dmg_type": "slashing"},
            {"name": "Jolt", "ranged": True, "to_hit": 4, "dice": "1d8", "plus": 2,
             "dmg_type": "psychic"},
        ],
        "resistances": [], "immunities": [],
    },
    "lesser cambion": {
        "display": "Cambion", "srd": False, "ac": 15, "hp": 40,
        "abilities": {"str": 16, "dex": 14, "con": 14, "int": 11, "wis": 10, "cha": 12},
        "cr": 2,
        "attacks": [
            {"name": "Trident", "to_hit": 5, "dice": "1d8", "plus": 3,
             "dmg_type": "piercing",
             "extra": {"save": ("dex", 11), "dice": "1d6",
                       "dmg_type": "fire", "half_on_save": True}},
            {"name": "Fire Ray", "ranged": True, "to_hit": 5, "dice": "2d6", "plus": 0,
             "dmg_type": "fire"},
        ],
        "resistances": ["cold", "fire", "poison"], "immunities": [],
    },
    "mind flayer ally": {
        "display": "Flayer", "srd": False, "ac": 15, "hp": 85,
        "abilities": {"str": 11, "dex": 12, "con": 12, "int": 19, "wis": 17, "cha": 17},
        "cr": 5,
        "attacks": [
            {"name": "Lash", "to_hit": 7, "dice": "2d10", "plus": 4,
             "dmg_type": "psychic"},
            {"name": "Mind Blast", "ranged": True, "to_hit": 7, "dice": "3d8", "plus": 0,
             "dmg_type": "psychic"},
        ],
        "resistances": [], "immunities": [],
    },
    "awakened thrall": {
        "display": "Thrall", "srd": False, "ac": 10, "hp": 9,
        "abilities": {"str": 12, "dex": 10, "con": 11, "int": 4, "wis": 6, "cha": 3},
        "cr": 0.25,
        "attacks": [
            {"name": "Rend", "to_hit": 3, "dice": "1d6", "plus": 1,
             "dmg_type": "bludgeoning"},
        ],
        "resistances": ["psychic"], "immunities": [],
    },
    "commander zhalk": {
        "display": "Zhalk", "srd": False, "ac": 16, "hp": 150,
        "abilities": {"str": 18, "dex": 14, "con": 16, "int": 12, "wis": 11, "cha": 14},
        "cr": 5,
        "attacks": [
            {"name": "Everburn", "to_hit": 7, "dice": "2d6", "plus": 4,
             "dmg_type": "slashing",
             "extra": {"save": ("dex", 12), "dice": "1d4",
                       "dmg_type": "fire", "half_on_save": True}},
            {"name": "Fire Ray", "ranged": True, "to_hit": 6, "dice": "3d6", "plus": 0,
             "dmg_type": "fire"},
        ],
        "resistances": ["cold", "fire", "poison"], "immunities": [],
    },
}

# Homebrew loot weapons (schema mirrors srd_data.WEAPONS + rider fields).
WEAPONS = {
    "everburn blade": {
        "display": "Everburn Blade",
        "dice": "2d6", "dmg_type": "slashing",
        "properties": ["heavy", "two-handed"], "melee": True,
        "rider": {"dice": "1d4", "dmg_type": "fire", "save": ("dex", 12)},
    },
    "imp stinger": {
        "display": "Imp Stinger",
        "dice": "1d4", "dmg_type": "piercing",
        "properties": ["finesse", "light"], "melee": True,
        "rider": {"dice": "1d4", "dmg_type": "poison", "save": ("con", 10)},
    },
}
