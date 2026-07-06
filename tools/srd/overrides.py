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
        "senses": {"darkvision": 120, "passive_perception": 11},   # devil's sight
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
        "senses": {"passive_perception": 9},
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
        "senses": {"blindsight": 60, "passive_perception": 12},
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
        "senses": {"darkvision": 60, "passive_perception": 12},
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
        "senses": {"darkvision": 120, "passive_perception": 16},
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
        "senses": {"passive_perception": 8},   # human eyes: the dark bites
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
        "senses": {"darkvision": 60, "passive_perception": 13},
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

# --------------------------------------------------------------------------
# SUBCLASSES_HB
#
# 24 original-text homebrew subclasses, two per class (piscodemon doctrine;
# stage_homebrew.py history carries the adjacency/licensing report).
# --------------------------------------------------------------------------

SUBCLASSES_HB = {'path of the spirit beast': {'class': 'barbarian',
                              'level': 3,
                              'display': 'Spirit',
                              'blurb': 'An ancestral animal walks in your '
                                       'skin when the red mist rises.',
                              'srd': False,
                              'features': {3: [{'name': 'Beast Within',
                                                'srd': False,
                                                'mech': {'kind': 'choice',
                                                         'when': 'level-up',
                                                         'options': [{'label': 'Spirit '
                                                                               'of '
                                                                               'the '
                                                                               'Cave',
                                                                      'kind': 'passive_bonus',
                                                                      'what': 'resistance',
                                                                      'value': 'all '
                                                                               'damage '
                                                                               'types '
                                                                               'except '
                                                                               'psychic',
                                                                      'while': 'raging'},
                                                                     {'label': 'Spirit '
                                                                               'of '
                                                                               'the '
                                                                               'Pack',
                                                                      'kind': 'aura',
                                                                      'range': 'adjacent',
                                                                      'effect': 'allies '
                                                                                'have '
                                                                                'advantage '
                                                                                'on '
                                                                                'melee '
                                                                                'attacks '
                                                                                'against '
                                                                                'enemies '
                                                                                'adjacent '
                                                                                'to '
                                                                                'you',
                                                                      'while': 'raging'},
                                                                     {'label': 'Spirit '
                                                                               'of '
                                                                               'the '
                                                                               'Updraft',
                                                                      'kind': 'advantage',
                                                                      'on': 'DEX '
                                                                            'saving '
                                                                            'throws',
                                                                      'while': 'raging'}]}},
                                               {'name': 'Whisper of Beasts',
                                                'srd': False,
                                                'mech': {'kind': 'flavor',
                                                         'text': 'wild '
                                                                 'animals '
                                                                 'read you '
                                                                 'as kin; '
                                                                 'you can '
                                                                 'calm, '
                                                                 'warn, or '
                                                                 'question '
                                                                 'them in '
                                                                 'rough '
                                                                 'images'}}]}},
 'path of roiling chaos': {'class': 'barbarian',
                           'level': 3,
                           'display': 'Chaos',
                           'blurb': 'Your rage taps something that was never '
                                    'housebroken.',
                           'srd': False,
                           'features': {3: [{'name': 'Chaos Boil',
                                             'srd': False,
                                             'mech': {'kind': 'random',
                                                      'die': 4,
                                                      'when': 'you enter a '
                                                              'rage',
                                                      'options': [{'label': 'Flare',
                                                                   'kind': 'rider',
                                                                   'dice': '1d6',
                                                                   'dmg_type': 'force',
                                                                   'trigger': 'your '
                                                                              'weapon '
                                                                              'hits, '
                                                                              'for '
                                                                              'this '
                                                                              'rage'},
                                                                  {'label': 'Wardhide',
                                                                   'kind': 'temp_hp',
                                                                   'amount': '2 '
                                                                             '+ '
                                                                             'level',
                                                                   'target': 'you'},
                                                                  {'label': 'Overspill',
                                                                   'kind': 'heal_bonus',
                                                                   'amount': '1d6',
                                                                   'target': 'one '
                                                                             'ally '
                                                                             'you '
                                                                             'can '
                                                                             'see'},
                                                                  {'label': 'Static '
                                                                            'Skin',
                                                                   'kind': 'reaction',
                                                                   'trigger': 'the '
                                                                              'first '
                                                                              'melee '
                                                                              'hit '
                                                                              'against '
                                                                              'you '
                                                                              'this '
                                                                              'rage',
                                                                   'effect': 'the '
                                                                             'attacker '
                                                                             'takes '
                                                                             '1d6 '
                                                                             'lightning'}]}},
                                            {'name': 'Aftertaste of Magic',
                                             'srd': False,
                                             'mech': {'kind': 'flavor',
                                                      'text': 'you taste '
                                                              'nearby '
                                                              'enchantment '
                                                              'like a storm '
                                                              'coming; '
                                                              'curses taste '
                                                              'of '
                                                              'copper'}}]}},
 'college of banners': {'class': 'bard',
                        'level': 3,
                        'display': 'Banners',
                        'blurb': "A war-poet's verse is armor you can march "
                                 'behind.',
                        'srd': False,
                        'features': {3: [{'name': 'Armored Cadence',
                                          'srd': False,
                                          'mech': {'kind': 'passive_bonus',
                                                   'what': 'proficiencies',
                                                   'value': ['medium armor',
                                                             'shields',
                                                             'martial '
                                                             'weapons']}},
                                         {'name': 'Rallying Verse',
                                          'srd': False,
                                          'mech': {'kind': 'rider',
                                                   'dice': '1d6',
                                                   'trigger': 'an ally '
                                                              'holding your '
                                                              'inspiration '
                                                              'die may spend '
                                                              'it on a '
                                                              'damage roll '
                                                              'instead of a '
                                                              'd20 roll'}},
                                         {'name': 'Counter-Chorus',
                                          'srd': False,
                                          'mech': {'kind': 'reaction',
                                                   'trigger': 'the holder of '
                                                              'your '
                                                              'inspiration '
                                                              'die is hit by '
                                                              'an attack',
                                                   'effect': 'spend the die: '
                                                             'add its roll '
                                                             'to AC against '
                                                             'that attack, '
                                                             'possibly '
                                                             'turning the '
                                                             'hit into a '
                                                             'miss'}}]}},
 'college of blades': {'class': 'bard',
                       'level': 3,
                       'display': 'Blades',
                       'blurb': 'Every duel is a performance; every scar, a '
                                'review.',
                       'srd': False,
                       'features': {3: [{'name': 'Cut and Caper',
                                         'srd': False,
                                         'mech': {'kind': 'rider',
                                                  'dice': '1d6',
                                                  'trigger': 'your own '
                                                             'weapon hit',
                                                  'cost': 'one Bardic '
                                                          'Inspiration use',
                                                  'also': {'kind': 'passive_bonus',
                                                           'what': 'ac',
                                                           'amount': 'the '
                                                                     'same '
                                                                     'roll',
                                                           'duration': 'until '
                                                                       'your '
                                                                       'next '
                                                                       'turn'},
                                                  'note': 'one roll, spent '
                                                          'twice: damage '
                                                          'now, footwork '
                                                          'after'}},
                                        {'name': "Fencer's Footing",
                                         'srd': False,
                                         'mech': {'kind': 'passive_bonus',
                                                  'what': 'damage',
                                                  'amount': 2,
                                                  'when': 'wielding a '
                                                          'finesse weapon in '
                                                          'one hand and '
                                                          'nothing in the '
                                                          'other'}},
                                        {'name': 'Show-Fighter',
                                         'srd': False,
                                         'mech': {'kind': 'flavor',
                                                  'text': 'any audience that '
                                                          'watches you fight '
                                                          'remembers your '
                                                          'name and one '
                                                          'flattering detail '
                                                          'of your '
                                                          'choosing'}}]}},
 'domain of the vanguard': {'class': 'cleric',
                            'level': 1,
                            'display': 'Vanguard',
                            'blurb': 'Faith that marches in the first rank '
                                     'and swings first.',
                            'srd': False,
                            'features': {1: [{'name': "Vanguard's Arms",
                                              'srd': False,
                                              'mech': {'kind': 'passive_bonus',
                                                       'what': 'proficiencies',
                                                       'value': ['martial '
                                                                 'weapons',
                                                                 'heavy '
                                                                 'armor']}},
                                             {'name': 'Pressing Attack',
                                              'srd': False,
                                              'mech': {'kind': 'resource',
                                                       'name': 'pressing '
                                                               'attacks',
                                                       'pool': 'wis_mod (min '
                                                               '1)',
                                                       'refresh': 'long',
                                                       'spend': 'after you '
                                                                'take the '
                                                                'Attack '
                                                                'action, '
                                                                'make one '
                                                                'extra '
                                                                'weapon '
                                                                'attack as a '
                                                                'bonus '
                                                                'action'}},
                                             {'name': 'Vanguard Writ',
                                              'srd': False,
                                              'mech': {'kind': 'always_prepared',
                                                       'spells': ['bless',
                                                                  'shield of '
                                                                  'faith']}}],
                                         2: [{'name': 'Rite of the Honed '
                                                      'Blade',
                                              'srd': False,
                                              'mech': {'kind': 'rider',
                                                       'dice': '1d8',
                                                       'dmg_type': 'radiant',
                                                       'trigger': 'your next '
                                                                  'weapon '
                                                                  'hit this '
                                                                  'turn',
                                                       'cost': 'Channel '
                                                               'Divinity',
                                                       'adv': True,
                                                       'note': 'the rite '
                                                               'also grants '
                                                               'advantage on '
                                                               'that attack '
                                                               'roll'}}],
                                         3: [{'name': 'Vanguard Writ II',
                                              'srd': False,
                                              'mech': {'kind': 'always_prepared',
                                                       'spells': ['magic '
                                                                  'weapon',
                                                                  'aid']}}]}},
 'domain of masks': {'class': 'cleric',
                     'level': 1,
                     'display': 'Masks',
                     'blurb': 'The sly god teaches that a good lie is a kind '
                              'of shelter.',
                     'srd': False,
                     'features': {1: [{'name': 'Blessing of the Sly',
                                       'srd': False,
                                       'mech': {'kind': 'advantage',
                                                'on': 'Stealth checks',
                                                'target': 'you or one ally '
                                                          '(touch)',
                                                'duration': 'until your next '
                                                            'long rest'}},
                                      {'name': 'Masked Writ',
                                       'srd': False,
                                       'mech': {'kind': 'always_prepared',
                                                'spells': ['charm person',
                                                           'silent image']}}],
                                  2: [{'name': 'Rite of the Mirrored Self',
                                       'srd': False,
                                       'mech': {'kind': 'impose_dis',
                                                'on': 'attack rolls against '
                                                      'you until your next '
                                                      'turn',
                                                'cost': 'Channel Divinity',
                                                'also': {'kind': 'advantage',
                                                         'on': 'your attack '
                                                               'rolls until '
                                                               'your next '
                                                               'turn'},
                                                'note': 'a phantom twin '
                                                        'flickers half a '
                                                        'step out of true '
                                                        'with you'}}],
                                  3: [{'name': 'Masked Writ II',
                                       'srd': False,
                                       'mech': {'kind': 'always_prepared',
                                                'spells': ['blur',
                                                           'pass without '
                                                           'trace']}}]}},
 'circle of the moonfang': {'class': 'druid',
                            'level': 2,
                            'display': 'Moonfang',
                            'blurb': 'Druids who wear the beast not as a '
                                     'cloak but as a drawn blade.',
                            'srd': False,
                            'features': {2: [{'name': 'Battle Shape',
                                              'srd': False,
                                              'mech': {'kind': 'passive_bonus',
                                                       'what': 'wild shape '
                                                               'action '
                                                               'economy',
                                                       'value': 'shape as a '
                                                                'bonus '
                                                                'action '
                                                                'instead of '
                                                                'an action'}},
                                             {'name': "Predator's Form",
                                              'srd': False,
                                              'mech': {'kind': 'passive_bonus',
                                                       'what': 'wild shape '
                                                               'form list',
                                                       'value': ['boar',
                                                                 'wolf',
                                                                 'black '
                                                                 'bear'],
                                                       'note': 'combat stat '
                                                               'blocks via '
                                                               'the monster '
                                                               'pipeline; '
                                                               'boar is '
                                                               'already '
                                                               'in-engine '
                                                               '(SRD, '
                                                               'R5M_BOAR), '
                                                               'wolf and '
                                                               'black bear '
                                                               'stage as SRD '
                                                               'beasts'}},
                                             {'name': 'Savage Mending',
                                              'srd': False,
                                              'mech': {'kind': 'heal_bonus',
                                                       'amount': '1d8 per '
                                                                 'level of '
                                                                 'the slot',
                                                       'cost': 'one spell '
                                                               'slot (bonus '
                                                               'action)',
                                                       'when': 'while '
                                                               'wild-shaped'}}]}},
 'circle of rot and bloom': {'class': 'druid',
                             'level': 2,
                             'display': 'Rotbloom',
                             'blurb': 'Decay is just the garden setting the '
                                      'table again.',
                             'srd': False,
                             'features': {2: [{'name': 'Grave Bloom',
                                               'srd': False,
                                               'mech': {'kind': 'temp_hp',
                                                        'amount': '3*level',
                                                        'cost': 'one wild '
                                                                'shape use '
                                                                '(action)',
                                                        'duration': 'until '
                                                                    'depleted '
                                                                    'or the '
                                                                    'battle '
                                                                    'ends'}},
                                              {'name': 'Rot Touch',
                                               'srd': False,
                                               'mech': {'kind': 'rider',
                                                        'dice': '1d6',
                                                        'dmg_type': 'necrotic',
                                                        'trigger': 'your '
                                                                   'weapon '
                                                                   'hits '
                                                                   'while '
                                                                   'any '
                                                                   'Grave '
                                                                   'Bloom '
                                                                   'temp HP '
                                                                   'remains'}},
                                              {'name': 'Spore Spite',
                                               'srd': False,
                                               'mech': {'kind': 'reaction',
                                                        'trigger': 'a '
                                                                   'creature '
                                                                   'hits you '
                                                                   'with a '
                                                                   'melee '
                                                                   'attack',
                                                        'dice': '1d4',
                                                        'dmg_type': 'necrotic',
                                                        'save': ('con',
                                                                 '8+prof+wis'),
                                                        'on_save': 'no '
                                                                   'damage'}}],
                                          3: [{'name': 'Gifts of the Mold',
                                               'srd': False,
                                               'mech': {'kind': 'always_prepared',
                                                        'spells': ['chill '
                                                                   'touch',
                                                                   'blindness/deafness']}}]}},
 'the tactician': {'class': 'fighter',
                   'level': 3,
                   'display': 'Tactician',
                   'blurb': 'Every swing is a lesson; every fight, a '
                            'chalkboard.',
                   'srd': False,
                   'features': {3: [{'name': 'Tactic Dice',
                                     'srd': False,
                                     'mech': {'kind': 'resource',
                                              'name': 'tactic dice',
                                              'pool': 3,
                                              'die': '1d8',
                                              'refresh': 'short'}},
                                    {'name': 'Turning Strikes',
                                     'srd': False,
                                     'mech': {'kind': 'choice',
                                              'when': 'per-use',
                                              'cost': 'one tactic die',
                                              'options': [{'label': 'Toppling '
                                                                    'Strike',
                                                           'kind': 'rider',
                                                           'dice': '1d8',
                                                           'dmg_type': 'same '
                                                                       'as '
                                                                       'the '
                                                                       'weapon',
                                                           'trigger': 'your '
                                                                      'weapon '
                                                                      'hit',
                                                           'save': ('str',
                                                                    '8+prof+str'),
                                                           'condition': 'prone',
                                                           'on_save': 'extra '
                                                                      'damage '
                                                                      'only'},
                                                          {'label': 'Rattling '
                                                                    'Blow',
                                                           'kind': 'rider',
                                                           'dice': '1d8',
                                                           'dmg_type': 'same '
                                                                       'as '
                                                                       'the '
                                                                       'weapon',
                                                           'trigger': 'your '
                                                                      'weapon '
                                                                      'hit',
                                                           'save': ('wis',
                                                                    '8+prof+str'),
                                                           'condition': 'frightened '
                                                                        'until '
                                                                        'the '
                                                                        'end '
                                                                        'of '
                                                                        'its '
                                                                        'next '
                                                                        'turn',
                                                           'on_save': 'extra '
                                                                      'damage '
                                                                      'only'}]}},
                                    {'name': 'Deft Deflection',
                                     'srd': False,
                                     'mech': {'kind': 'reaction',
                                              'trigger': 'you are hit by a '
                                                         'melee attack',
                                              'effect': 'reduce the damage '
                                                        'by 1d8 + DEX mod',
                                              'cost': 'one tactic die'}}]}},
 'the sigil knight': {'class': 'fighter',
                      'level': 3,
                      'display': 'Sigil',
                      'blurb': 'A soldier who scratches spellwork into sword '
                               'steel.',
                      'srd': False,
                      'features': {3: [{'name': 'Cantrip Spark',
                                        'srd': False,
                                        'mech': {'kind': 'choice',
                                                 'when': 'level-up',
                                                 'options': [{'label': 'fire '
                                                                       'bolt',
                                                              'kind': 'always_prepared',
                                                              'spells': ['fire '
                                                                         'bolt']},
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
                                                                         'grasp']}]}},
                                       {'name': 'Glyph Guard',
                                        'srd': False,
                                        'mech': {'kind': 'reaction',
                                                 'trigger': 'you are hit by '
                                                            'an attack',
                                                 'effect': '+3 AC against '
                                                           'that attack '
                                                           'only, possibly '
                                                           'turning the hit '
                                                           'into a miss',
                                                 'uses': 'prof',
                                                 'refresh': 'long'}},
                                       {'name': 'Bonded Blade',
                                        'srd': False,
                                        'mech': {'kind': 'flavor',
                                                 'text': 'your etched weapon '
                                                         'cannot be taken '
                                                         'from you; drop it '
                                                         'anywhere and it is '
                                                         'back in your hand '
                                                         'by morning'}}]}},
 'way of the long night': {'class': 'monk',
                           'level': 3,
                           'display': 'Long Night',
                           'blurb': 'Monks trained where the light forgets '
                                    'to look.',
                           'srd': False,
                           'features': {3: [{'name': 'Veilstep',
                                             'srd': False,
                                             'mech': {'kind': 'impose_dis',
                                                      'on': 'attack rolls '
                                                            'against you',
                                                      'condition': 'invisible',
                                                      'cost': '1 ki (bonus '
                                                              'action)',
                                                      'duration': 'until you '
                                                                  'attack or '
                                                                  'your next '
                                                                  'turn ends',
                                                      'note': 'engine '
                                                              'C_INVISIBLE: '
                                                              'attacks '
                                                              'against you '
                                                              'at '
                                                              'disadvantage, '
                                                              'yours at '
                                                              'advantage'}},
                                            {'name': 'Hush',
                                             'srd': False,
                                             'mech': {'kind': 'advantage',
                                                      'on': 'Stealth checks',
                                                      'scope': 'you and all '
                                                               'allies',
                                                      'cost': '1 ki',
                                                      'duration': 'until the '
                                                                  'party '
                                                                  'attacks '
                                                                  'or is '
                                                                  'spotted'}},
                                            {'name': 'Dark Reading',
                                             'srd': False,
                                             'mech': {'kind': 'flavor',
                                                      'text': 'you read '
                                                              'shape and '
                                                              'motion in '
                                                              'total '
                                                              'darkness as '
                                                              'easily as '
                                                              'print'}}]}},
 'way of the honed edge': {'class': 'monk',
                           'level': 3,
                           'display': 'Honed Edge',
                           'blurb': 'The weapon is a syllable in the '
                                    'sentence of the body.',
                           'srd': False,
                           'features': {3: [{'name': 'Chosen Implement',
                                             'srd': False,
                                             'mech': {'kind': 'choice',
                                                      'when': 'level-up',
                                                      'options': [{'label': 'longsword',
                                                                   'kind': 'passive_bonus',
                                                                   'what': 'monk '
                                                                           'weapon',
                                                                   'value': 'longsword'},
                                                                  {'label': 'rapier',
                                                                   'kind': 'passive_bonus',
                                                                   'what': 'monk '
                                                                           'weapon',
                                                                   'value': 'rapier'},
                                                                  {'label': 'trident',
                                                                   'kind': 'passive_bonus',
                                                                   'what': 'monk '
                                                                           'weapon',
                                                                   'value': 'trident'}],
                                                      'note': 'the chosen '
                                                              'weapon uses '
                                                              'your '
                                                              'martial-arts '
                                                              'die and DEX, '
                                                              'like unarmed '
                                                              'strikes'}},
                                            {'name': 'Guard Form',
                                             'srd': False,
                                             'mech': {'kind': 'passive_bonus',
                                                      'what': 'ac',
                                                      'amount': 2,
                                                      'when': 'you made an '
                                                              'unarmed '
                                                              'strike this '
                                                              'turn',
                                                      'duration': 'until '
                                                                  'your next '
                                                                  'turn'}},
                                            {'name': 'Read the Grain',
                                             'srd': False,
                                             'mech': {'kind': 'flavor',
                                                      'text': 'hold any '
                                                              'weapon and '
                                                              'you know its '
                                                              'age, its last '
                                                              "owner's "
                                                              'habits, and '
                                                              'its next '
                                                              'flaw'}}]}},
 'oath of the reckoning': {'class': 'paladin',
                           'level': 3,
                           'display': 'Reckoning',
                           'blurb': 'Some debts are collected in daylight, '
                                    'with interest.',
                           'srd': False,
                           'features': {3: [{'name': 'Writ of the Hunt',
                                             'srd': False,
                                             'mech': {'kind': 'always_prepared',
                                                      'spells': ["hunter's "
                                                                 'mark',
                                                                 'command']}},
                                            {'name': 'Rite of the Sworn Foe',
                                             'srd': False,
                                             'mech': {'kind': 'advantage',
                                                      'on': 'your attack '
                                                            'rolls against '
                                                            'the sworn foe',
                                                      'cost': 'bonus action '
                                                              'to swear '
                                                              'against one '
                                                              'creature you '
                                                              'can see',
                                                      'uses': 1,
                                                      'refresh': 'short',
                                                      'duration': 'until the '
                                                                  'battle '
                                                                  'ends'}},
                                            {'name': 'Cold Trail Never',
                                             'srd': False,
                                             'mech': {'kind': 'flavor',
                                                      'text': 'you always '
                                                              'know the '
                                                              'compass '
                                                              'direction of '
                                                              'any creature '
                                                              'that has '
                                                              'drawn an '
                                                              "ally's blood "
                                                              'within the '
                                                              'last day'}}]}},
 'oath of the greenwood': {'class': 'paladin',
                           'level': 3,
                           'display': 'Greenwood',
                           'blurb': 'Joy is a fortress; keep it garrisoned.',
                           'srd': False,
                           'features': {3: [{'name': 'Greenwood Writ',
                                             'srd': False,
                                             'mech': {'kind': 'always_prepared',
                                                      'spells': ['ensnaring '
                                                                 'strike',
                                                                 'healing '
                                                                 'word']}},
                                            {'name': 'Grasp of Roots',
                                             'srd': False,
                                             'mech': {'kind': 'rider',
                                                      'trigger': 'action: '
                                                                 'roots '
                                                                 'erupt '
                                                                 'beneath '
                                                                 'one foe '
                                                                 'you can '
                                                                 'see',
                                                      'condition': 'restrained '
                                                                   'until '
                                                                   'the end '
                                                                   'of its '
                                                                   'next '
                                                                   'turn',
                                                      'save': ('str',
                                                               '8+prof+cha'),
                                                      'on_save': 'unaffected',
                                                      'uses': 1,
                                                      'refresh': 'short'}},
                                            {'name': 'Warmth of the Glade',
                                             'srd': False,
                                             'mech': {'kind': 'heal_bonus',
                                                      'amount': 'cha_mod',
                                                      'when': 'any spell you '
                                                              'cast restores '
                                                              'HP',
                                                      'also': {'kind': 'aura',
                                                               'range': 10,
                                                               'effect': 'allies '
                                                                         'gain '
                                                                         '+1 '
                                                                         'on '
                                                                         'saves '
                                                                         'against '
                                                                         'the '
                                                                         'charmed '
                                                                         'and '
                                                                         'frightened '
                                                                         'conditions'}}}]}},
 'warden of the pack': {'class': 'ranger',
                        'level': 3,
                        'display': 'Pack',
                        'blurb': 'You never tamed the beast -- you were '
                                 'adopted by it.',
                        'srd': False,
                        'features': {3: [{'name': 'Pack Companion',
                                          'srd': False,
                                          'mech': {'kind': 'companion',
                                                   'stat_block': 'boar',
                                                   'note': 'SRD boar '
                                                           '(R5M_BOAR, '
                                                           'already '
                                                           'in-engine); acts '
                                                           'on your turn and '
                                                           'obeys simple '
                                                           'orders; add your '
                                                           'prof to its '
                                                           'attack rolls; if '
                                                           'it falls, a long '
                                                           'rest calls '
                                                           'another of its '
                                                           'line'}},
                                         {'name': 'Hunt as One',
                                          'srd': False,
                                          'mech': {'kind': 'advantage',
                                                   'on': 'your attack rolls '
                                                         'against enemies '
                                                         'adjacent to your '
                                                         'companion'}},
                                         {'name': 'Shared Supper',
                                          'srd': False,
                                          'mech': {'kind': 'flavor',
                                                   'text': 'your companion '
                                                           'eats first; in '
                                                           'exchange it has '
                                                           'never once let '
                                                           'something sneak '
                                                           'up on your '
                                                           'camp'}}]}},
 'the rift strider': {'class': 'ranger',
                      'level': 3,
                      'display': 'Rift',
                      'blurb': 'The world has seams, and you know where they '
                               'gape.',
                      'srd': False,
                      'features': {3: [{'name': 'Seam Step',
                                        'srd': False,
                                        'mech': {'kind': 'resource',
                                                 'name': 'seam steps',
                                                 'pool': 'wis_mod (min 1)',
                                                 'refresh': 'long',
                                                 'spend': {'kind': 'impose_dis',
                                                           'on': 'attack '
                                                                 'rolls '
                                                                 'against '
                                                                 'you until '
                                                                 'your next '
                                                                 'turn',
                                                           'action': 'bonus',
                                                           'note': 'you slip '
                                                                   'through '
                                                                   'a seam '
                                                                   'and '
                                                                   'reappear '
                                                                   'across '
                                                                   'the '
                                                                   'field, '
                                                                   'edges '
                                                                   'flickering'}}},
                                       {'name': 'Edgewise Strike',
                                        'srd': False,
                                        'mech': {'kind': 'rider',
                                                 'dice': '1d6',
                                                 'dmg_type': 'force',
                                                 'trigger': 'once per turn, '
                                                            'your first '
                                                            'weapon hit'}},
                                       {'name': 'Distant Doors',
                                        'srd': False,
                                        'mech': {'kind': 'flavor',
                                                 'text': 'you feel every '
                                                         'nearby crossing, '
                                                         'portal, and thin '
                                                         'place like a draft '
                                                         'under a door'}}]}},
 'the ambush artist': {'class': 'rogue',
                       'level': 3,
                       'display': 'Ambush',
                       'blurb': 'The fight is decided before the first '
                                'shout.',
                       'srd': False,
                       'features': {3: [{'name': 'First Cut',
                                         'srd': False,
                                         'mech': {'kind': 'advantage',
                                                  'on': 'attack rolls '
                                                        'against creatures '
                                                        'that have not yet '
                                                        'taken a turn this '
                                                        'battle'}},
                                        {'name': 'Perfect Opening',
                                         'srd': False,
                                         'mech': {'kind': 'crit_range',
                                                  'autocrit_when': 'the '
                                                                   'target '
                                                                   'is '
                                                                   'surprised',
                                                  'note': 'engine '
                                                          'R5F_AUTOCRIT: a '
                                                          'hit against a '
                                                          'surprised target '
                                                          'is a critical '
                                                          'hit'}},
                                        {'name': 'Faces and Phials',
                                         'srd': False,
                                         'mech': {'kind': 'flavor',
                                                  'text': 'you keep a second '
                                                          'face and a small '
                                                          'bottle for every '
                                                          'occasion; '
                                                          'officials '
                                                          'remember meeting '
                                                          'someone else'}}]}},
 'the duelist': {'class': 'rogue',
                 'level': 3,
                 'display': 'Duelist',
                 'blurb': 'One opponent at a time, loudly and beautifully.',
                 'srd': False,
                 'features': {3: [{'name': 'Single Combat',
                                   'srd': False,
                                   'mech': {'kind': 'passive_bonus',
                                            'what': 'sneak attack gating',
                                            'value': 'no adjacent ally '
                                                     'needed when no '
                                                     'creature other than '
                                                     'you is within reach of '
                                                     'the target'}},
                                  {'name': 'Cutting Retort',
                                   'srd': False,
                                   'mech': {'kind': 'impose_dis',
                                            'on': "the struck foe's attack "
                                                  'rolls against anyone but '
                                                  'you',
                                            'trigger': 'once per turn, when '
                                                       'your weapon hits a '
                                                       'creature',
                                            'duration': 'until your next '
                                                        'turn'}},
                                  {'name': 'Light Feet',
                                   'srd': False,
                                   'mech': {'kind': 'passive_bonus',
                                            'what': 'ac',
                                            'amount': 1,
                                            'when': 'no ally is adjacent to '
                                                    'you'}}]}},
 'riotous blood': {'class': 'sorcerer',
                   'level': 1,
                   'display': 'Riot Blood',
                   'blurb': 'Your magic arrives like weather: usually yours, '
                            "sometimes everyone's.",
                   'srd': False,
                   'features': {1: [{'name': 'Spellstorm Hiccup',
                                     'srd': False,
                                     'mech': {'kind': 'random',
                                              'die': 4,
                                              'when': 'you cast a spell of '
                                                      '1st level or higher, '
                                                      'roll 1d6; on a 5-6 '
                                                      'the storm answers',
                                              'options': [{'label': 'Crackle',
                                                           'kind': 'rider',
                                                           'dice': '1d6',
                                                           'dmg_type': 'lightning',
                                                           'trigger': 'the '
                                                                      "spell's "
                                                                      'target '
                                                                      '(or '
                                                                      'nearest '
                                                                      'enemy)'},
                                                          {'label': 'Stormskin',
                                                           'kind': 'temp_hp',
                                                           'amount': '1d6',
                                                           'target': 'you'},
                                                          {'label': 'Overflow',
                                                           'kind': 'heal_bonus',
                                                           'amount': '1d4',
                                                           'target': 'the '
                                                                     'lowest-HP '
                                                                     'ally'},
                                                          {'label': 'Backwash',
                                                           'kind': 'rider',
                                                           'dice': '1d4',
                                                           'dmg_type': 'psychic',
                                                           'trigger': 'you '
                                                                      '(the '
                                                                      'storm '
                                                                      'bites '
                                                                      'back)'}]}},
                                    {'name': 'Borrowed Luck',
                                     'srd': False,
                                     'mech': {'kind': 'advantage',
                                              'on': 'one attack roll or '
                                                    'saving throw, declared '
                                                    'before rolling',
                                              'uses': 1,
                                              'refresh': 'long; also '
                                                         'restored whenever '
                                                         'Spellstorm Hiccup '
                                                         'fires'}}]}},
 'blood of the storm': {'class': 'sorcerer',
                        'level': 1,
                        'display': 'Storm',
                        'blurb': 'Thunder is just your pulse, amplified.',
                        'srd': False,
                        'features': {1: [{'name': 'Skirl of Wind',
                                          'srd': False,
                                          'mech': {'kind': 'impose_dis',
                                                   'on': 'melee attack rolls '
                                                         'against you until '
                                                         'your next turn',
                                                   'when': 'after you cast a '
                                                           'spell of 1st '
                                                           'level or higher '
                                                           '(bonus action)',
                                                   'note': 'a gust lifts you '
                                                           'a pace off the '
                                                           'ground -- the '
                                                           'hop is the '
                                                           'flavor, the '
                                                           'disadvantage is '
                                                           'the mechanic'}},
                                         {'name': 'Charged Casting',
                                          'srd': False,
                                          'mech': {'kind': 'rider',
                                                   'dice': '1d4',
                                                   'dmg_type': 'lightning',
                                                   'trigger': 'once per '
                                                              'turn, when a '
                                                              'spell you '
                                                              'cast deals '
                                                              'damage'}},
                                         {'name': 'Weather Wise',
                                          'srd': False,
                                          'mech': {'kind': 'flavor',
                                                   'text': 'you know '
                                                           "tomorrow's "
                                                           'weather the way '
                                                           'others remember '
                                                           "yesterday's"}}]}},
 'the laughing court': {'class': 'warlock',
                        'level': 1,
                        'display': 'Fey Court',
                        'blurb': 'Your patron collects jokes; mortals are '
                                 'the best ones.',
                        'srd': False,
                        'features': {1: [{'name': 'Court Gifts',
                                          'srd': False,
                                          'mech': {'kind': 'always_prepared',
                                                   'spells': ['faerie fire',
                                                              'hideous '
                                                              'laughter']}},
                                         {'name': 'Gift of Giddy Mirth',
                                          'srd': False,
                                          'mech': {'kind': 'rider',
                                                   'trigger': 'action: a '
                                                              'burst of '
                                                              'glamour '
                                                              'washes over '
                                                              'each enemy '
                                                              'adjacent to '
                                                              'you',
                                                   'condition': 'charmed or '
                                                                'frightened '
                                                                '(your '
                                                                'choice) '
                                                                'until the '
                                                                'end of your '
                                                                'next turn',
                                                   'save': ('wis',
                                                            '8+prof+cha'),
                                                   'on_save': 'unaffected',
                                                   'uses': 1,
                                                   'refresh': 'short'}},
                                         {'name': 'Guest-Right',
                                          'srd': False,
                                          'mech': {'kind': 'flavor',
                                                   'text': 'fey creatures '
                                                           'treat you as an '
                                                           'envoy under an '
                                                           'old truce '
                                                           'neither side '
                                                           'fully '
                                                           'remembers'}}],
                                     3: [{'name': 'Court Gifts II',
                                          'srd': False,
                                          'mech': {'kind': 'always_prepared',
                                                   'spells': ['calm emotions',
                                                              'invisibility']}}]}},
 'the voice beneath': {'class': 'warlock',
                       'level': 1,
                       'display': 'Voice',
                       'blurb': 'Something vast rolled over in its sleep, '
                                'and now it talks in yours.',
                       'srd': False,
                       'features': {1: [{'name': 'Skull Static',
                                         'srd': False,
                                         'mech': {'kind': 'flavor',
                                                  'text': 'you can push '
                                                          'short phrases '
                                                          'directly into any '
                                                          'mind you can see; '
                                                          'nobody enjoys '
                                                          'it'}},
                                        {'name': 'Whispered Barb',
                                         'srd': False,
                                         'mech': {'kind': 'rider',
                                                  'dice': '1d4',
                                                  'dmg_type': 'psychic',
                                                  'trigger': 'once per turn, '
                                                             'when a spell '
                                                             'you cast '
                                                             'damages a '
                                                             'creature'}},
                                        {'name': 'Nightmare Recoil',
                                         'srd': False,
                                         'mech': {'kind': 'reaction',
                                                  'trigger': 'a creature '
                                                             'hits you with '
                                                             'a melee attack',
                                                  'dice': '1d4',
                                                  'dmg_type': 'psychic',
                                                  'save': ('wis',
                                                           '8+prof+cha'),
                                                  'on_save': 'no damage',
                                                  'uses': 'cha_mod (min 1)',
                                                  'refresh': 'long'}}],
                                    3: [{'name': 'Deep Library',
                                         'srd': False,
                                         'mech': {'kind': 'always_prepared',
                                                  'spells': ['detect '
                                                             'thoughts',
                                                             'suggestion']}}]}},
 'school of abjuration': {'class': 'wizard',
                          'level': 2,
                          'display': 'Abjuration',
                          'blurb': 'The best spell is the one your enemy '
                                   'wasted on your shield.',
                          'srd': False,
                          'features': {2: [{'name': 'Ward Sigil',
                                            'srd': False,
                                            'mech': {'kind': 'temp_hp',
                                                     'amount': '2*level',
                                                     'pool': True,
                                                     'when': 'you cast an '
                                                             'abjuration '
                                                             'spell of 1st '
                                                             'level or '
                                                             'higher',
                                                     'note': 'a standing '
                                                             'ward (engine '
                                                             'temp-HP pool); '
                                                             'while it '
                                                             'holds, '
                                                             'recasting '
                                                             'refills it by '
                                                             'twice the '
                                                             "spell's level "
                                                             'instead of '
                                                             'restarting '
                                                             'it'}},
                                           {'name': 'Well-Worn Wards',
                                            'srd': False,
                                            'mech': {'kind': 'flavor',
                                                     'text': 'warding '
                                                             'formulae come '
                                                             'cheap to you: '
                                                             'half the time '
                                                             'and gold to '
                                                             'copy one into '
                                                             'your book'}}]}},
 'school of necromancy': {'class': 'wizard',
                          'level': 2,
                          'display': 'Necromancy',
                          'blurb': 'Nothing is wasted; every ending stocks '
                                   'the pantry.',
                          'srd': False,
                          'features': {2: [{'name': "Death's Dividend",
                                            'srd': False,
                                            'mech': {'kind': 'heal_bonus',
                                                     'amount': '2 + the '
                                                               "spell's "
                                                               'level',
                                                     'when': 'once per turn, '
                                                             'a spell you '
                                                             'cast drops a '
                                                             'creature to 0 '
                                                             'HP',
                                                     'target': 'you',
                                                     'note': 'nothing owed '
                                                             'on constructs '
                                                             'or the '
                                                             'already-dead'}},
                                           {'name': 'Cold Comfort',
                                            'srd': False,
                                            'mech': {'kind': 'always_prepared',
                                                     'spells': ['chill '
                                                                'touch']}},
                                           {'name': 'Pale Scholarship',
                                            'srd': False,
                                            'mech': {'kind': 'flavor',
                                                     'text': 'grave-school '
                                                             'formulae come '
                                                             'cheap to you: '
                                                             'half the time '
                                                             'and gold to '
                                                             'copy one into '
                                                             'your book'}}]}}}

