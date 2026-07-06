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

WEAPON_MAP = [  # (enum, source, key) in rules.h R5W_ order
    ("R5W_DAGGER", "srd", "dagger"), ("R5W_SHORTSWORD", "srd", "shortsword"),
    ("R5W_LONGSWORD", "srd", "longsword"), ("R5W_GREATSWORD", "srd", "greatsword"),
    ("R5W_RAPIER", "srd", "rapier"), ("R5W_MACE", "srd", "mace"),
    ("R5W_QUARTERSTAFF", "srd", "quarterstaff"), ("R5W_SHORTBOW", "srd", "shortbow"),
    ("R5W_LONGBOW", "srd", "longbow"), ("R5W_TRIDENT", "srd", "trident"),
    ("R5W_EVERBURN", "ovr", "everburn blade"), ("R5W_STINGER", "ovr", "imp stinger"),
]

def gen_weapons(o):
    o.append("const R5Weapon r5_weapons[] = {")
    for enum, src, key in WEAPON_MAP:
        w = (SRD.WEAPONS if src == "srd" else OVR.WEAPONS)[key]
        props = [WP[p] for p in w["properties"] if p in WP]
        if not w.get("melee", True):
            props.append("WP_RANGED")
        rider = w.get("rider")
        if rider:
            ab, dc = rider["save"]
            rtxt = "%s, %s, %d, %d" % (dice(rider["dice"]),
                                       DT[rider["dmg_type"]], AB[ab], dc)
        else:
            rtxt = "{ 0, 0, 0 }, 0, 0, 0"
        o.append("    [%s] = { \"%s\", %s, %s, %s, %s, %s }," % (
            enum, w.get("display", key.capitalize()), dice(w["dice"]),
            dice(w.get("versatile_dice", "")),
            " | ".join(props) if props else "0", DT[w["dmg_type"]], rtxt))
    o.append("};")

# ---------------------------------------------------------------- classes

