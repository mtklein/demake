#!/usr/bin/env python3
"""Generate build/gen/srd_tables.c from tools/srd/srd_data.py + overrides.py.

Replaces the hand-written stopgap tables: SRD facts flow from the extracted
data (CC-BY-4.0, see tools/srd/ATTRIBUTION.md); non-SRD prologue monsters
come from our homebrew overrides. Validates the load-bearing class features
(Second Wind at fighter 1, Action Surge at 2, ...) at generation time.
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, os.path.join(HERE, "srd"))
import srd_data as SRD
import overrides as OVR

OUT = os.path.join(ROOT, "build", "gen", "srd_tables.c")

AB = {"str": 0, "dex": 1, "con": 2, "int": 3, "wis": 4, "cha": 5}
DT = {"slashing": "DT_SLASHING", "piercing": "DT_PIERCING",
      "bludgeoning": "DT_BLUDGEONING", "fire": "DT_FIRE", "cold": "DT_COLD",
      "poison": "DT_POISON", "psychic": "DT_PSYCHIC", "radiant": "DT_RADIANT",
      "necrotic": "DT_NECROTIC", "lightning": "DT_LIGHTNING",
      "thunder": "DT_THUNDER", "acid": "DT_ACID", "force": "DT_FORCE"}
WP = {"finesse": "WP_FINESSE", "light": "WP_LIGHT", "heavy": "WP_HEAVY",
      "two-handed": "WP_TWO_HANDED", "versatile": "WP_VERSATILE",
      "thrown": "WP_THROWN", "ammunition": "WP_AMMUNITION",
      "loading": "WP_LOADING", "reach": "WP_REACH"}

def dice(s, plus=0):
    """'1d8' or '2d6+1' -> C initializer {n, sides, mod}"""
    if not s:
        return "{ 0, 0, 0 }"
    m = re.fullmatch(r"(\d+)d(\d+)(?:\+(\d+))?", s)
    assert m, f"bad dice {s!r}"
    return "{ %s, %s, %d }" % (m.group(1), m.group(2),
                               int(m.group(3) or 0) + plus)

def dmg_mask(names):
    bits = []
    for n in names or []:
        if n in DT:
            bits.append(f"(1 << {DT[n]})")
        # compound entries ("... from nonmagical attacks") are battle-layer
        # concerns we don't model; skip silently but visibly:
        else:
            print(f"  note: skipping compound resistance {n!r}")
    return " | ".join(bits) if bits else "0"

# ---------------------------------------------------------------- weapons

WEAPON_MAP = [  # (enum, srd key) in rules.h R5W_ order
    ("R5W_DAGGER", "dagger"), ("R5W_SHORTSWORD", "shortsword"),
    ("R5W_LONGSWORD", "longsword"), ("R5W_GREATSWORD", "greatsword"),
    ("R5W_RAPIER", "rapier"), ("R5W_MACE", "mace"),
    ("R5W_QUARTERSTAFF", "quarterstaff"), ("R5W_SHORTBOW", "shortbow"),
    ("R5W_LONGBOW", "longbow"), ("R5W_TRIDENT", "trident"),
]

def gen_weapons(o):
    o.append("const R5Weapon r5_weapons[] = {")
    for enum, key in WEAPON_MAP:
        w = SRD.WEAPONS[key]
        props = [WP[p] for p in w["properties"] if p in WP]
        if not w.get("melee", True):
            props.append("WP_RANGED")
        o.append("    [%s] = { \"%s\", %s, %s, %s, %s }," % (
            enum, key.capitalize(), dice(w["dice"]),
            dice(w.get("versatile_dice", "")),
            " | ".join(props) if props else "0", DT[w["dmg_type"]]))
    o.append("};")

# ---------------------------------------------------------------- classes

CLASS_MAP = [  # rules.h R5C_ order
    ("R5C_BARD", "bard"), ("R5C_ROGUE", "rogue"), ("R5C_RANGER", "ranger"),
    ("R5C_WIZARD", "wizard"), ("R5C_FIGHTER", "fighter"), ("R5C_CLERIC", "cleric"),
]

def gen_classes(o):
    f = SRD.CLASSES["fighter"]["features"]
    assert "Second Wind" in f[1], "fighter L1 must grant Second Wind"
    assert "Action Surge" in f[2], "Action Surge arrives at fighter L2"
    assert "Action Surge" not in f[1], "Action Surge must NOT be at L1"
    assert "Sneak Attack" in SRD.CLASSES["rogue"]["features"][1]
    assert "Cunning Action" in SRD.CLASSES["rogue"]["features"][2]

    o.append("const R5Class r5_classes[R5C_COUNT] = {")
    for enum, key in CLASS_MAP:
        c = SRD.CLASSES[key]
        saves = " | ".join(f"(1 << {AB[s]})" for s in c["saves"])
        prof = [0] + [c["prof_bonus"][l] for l in (1, 2, 3)]
        slots = [[0, 0, 0]]
        for l in (1, 2, 3):
            row = c.get("spell_slots", {}).get(l, {})
            slots.append([row.get(s, 0) for s in (1, 2, 3)])
        sneak = [0, 0, 0, 0]
        for l, d in c.get("sneak_attack", {}).items():
            sneak[l] = int(re.match(r"(\d+)d6", d).group(1))
        feats = "; ".join(f"L{l}: {', '.join(v)}"
                          for l, v in sorted(c["features"].items()))
        o.append(f"    /* {key}: {feats} */")
        o.append("    [%s] = { %d, %s, { %s }, { %s }, { %s } }," % (
            enum, c["hit_die"], saves,
            ", ".join(map(str, prof)),
            ", ".join("{ %s }" % ", ".join(map(str, r)) for r in slots),
            ", ".join(map(str, sneak))))
    o.append("};")

# ---------------------------------------------------------------- spells

SPELL_MAP = [  # rules.h R5S_ order
    ("R5S_VICIOUS_MOCKERY", "vicious mockery"),
    ("R5S_HEALING_WORD", "healing word"),
    ("R5S_FIRE_BOLT", "fire bolt"),
    ("R5S_MAGIC_MISSILE", "magic missile"),
    ("R5S_SLEEP", "sleep"),
    ("R5S_CURE_WOUNDS", "cure wounds"),
    ("R5S_GUIDING_BOLT", "guiding bolt"),
    ("R5S_BLESS", "bless"),
    ("R5S_HUNTERS_MARK", "hunter's mark"),
]
HEAL_SPELLS = {"healing word", "cure wounds"}

def gen_spells(o):
    o.append("const R5Spell r5_spells[] = {")
    for enum, key in SPELL_MAP:
        s = SRD.SPELLS[key]
        d, count, plus = s.get("dice"), 1, 0
        if isinstance(d, dict):
            count, plus, d = d["count"], d.get("plus", 0), d["dice"]
        save_ab, save_half = "0xFF", 0
        if s.get("save"):
            ab, mode = s["save"]
            save_ab, save_half = str(AB[ab]), 1 if mode == "half" else 0
        heal = key in HEAL_SPELLS
        add_mod = 1 if heal else 0
        dt = DT.get(s.get("dmg_type") or "", "0")
        o.append('    [%s] = { "%s", %d, %d, %d, %d, %s, %d, %s, %d, %s, %d, %d },' % (
            enum, key.title().replace("'S", "'s"), s["level"],
            1 if s["cast"] == "bonus" else 0,
            1 if s["concentration"] else 0,
            1 if s.get("attack") else 0,
            save_ab, save_half, dice(d, plus), count, dt, heal, add_mod))
    o.append("};")

# ---------------------------------------------------------------- monsters

MONSTER_MAP = [  # (enum, source dict, key) in rules.h R5M_ order
    ("R5M_IMP", "srd", "imp"),
    ("R5M_BOAR", "srd", "boar"),
    ("R5M_DEVOURER", "ovr", "intellect devourer"),
    ("R5M_CAMBION", "ovr", "lesser cambion"),
    ("R5M_FLAYER", "ovr", "mind flayer ally"),
    ("R5M_ZHALK", "ovr", "commander zhalk"),
    ("R5M_LESSER_IMP", "ovr", "lesser imp"),
    ("R5M_LESSER_BOAR", "ovr", "lesser hellsboar"),
]

def attack_init(a):
    x = a.get("extra")
    if x:
        save_ab, save_dc = x["save"]
        rider = "%s, %s, %d, %d" % (dice(x["dice"]), DT[x["dmg_type"]],
                                    AB[save_ab], save_dc)
    else:
        rider = "{ 0, 0, 0 }, 0, 0, 0"
    return "{ \"%s\", %d, %s, %s, %s }" % (
        a["name"], a["to_hit"], dice(a["dice"], a.get("plus", 0)),
        DT[a["dmg_type"]], rider)

def gen_monsters(o):
    for _, src, key in MONSTER_MAP:
        m = (SRD.MONSTERS if src == "srd" else OVR.MONSTERS)[key]
        if src == "srd":
            assert m.get("srd"), f"{key}: expected true-SRD stat block"
    o.append("const R5Monster r5_monsters[] = {")
    for enum, src, key in MONSTER_MAP:
        m = (SRD.MONSTERS if src == "srd" else OVR.MONSTERS)[key]
        ab = m["abilities"]
        attacks = m["attacks"][:2]
        o.append("    [%s] = { \"%s\", %d, %d, { %s }, %s, %s,\n"
                 "        { %s }, %d }," % (
            enum, m.get("display", key.title()), m["ac"], m["hp"],
            ", ".join(str(ab[k]) for k in ("str", "dex", "con", "int", "wis", "cha")),
            dmg_mask(m.get("resistances")), dmg_mask(m.get("immunities")),
            ",\n          ".join(attack_init(a) for a in attacks),
            len(attacks)))
    o.append("};")

def main():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    o = [
        "/* GENERATED by tools/mksrd.py -- do not edit.",
        " * SRD 5.1/5.2.1 content under CC-BY-4.0: see tools/srd/ATTRIBUTION.md.",
        " * Non-SRD prologue monsters are original homebrew (tools/srd/overrides.py). */",
        '#include "rules.h"', "",
    ]
    gen_weapons(o); o.append("")
    gen_classes(o); o.append("")
    gen_spells(o); o.append("")
    gen_monsters(o)
    with open(OUT, "w") as f:
        f.write("\n".join(o) + "\n")
    print(f"srd tables: {len(WEAPON_MAP)} weapons, {len(CLASS_MAP)} classes, "
          f"{len(MONSTER_MAP)} monsters -> {OUT}")

if __name__ == "__main__":
    main()