# --------------------------------------------------------------------------
# RACES_HB
#
# Homebrew githyanki for origin Lae'zel: our numbers, our text.
# --------------------------------------------------------------------------

# 5.1 half-elf RAW grants "+1 to two other ability scores of your choice" --
# a floating pick, and fixed ASIs are doctrine (character2.md: "Racial ASIs
# are FIXED per 5.1"). This pin is OUR game decision, one per race, chosen
# for the half-elf the game actually casts: Shadowheart, a trickery cleric.
RACE_ASI_PINS = {'half-elf': {'dex': 1, 'wis': 1}}

RACES_HB = {'githyanki': {'srd': False,
               'blurb': 'Sword-drilled exiles of a stolen people: they trust '
                        'steel, discipline, and very little else.',
               'asi': {'str': 2, 'int': 1},
               'speed': 30,
               'traits': [{'name': 'Warrior-Caste Schooling',
                           'srd': False,
                           'mech': {'kind': 'passive_bonus',
                                    'what': 'proficiencies',
                                    'value': ['light armor',
                                              'medium armor',
                                              'shortsword',
                                              'longsword',
                                              'greatsword']}},
                          {'name': 'Silver Reach',
                           'srd': False,
                           'mech': {'kind': 'advantage',
                                    'on': 'one Sleight of Hand or '
                                          'Investigation check, declared '
                                          'before rolling',
                                    'uses': 1,
                                    'refresh': 'long',
                                    'note': 'an unseen hand of will does the '
                                            'fine work at ten paces (R5F_ADV '
                                            'on a field_check)'}},
                          {'name': 'Languages',
                           'srd': False,
                           'mech': {'kind': 'flavor',
                                    'text': 'Common and Gith'}}],
               'subraces': {}}}