CLASS_MAP = [  # rules.h R5C_ order
    ("R5C_BARD", "bard"), ("R5C_ROGUE", "rogue"), ("R5C_RANGER", "ranger"),
    ("R5C_WIZARD", "wizard"), ("R5C_FIGHTER", "fighter"), ("R5C_CLERIC", "cleric"),
    ("R5C_BARBARIAN", "barbarian"), ("R5C_DRUID", "druid"), ("R5C_MONK", "monk"),
    ("R5C_PALADIN", "paladin"), ("R5C_SORCERER", "sorcerer"), ("R5C_WARLOCK", "warlock"),
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
        def lv(tab, lvl):
            v = (tab or {}).get(lvl, 0)
            return v if isinstance(v, int) else (v.get("uses", 0) if v else 0)
        rsrc = []
        for l in (0, 1, 2, 3):
            if l == 0: rsrc.append([0] * 6); continue
            lay = c.get("lay_on_hands")
            pact = c.get("pact_slots", {}).get(l, {})
            rsrc.append([
                lv(c.get("rages"), l),
                lv(c.get("ki_points"), l),
                lv(c.get("sorcery_points"), l),
                (5 * l) if lay else 0,
                lv(c.get("wild_shape"), l),
                sum(pact.values()) if pact else 0,
            ])
        pact_lvl = [0] + [max(c.get("pact_slots", {}).get(l, {0: 0}).keys() or [0])
                          for l in (1, 2, 3)]
        o.append("    [%s] = { %d, %s, { %s }, { %s }, { %s },\n"
                 ""
                 "             { %s }, { %s } }," % (
            enum, c["hit_die"], saves,
            ", ".join(map(str, prof)),
            ", ".join("{ %s }" % ", ".join(map(str, r)) for r in slots),
            ", ".join(map(str, sneak)),
            ", ".join("{ %s }" % ", ".join(map(str, r)) for r in rsrc),
            ", ".join(map(str, pact_lvl))))
    o.append("};")

# ---------------------------------------------------------------- spells

LEGACY_SPELLS = [  # original ids, order frozen (encounter.c uses these names)
    "vicious mockery", "healing word", "fire bolt", "magic missile", "sleep",
    "cure wounds", "guiding bolt", "bless", "hunter's mark",
]
HEAL_SPELLS = {"healing word", "cure wounds", "mass healing word", "prayer of healing"}
COND_BITS = { "blinded": 0, "charmed": 1, "deafened": 2, "frightened": 3,
    "grappled": 4, "incapacitated": 5, "invisible": 6, "paralyzed": 7,
    "petrified": 8, "poisoned": 9, "prone": 10, "restrained": 11,
    "stunned": 12, "unconscious": 13 }
KINDS = { "attack": 0, "save": 1, "heal": 2, "buff": 3, "condition": 4,
          "multi": 5, "pool": 6, "utility": 7 }
CLS_ORDER = ["bard", "rogue", "ranger", "wizard", "fighter", "cleric",
             "barbarian", "druid", "monk", "paladin", "sorcerer", "warlock"]

def spell_enum(key):
    out = "R5S_"
    for ch in key.upper():
        if ch == "'": continue            # hunter's -> HUNTERS (legacy names)
        out += ch if ch.isalnum() else "_"
    while "__" in out: out = out.replace("__", "_")
    return out.rstrip("_")

def spell_order():
    rest = sorted((k for k in SRD.SPELLS if k not in LEGACY_SPELLS),
                  key=lambda k: (SRD.SPELLS[k]["level"], k))
    return LEGACY_SPELLS + rest

def gen_spell_ids(h):
    h.append("/* generated: R5S_* spell ids (legacy 13 first, then by level/name) */")
    h.append("#ifndef SRD_IDS_H")
    h.append("#define SRD_IDS_H")
    h.append("enum {")
    for i, key in enumerate(spell_order()):
        h.append(f"    {spell_enum(key)} = {i},")
    h.append(f"    R5S_COUNT = {len(spell_order())}")
    h.append("};")
    h.append("#endif")

def gen_spells(o):
    o.append("const R5Spell r5_spells[R5S_COUNT] = {")
    for key in spell_order():
        s = SRD.SPELLS[key]
        enum = spell_enum(key)
        d, count, plus = s.get("dice"), 1, 0
        if isinstance(d, dict):
            count, plus, d = d.get("count", 1), d.get("plus", 0), d.get("dice")
        save_ab, save_half = "0xFF", 0
        sv = s.get("save")
        if sv:
            ab, mode = (sv[0], sv[1]) if isinstance(sv, (list, tuple)) else (sv, "negate")
            save_ab, save_half = str(AB[ab]), 1 if mode == "half" else 0
        heal = key in HEAL_SPELLS or s.get("kind") == "heal"
        add_mod = 1 if heal else 0
        dt = DT.get(s.get("dmg_type") or "", "0")
        kind = KINDS.get(s.get("kind", "utility"), 7)
        cond = s.get("condition")
        if isinstance(cond, (list, tuple)): cond = cond[0]
        cond_bit = COND_BITS.get(cond, 255) if cond else 255
        rounds = s.get("rounds") or 0
        if rounds > 250: rounds = 250
        aoe = s.get("aoe")
        tgtf = s.get("targets")
        if tgtf == "party": tgt = 250
        elif tgtf == "self": tgt = 251
        elif aoe: tgt = 2 if (aoe.get("size_ft", 15) <= 15) else 3
        else: tgt = 1
        clsmask = 0
        for cn in s.get("classes", []):
            if cn in CLS_ORDER: clsmask |= 1 << CLS_ORDER.index(cn)
        cast = s.get("cast", s.get("action", "action"))
        o.append('    [%s] = { "%s", %d, %d, %d, %d, %s, %d, %s, %d, %s, %d, %d, %d, %d, %d, %d, 0x%03x },' % (
            enum, key.title().replace("'S", "'s"), s["level"],
            1 if cast == "bonus" else 0,
            1 if s.get("concentration") else 0,
            1 if s.get("attack") else 0,
            save_ab, save_half, dice(d, plus), count, dt, heal, add_mod,
            kind, cond_bit, rounds, tgt, clsmask))
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
    ("R5M_THRALL", "ovr", "awakened thrall"),
]

def attack_init(a):
    x = a.get("extra")
    if x:
        save_ab, save_dc = x["save"]
        rider = "%s, %s, %d, %d" % (dice(x["dice"]), DT[x["dmg_type"]],
                                    AB[save_ab], save_dc)
    else:
        rider = "{ 0, 0, 0 }, 0, 0, 0"
    return "{ \"%s\", %d, %s, %s, %s, %d }" % (
        a["name"], a["to_hit"], dice(a["dice"], a.get("plus", 0)),
        DT[a["dmg_type"]], rider, 1 if a.get("ranged") else 0)

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


# ---------------------------------------------------------------- subclasses

def all_subclasses():
    subs = dict(SRD.SUBCLASSES)
    subs.update(getattr(OVR, "SUBCLASSES_HB", {}))
    # stable order: by class enum, srd-first, then name
    def key(kv):
        n, s = kv
        cls = CLS_ORDER.index(s["class"]) if s["class"] in CLS_ORDER else 99
        return (cls, 0 if s.get("srd", not n[0].isupper()) else 1, n)
    return sorted(subs.items(), key=lambda kv: key(kv))

def sub_enum(name):
    out = "R5SUB_"
    for ch in name.upper():
        out += ch if ch.isalnum() else "_"
    while "__" in out: out = out.replace("__", "_")
    return out.rstrip("_")

def gen_sub_ids(h):
    h.append("/* generated: R5SUB_* subclass ids */")
    order = all_subclasses()
    for i, (name, s) in enumerate(order):
        h.append(f"#define {sub_enum(name)} {i}")
    h.append(f"#define R5SUB_COUNT {len(order)}")

SPELL_INDEX = {k: i for i, k in enumerate(None or [])}  # filled in gen_spells era

def spell_bit(key):
    order = spell_order()
    return order.index(key) if key in order else -1

def gen_subclasses(o):
    o.append("const R5Subclass r5_subclasses[R5SUB_COUNT] = {")
    for name, s in all_subclasses():
        disp = s.get("display", name.title())[:10]
        cls = CLS_ORDER.index(s["class"])
        passive = []
        prepared = 0
        for lvl, feats in s["features"].items():
            for f in feats:
                mk = f["mech"].get("kind")
                if mk == "crit_range": passive.append("SUBP_CRIT19")
                elif mk == "heal_bonus": passive.append("SUBP_HEAL_DISCIPLE")
                elif mk in ("always_prepared", "spell_list_extend"):
                    v = f["mech"].get("spells", f["mech"].get("list", []))
                    def walk(x):
                        if isinstance(x, str):
                            b = spell_bit(x)
                            return 1 << b if 0 <= b < 32 else 0
                        if isinstance(x, dict):
                            r = 0
                            for y in x.values(): r |= walk(y)
                            return r
                        if isinstance(x, (list, tuple)):
                            r = 0
                            for y in x: r |= walk(y)
                            return r
                        return 0
                    prepared |= walk(v)
        pf = " | ".join(sorted(set(passive))) or "0"
        srd = 1 if s.get("srd", not name[0].isupper()) else 0
        o.append('    [%s] = { "%s", %d, %d, %s, %d, 0x%08x },' % (
            sub_enum(name), disp, cls, s["level"], pf, srd, prepared))
    o.append("};")


def gen_subclass_menu(o):
    """per-class arrays of subclass ids + their choose-level, for the UI"""
    order = all_subclasses()
    bycls = {}
    for i, (name, s) in enumerate(order):
        bycls.setdefault(CLS_ORDER.index(s["class"]), []).append((i, name, s))
    o.append("const uint8_t r5_subclass_of_class[R5C_COUNT][4] = {")
    for cls in range(12):
        ids = [str(i) for i, n, s in bycls.get(cls, [])][:3]
        while len(ids) < 4: ids.append("255")
        o.append("    { %s }," % ", ".join(ids))
    o.append("};")
    o.append("const uint8_t r5_subclass_level[R5C_COUNT] = {")
    for cls in range(12):
        lst = bycls.get(cls, [])
        lvl = lst[0][2]["level"] if lst else 3
        o.append("    %d," % lvl)
    o.append("};")


def gen_class_spells(o):
    """per-class spell option lists (levels 0-2) as id arrays for prepare menus"""
    order = spell_order()
    idx = {k: i for i, k in enumerate(order)}
    o.append("const uint8_t r5_class_spells[R5C_COUNT][16] = {")
    for cls in range(12):
        cn = CLS_ORDER[cls]
        ids = [idx[k] for k in order if cn in SRD.SPELLS[k].get("classes", [])][:16]
        while len(ids) < 16: ids.append(255)
        o.append("    { %s }," % ", ".join(str(i) for i in ids))
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
    gen_monsters(o); o.append("")
    gen_subclasses(o); o.append("")
    gen_subclass_menu(o); o.append("")
    gen_class_spells(o)
    with open(OUT, "w") as f:
        f.write("\n".join(o) + "\n")
    h = []
    gen_spell_ids(h)
    gen_sub_ids(h)
    with open(os.path.join(os.path.dirname(OUT), "srd_ids.h"), "w") as f:
        f.write("\n".join(h) + "\n")
    print(f"srd tables: {len(WEAPON_MAP)} weapons, {len(CLASS_MAP)} classes, "
          f"{len(spell_order())} spells, {len(MONSTER_MAP)} monsters -> {OUT}")

if __name__ == "__main__":
    main()
