#!/usr/bin/env python3
"""Generate build/gen/assets.{c,h} from Python-authored art/music sources.

GBA formats: 4bpp tiles (32 bytes: 8 rows x 4 bytes, low nibble = left pixel),
BGR555 palettes. Pass --preview to also render PNG sheets to test/shots/.
"""
import os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OUT = os.path.join(ROOT, "build", "gen")
sys.path.insert(0, HERE)

from font8x8 import FONT

# ---------------------------------------------------------------- helpers

def rgb15(r, g, b):
    assert 0 <= r < 32 and 0 <= g < 32 and 0 <= b < 32, (r, g, b)
    return r | (g << 5) | (b << 10)

def pack_tile(rows, legend):
    """rows: 8 strings of 8 chars; legend: char -> palette index. -> 16 u16s"""
    assert len(rows) == 8, rows
    out = []
    for row in rows:
        row = row.replace(" ", "")
        assert len(row) == 8, repr(row)
        px = [legend[c] for c in row]
        for i in (0, 4):
            out.append(px[i] | (px[i + 1] << 4) | (px[i + 2] << 8) | (px[i + 3] << 12))
    return out  # 16 halfwords

def hflip_tile(t):
    out = []
    for r in range(8):
        hw = t[r * 2], t[r * 2 + 1]
        px = []
        for h in hw:
            px += [h & 15, (h >> 4) & 15, (h >> 8) & 15, (h >> 12) & 15]
        px.reverse()
        out.append(px[0] | (px[1] << 4) | (px[2] << 8) | (px[3] << 12))
        out.append(px[4] | (px[5] << 4) | (px[6] << 8) | (px[7] << 12))
    return out

def grid_tiles(rows, legend, w_tiles, h_tiles):
    """Cut a (w*8 x h*8) pixel-string image into row-major 8x8 tiles."""
    tiles = []
    rows = [r.replace(" ", "") for r in rows]
    assert len(rows) == h_tiles * 8, (len(rows), h_tiles)
    for ty in range(h_tiles):
        for tx in range(w_tiles):
            sub = [rows[ty * 8 + y][tx * 8:(tx + 1) * 8] for y in range(8)]
            tiles.append(pack_tile(sub, legend))
    return tiles

# ---------------------------------------------------------------- palettes

# BG palette 0: UI, colors sampled from FF4 Advance screenshots.
UI_PAL = [
    (0, 0, 0),     # 0 transparent
    (31, 31, 31),  # 1 text white
    (4, 4, 4),     # 2 text shadow
    (7, 7, 30),    # 3 window gradient top (lightest)
    (5, 5, 26),    # 4
    (4, 4, 23),    # 5
    (2, 2, 19),    # 6
    (0, 0, 16),    # 7 window gradient bottom (darkest)
    (31, 31, 31),  # 8 border white
    (25, 25, 25),  # 9 border grey
    (4, 4, 4),     # 10 border near-black
    (4, 10, 17),   # 11 border slate-blue accent
    (31, 29, 11),  # 12 yellow (highlights, ATB full)
    (16, 17, 20),  # 13 grey text
    (30, 8, 6),    # 14 red
    (10, 29, 12),  # 15 heal green
]

def pal16(colors):
    assert len(colors) <= 16
    out = [rgb15(*c) for c in colors]
    return out + [0] * (16 - len(out))

BG_PALS = [[0] * 16 for _ in range(16)]
OBJ_PALS = [[0] * 16 for _ in range(16)]

BG_PALS[0] = pal16(UI_PAL)
# pal 1: yellow text variant; pal 2: grey text variant
yellow = list(UI_PAL); yellow[1] = (31, 30, 10)
grey = list(UI_PAL); grey[1] = (16, 17, 20)
BG_PALS[1] = pal16(yellow)
BG_PALS[2] = pal16(grey)

# OBJ palette 7: UI cursor hand
OBJ_PALS[7] = pal16([(0, 0, 0), (31, 31, 31), (24, 24, 26), (13, 13, 15), (1, 1, 2)])

# ---------------------------------------------------------------- font

def font_tiles():
    """ASCII 32..126 -> tiles 0..94, plus 95: dialogue 'more' arrow."""
    tiles = []
    for code in range(32, 127):
        g = FONT[chr(code)]
        solid = [[0] * 8 for _ in range(8)]
        for y in range(8):
            for x in range(8):
                if g[y] >> x & 1:
                    solid[y][x] = 1
        # drop shadow at +1,+1
        for y in range(8):
            for x in range(8):
                if solid[y][x] == 1 and y + 1 < 8 and x + 1 < 8 and not solid[y + 1][x + 1]:
                    solid[y + 1][x + 1] = 2
        t = []
        for y in range(8):
            px = solid[y]
            t.append(px[0] | (px[1] << 4) | (px[2] << 8) | (px[3] << 12))
            t.append(px[4] | (px[5] << 4) | (px[6] << 8) | (px[7] << 12))
        tiles.append(t)
    # 95: downward arrow marker
    arrow = pack_tile([
        "........",
        "11111111",
        ".111111.",
        ".111111.",
        "..1111..",
        "..1111..",
        "...11...",
        "........",
    ], {".": 0, "1": 1})
    tiles.append(arrow)
    return tiles

# ---------------------------------------------------------------- window chrome

def window_tiles():
    """FF4A window: 1px white, 1px grey, 1px near-black, 1px slate accent, gradient fill."""
    shades = "34567"  # palette indices for gradient rows top->bottom

    def leg(shade):
        return {".": 0, "W": 8, "g": 9, "b": 10, "a": 11, "i": int(shade)}

    corner_t_rows = [
        "..WWWWWW",
        ".WWggggg",
        "WWgbbbbb",
        "Wgbaaaaa",
        "Wgbaiiii",
        "Wgbaiiii",
        "Wgbaiiii",
        "Wgbaiiii",
    ]
    corner_b_rows = list(reversed(corner_t_rows))
    edge_t_rows = ["WWWWWWWW", "gggggggg", "bbbbbbbb", "aaaaaaaa"] + ["iiiiiiii"] * 4
    edge_b_rows = ["iiiiiiii"] * 4 + ["aaaaaaaa", "bbbbbbbb", "gggggggg", "WWWWWWWW"]

    tiles = []
    tiles.append(pack_tile(corner_t_rows, leg(shades[0])))          # +0 corner TL
    tiles.append(pack_tile(corner_b_rows, leg(shades[4])))          # +1 corner BL
    tiles.append(pack_tile(edge_t_rows, leg(shades[0])))            # +2 top edge
    tiles.append(pack_tile(edge_b_rows, leg(shades[4])))            # +3 bottom edge
    for s in shades:                                                # +4..8 left edges
        tiles.append(pack_tile(["Wgbaiiii"] * 8, leg(s)))
    for s in shades:                                                # +9..13 interiors
        tiles.append(pack_tile(["iiiiiiii"] * 8, leg(s)))
    tiles.append(pack_tile(["bbbbbbbb"] * 8, {"b": 10}))            # +14 solid black
    tiles.append(pack_tile(["WWWWWWWW"] * 8, {"W": 8}))             # +15 solid white

    # +16..24: ATB gauge fills 0..8 px (silver), +25: full gold
    def gauge(fill, color):
        rows = ["........", "gggggggg"]
        for _ in range(3):
            rows.append(color * fill + "t" * (8 - fill))
        rows += ["gggggggg", "........", "........"]
        rows = rows[:8]
        return pack_tile(rows, {".": 0, "g": 9, "t": 6, "s": 13, "y": 12})
    for f in range(9):
        tiles.append(gauge(f, "s"))
    tiles.append(gauge(8, "y"))
    return tiles

# ---------------------------------------------------------------- sprites

def sprite_tiles(rows, legend, w_tiles, h_tiles):
    """Cut a sprite image into 1D-mapping OBJ tile order (row-major)."""
    return grid_tiles(rows, legend, w_tiles, h_tiles)

HAND = [
    "................",
    "................",
    "................",
    "..bb............",
    ".bWWb...bbbb....",
    ".bWWb.bbWWWWbbb.",
    ".bWWbbWWWWWWWWWb",
    ".bWWbWWWWWWWWWWb",
    ".bWWbWWbbbbbbbb.",
    ".bWWWWWWWWWb....",
    ".bWWWWWWWWWWb...",
    "..bWWWWWWggWb...",
    "..bWWWWWWWgb....",
    "...bbbbbbbb.....",
    "................",
    "................",
]
SPR_LEG = {".": 0, "W": 1, "g": 2, "d": 3, "b": 4}

# Polyhedral dice, 16x16: f face, o outline, s shade. Tinted per damage
# type by OBJ palette; rolled values overdraw as digit sprites.
DICE_LEG = {".": 0, "f": 4, "o": 5, "s": 6}
DICE = {
    "d4": [
        "................",
        ".......oo.......",
        "......offo......",
        "......offo......",
        ".....offffo.....",
        ".....offffo.....",
        "....offffffo....",
        "....offffsso....",
        "...offffffsso...",
        "...offffffsso...",
        "..offffffffsso..",
        "..offffffffsso..",
        ".offffffffffsso.",
        ".osssssssssssso.",
        "..oooooooooooo..",
        "................",
    ],
    "d6": [
        "................",
        "..oooooooooooo..",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffsso.",
        ".offffffffffsso.",
        ".offffffffsssso.",
        ".offsssssssssso.",
        ".osssssssssssso.",
        "..oooooooooooo..",
        "................",
    ],
    "d8": [
        "................",
        ".......oo.......",
        "......offo......",
        ".....offffo.....",
        "....offffffo....",
        "...offffffffo...",
        "..offffffffffo..",
        ".offffffffffffo.",
        ".offffffffffsso.",
        "..offffffffsso..",
        "...offffffsso...",
        "....offfssso....",
        ".....ofssso.....",
        "......osso......",
        ".......oo.......",
        "................",
    ],
    "d10": [
        "................",
        ".......oo.......",
        "......offo......",
        ".....offffo.....",
        "....offffffo....",
        "...offffffffo...",
        "..offffffffffo..",
        ".offffffffffffo.",
        ".offffffffffsso.",
        "..offffffffsso..",
        "..offffffffsso..",
        "...offffffsso...",
        "...offffffsso...",
        "....osssssso....",
        ".....oooooo.....",
        "................",
    ],
    "d12": [
        "................",
        ".......oo.......",
        ".....offffo.....",
        "....offffffo....",
        "...offffffffo...",
        "..offffffffffo..",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffsso.",
        ".offffffffffsso.",
        "..offffffffsso..",
        "..offffffffsso..",
        "...offffssssо...".replace("о", "o"),
        "....osssssso....",
        ".....oooooo.....",
        "................",
    ],
    "d20": [
        "................",
        ".......oo.......",
        ".....offffo.....",
        "....offffffo....",
        "...offffffffo...",
        "..offffffffffo..",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffffo.",
        ".offffffffffsso.",
        ".offffffffffsso.",
        "..offffffssssо..".replace("о", "o"),
        "...offffsssso...",
        "....osssssso....",
        ".....oooooo.....",
        "................",
    ],
}

def obj_tiles():
    tiles = []
    defs = {}
    pals = []   # OBJ palette index per 4-tile (16x16) group, for the preview
    defs["OBJT_HAND"] = len(tiles)
    tiles += sprite_tiles(HAND, SPR_LEG, 2, 2)
    pals.append(7)
    # dice are tinted per damage type at runtime; preview one die per ramp
    for name, pal in [("d4", 11), ("d6", 10), ("d8", 12),
                      ("d10", 13), ("d12", 14), ("d20", 9)]:
        defs["OBJT_DIE_" + name.upper()] = len(tiles)
        tiles += sprite_tiles(DICE[name], DICE_LEG, 2, 2)
        pals.append(pal)
    # tether dot (tile +0) and sneak pip (tile +1), drawn as 8x8 objs
    GARN = ["........" "........",
            "..ggg..." "...WW...",
            ".gWWWg.." "..WWWW..",
            ".gWWWg.." "..WWWW..",
            ".gWWWg.." "...WW...",
            "..ggg..." "........",
            "........" "........",
            "........" "........"] + ["................"] * 8
    defs["OBJT_GARN"] = len(tiles)
    tiles += sprite_tiles(GARN, SPR_LEG, 2, 2)
    pals.append(10)
    return tiles, defs, pals

# ---------------------------------------------------------------- field art

from art import field_tiles as FT
from art import sprites_field as SF
from art import maps as MP

def build_field():
    """Dedupe 8x8 tiles across metatiles; build metatile LUT + solid flags."""
    tiles, index = [], {}

    def add(t):
        k = tuple(t)
        if k not in index:
            index[k] = len(tiles)
            tiles.append(t)
        return index[k]

    lut, solid, mt_defs = [], [], {}
    for i, name in enumerate(FT.ORDER):
        ts = grid_tiles(FT.METATILES[name], FT.LEGEND, 2, 2)
        lut += [add(t) | (3 << 12) for t in ts]      # palette 3 baked in
        solid.append(1 if name in FT.SOLID else 0)
        mt_defs["MT_" + name.upper()] = i
    return tiles, lut, solid, mt_defs

def build_maps():
    name_to_id = {n: i for i, n in enumerate(FT.ORDER)}
    out = {}
    for mname, grid in MP.MAPS.items():
        w, h = len(grid[0]), len(grid)
        cells = []
        for row in grid:
            assert len(row) == w, (mname, row)
            cells += [name_to_id[MP.MAPLEG[c]] for c in row]
        out[mname] = (w, h, cells)
    return out

from art import sprites_battle as SB
from art import portraits as PT
from music import songs as MUSIC

def build_portraits():
    tiles, defs = [], {}
    BG_PALS[5] = pal16(PT.PAL)
    for i, (name, rows) in enumerate(PT.PORTRAITS.items()):
        try:
            t = grid_tiles(rows, PT.LEG, 6, 6)
        except (KeyError, AssertionError) as e:
            print(f"  skip portrait {name}: {e}")
            continue
        defs["POR_" + name.upper()] = len(tiles) // 36
        tiles += t
    return tiles, defs

SIZEMAP = {(2, 2): 1, (2, 4): 7, (4, 4): 2, (4, 2): 5, (4, 8): 8}

def build_sprites():
    """OBJ tiles: UI sprites, field sprites, then battle sprites.
    grouppals records the OBJ palette index of every 4-tile (16x16) group as
    the stream is built, so previews can't drift from the real tile order."""
    tiles, defs, grouppals = obj_tiles()
    for sname, spec in SF.SPRITES.items():
        defs["OBJT_" + sname.upper()] = len(tiles)
        for fname in spec["frames"]:
            tiles += sprite_tiles(SF.FRAMES[fname], SF.LEG, 2, 2)
            grouppals.append(spec["pal"])
    for sname, spec in SB.SPRITES.items():
        w, hgt = spec["w"], spec["h"]
        try:                                    # resilient to mid-edit sprite files
            frames = [sprite_tiles(SB.FRAMES[f], SB.LEG, w, hgt) for f in spec["frames"]]
        except (KeyError, AssertionError) as e:
            print(f"  skip {sname}: {e}")
            continue
        up = sname.upper()
        defs["OBJT_" + up] = len(tiles)
        defs["OBJS_" + up] = SIZEMAP[(w, hgt)]
        defs["OBJP_" + up] = spec["pal"]
        defs["OBJTPF_" + up] = w * hgt
        for ft in frames:
            tiles += ft
            grouppals += [spec["pal"]] * (len(ft) // 4)
    for idx, colors in SF.PALS.items():
        OBJ_PALS[idx] = pal16(colors)
    assert len(grouppals) == len(tiles) // 4
    return tiles, defs, grouppals

# Digit/dice palettes: [transparent, face/digit, outline/shadow, face shade].
# Damage types tint both the popup numbers and the dice sprites.
def dmgpal(vivid):
    r, g, b = vivid
    face = (r * 5 // 9, g * 5 // 9, b * 5 // 9)          # darkened for contrast
    shade = (r * 3 // 9, g * 3 // 9, b * 3 // 9)
    return pal16([(0, 0, 0), vivid, (3, 3, 4), (0, 0, 0), face, (2, 2, 3), shade])

OBJ_PALS[8] = dmgpal((10, 29, 12))    # heal
OBJ_PALS[9] = dmgpal((31, 28, 8))     # radiant/gold
OBJ_PALS[10] = dmgpal((30, 30, 31))   # physical: grey die, white numbers
OBJ_PALS[11] = dmgpal((31, 12, 5))    # fire
OBJ_PALS[12] = dmgpal((13, 27, 8))    # poison
OBJ_PALS[13] = dmgpal((20, 12, 31))   # force
OBJ_PALS[14] = dmgpal((30, 13, 26))   # psychic
OBJ_PALS[15] = dmgpal((13, 22, 31))   # cold/lightning

def sky_tiles():
    """8 simple Avernus sky tiles: 2 variants x 4 gradient bands, BG pal 4."""
    import random
    rng = random.Random(7)
    tiles = []
    for variant in range(2):
        for band in range(4):
            rows = []
            for y in range(8):
                row = ""
                for x in range(8):
                    c = band + 1
                    r = rng.random()
                    if r < 0.06 and band < 3:
                        c += 1          # ember speck
                    elif r > 0.96 and band > 0:
                        c -= 1          # dark wisp
                    row += "%X" % c
                rows.append(row)
            tiles.append(pack_tile(rows, {"%X" % i: i for i in range(16)}))
    return tiles

BG_PALS[3] = pal16(FT.PAL)
BG_PALS[4] = pal16([
    (0, 0, 0), (7, 1, 2), (13, 3, 2), (21, 6, 2), (29, 11, 3),
    (31, 18, 6), (31, 25, 12),
])

# ---------------------------------------------------------------- screens

def gen_screens():
    """Build-time constrained layout: measure, center, assert, emit C."""
    import ui_screens as UI

    def width_of(it):
        k = it[0]
        if k == "l": return len(it[1])
        if k == "s": return it[2]
        if k == "g": return it[1]
        if k == "m": return 0
        raise AssertionError(it)

    H = ["#ifndef SCREENS_H", "#define SCREENS_H"]
    C = ['#include "gba.h"', '#include "engine.h"', '#include "screens.h"', ""]

    for scr in UI.SCREENS:
        sname = scr["name"].upper()
        fn = [f"void scr_{scr['name']}(void) {{"]
        rects = []

        def place_items(pfx, items, al, cx0, avail_r, ry):
            cx = cx0
            n = len(items)
            for j, it in enumerate(items):
                if al == "sp" and j == n - 1:
                    cx = avail_r - width_of(it)
                k = it[0]
                if k == "l":
                    txt = it[1].replace('"', '\\"')
                    fn.append(f'  txt_put({cx}, {ry}, "{txt}", {it[2]});')
                elif k == "s":
                    up = f"{pfx}_{it[1].upper()}"
                    H.append(f"#define {up}_X {cx}")
                    H.append(f"#define {up}_Y {ry}")
                    H.append(f"#define {up}_W {it[2]}")
                elif k == "m":
                    up = f"{pfx}_{it[1].upper()}"
                    H.append(f"#define {up}_X {cx}")
                    H.append(f"#define {up}_Y {ry}")
                cx += width_of(it)

        for win in scr["wins"]:
            pad = win.get("pad", 1)
            rows = win["rows"]
            interior = win.get("minw", 0)
            for r in rows:
                if r[0] == "row":
                    interior = max(interior, sum(width_of(i) for i in r[2]))
            W = interior + 2 + 2 * pad
            Hh = len(rows) + 2
            x = win.get("x", (30 - W) // 2)
            y = win["y"]
            wid = win["id"].upper()
            assert x >= 0 and x + W <= 30 and y >= 0 and y + Hh <= 20, \
                f"{scr['name']}.{win['id']}: rect ({x},{y} {W}x{Hh}) off-screen"
            for (oid, ox, oy, ow, oh) in rects:
                assert x + W <= ox or ox + ow <= x or y + Hh <= oy or oy + oh <= y, \
                    f"{scr['name']}: {win['id']} overlaps {oid}"
            rects.append((win["id"], x, y, W, Hh))
            pfx = f"SCR_{sname}_{wid}"
            for d, v in (("X", x), ("Y", y), ("W", W), ("H", Hh)):
                H.append(f"#define {pfx}_{d} {v}")
            fn.append(f"  win_draw({x}, {y}, {W}, {Hh});")
            for i, r in enumerate(rows):
                if r[0] != "row":
                    continue
                _, al, items = r
                rw = sum(width_of(i2) for i2 in items)
                assert rw <= interior, \
                    f"{scr['name']}.{win['id']} row {i}: {rw} > interior {interior}"
                ry = y + 1 + i
                if al == "c":
                    cx = x + (W - rw) // 2
                elif al == "r":
                    cx = x + W - 1 - pad - rw
                else:
                    cx = x + 1 + pad
                place_items(f"SCR_{sname}", items, al, cx, x + W - 1 - pad, ry)

        for fl in scr.get("floats", []):
            items = fl["items"]
            rw = sum(width_of(i2) for i2 in items)
            al = fl.get("align", "c")
            fy = fl["y"]
            assert rw <= 30 and 0 <= fy < 20, f"{scr['name']} float row {fy}"
            cx = (30 - rw) // 2 if al == "c" else fl.get("x", 0)
            place_items(f"SCR_{sname}", items, al, cx, 30, fy)

        fn.append("}")
        C += fn + [""]
        H.append(f"void scr_{scr['name']}(void);")

    seen = set()
    for line in H:
        if line.startswith("#define"):
            key = line.split()[1]
            assert key not in seen, f"duplicate screen define {key}"
            seen.add(key)
    H.append("#endif")
    return H, C

# ---------------------------------------------------------------- emit

def emit_u16(name, vals):
    lines = [f"const u16 {name}[{len(vals)}] = {{"]
    for i in range(0, len(vals), 12):
        lines.append("  " + ",".join(f"0x{v:04x}" for v in vals[i:i + 12]) + ",")
    lines.append("};")
    return "\n".join(lines)

def main():
    preview = "--preview" in sys.argv
    os.makedirs(OUT, exist_ok=True)

    fonts = font_tiles()
    wins = window_tiles()
    ui = fonts + wins
    flat = [hw for t in ui for hw in t]

    objs, objdefs, objpals = build_sprites()
    objflat = [hw for t in objs for hw in t]

    ftiles, flut, fsolid, mtdefs = build_field()
    fflat = [hw for t in ftiles for hw in t]
    maps = build_maps()
    sky = sky_tiles()
    skyflat = [hw for t in sky for hw in t]
    ptiles, pdefs = build_portraits()
    pflat = [hw for t in ptiles for hw in t]

    defs = {
        "TILE_WIN": len(fonts),          # window tile block base
        "UI_TILE_COUNT": len(ui),
        "OBJ_TILE_COUNT": len(objs),
        "FIELD_TILE_COUNT": len(ftiles),
        "SKY_TILE_COUNT": len(sky),
        "METATILE_COUNT": len(fsolid),
        "PORTRAIT_COUNT": len(ptiles) // 36,
    }
    defs.update(objdefs)
    defs.update(mtdefs)
    defs.update(pdefs)

    h = ["#ifndef ASSETS_H", "#define ASSETS_H", '#include "gba.h"',
         '#include "audio.h"', ""]
    c = ['#include "assets.h"', ""]
    for k, v in defs.items():
        h.append(f"#define {k} {v}")
    h += [
        "extern const u16 ui_tiles[%d];" % len(flat),
        "extern const u16 obj_tiles_gfx[%d];" % len(objflat),
        "extern const u16 field_tiles_gfx[%d];" % len(fflat),
        "extern const u16 sky_tiles_gfx[%d];" % len(skyflat),
        "extern const u16 metatile_lut[%d];" % len(flut),
        "extern const u8 metatile_solid[%d];" % len(fsolid),
        "extern const u16 pal_bg[256];",
        "extern const u16 pal_obj[256];",
        "extern const u16 pal_field_night[16];",
        "extern const u16 pal_tav_classes[4][16];",
    ]
    if pflat:
        h.append("extern const u16 portrait_tiles[%d];" % len(pflat))
        c.append(emit_u16("portrait_tiles", pflat))
    c.append(emit_u16("ui_tiles", flat))
    c.append(emit_u16("obj_tiles_gfx", objflat))
    c.append(emit_u16("field_tiles_gfx", fflat))
    c.append(emit_u16("sky_tiles_gfx", skyflat))
    c.append(emit_u16("metatile_lut", flut))
    c.append("const u8 metatile_solid[%d] = {%s};"
             % (len(fsolid), ",".join(map(str, fsolid))))
    c.append(emit_u16("pal_bg", [v for p in BG_PALS for v in p]))
    c.append(emit_u16("pal_obj", [v for p in OBJ_PALS for v in p]))
    # moonlight over the field family: the camp room swaps this over BG
    # palette 3 at room_enter (and every other room restores the daylight)
    c.append(emit_u16("pal_field_night", pal16(FT.NIGHT_PAL)))

    tavorder = ["bard", "rogue", "ranger", "wizard"]
    tavflat = [v for cls in tavorder for v in pal16(SF.PAL_TAV[cls])]
    c.append("const u16 pal_tav_classes[4][16] = {")
    for i in range(4):
        c.append("  {" + ",".join("0x%04x" % v for v in tavflat[i*16:(i+1)*16]) + "},")
    c.append("};")

    for mname, (w, hgt, cells) in maps.items():
        up = mname.upper()
        h.append(f"#define MAP_{up}_W {w}")
        h.append(f"#define MAP_{up}_H {hgt}")
        h.append(f"extern const u8 map_{mname}[{w * hgt}];")
        c.append("const u8 map_%s[%d] = {%s};" % (mname, w * hgt, ",".join(map(str, cells))))

    # --- music ---
    c.append('#include "audio.h"')
    for i, name in enumerate(MUSIC.ORDER):
        h.append(f"#define SONG_{name} {i}")
        s = MUSIC.SONGS[name]
        rows = len(s["ch"][0])
        for ci, chan in enumerate(s["ch"]):
            assert len(chan) == rows, (name, ci, len(chan), rows)
            c.append("static const u8 song_%s_%d[%d] = {%s};"
                     % (name, ci, rows, ",".join(str(v) for v in chan)))
    h.append(f"#define SONG_COUNT {len(MUSIC.ORDER)}")
    h.append("extern const Song songs[SONG_COUNT];")
    # jukebox display names, single-sourced from the song registry
    assert set(MUSIC.ORDER) == set(MUSIC.TITLES), "songs ORDER vs TITLES mismatch"
    h.append("extern const char* const song_names[SONG_COUNT];")
    c.append("const char* const song_names[%d] = {%s};"
             % (len(MUSIC.ORDER),
                ",".join('"%s"' % MUSIC.TITLES[n] for n in MUSIC.ORDER)))
    c.append("const Song songs[%d] = {" % len(MUSIC.ORDER))
    for name in MUSIC.ORDER:
        s = MUSIC.SONGS[name]
        rows = len(s["ch"][0])
        c.append("  { {song_%s_0,song_%s_1,song_%s_2,song_%s_3}, %d, 0x%04x, %d, 0x%04x, 0x%04x, 0x%04x },"
                 % (name, name, name, name, rows, s["loop"], s["speed"],
                    s["env1"], s["env2"], s["wavevol"]))
    c.append("};")

    h.append("#endif")

    with open(os.path.join(OUT, "assets.h"), "w") as f:
        f.write("\n".join(h) + "\n")
    with open(os.path.join(OUT, "assets.c"), "w") as f:
        f.write("\n".join(c) + "\n")

    sh, sc = gen_screens()
    trks = sum(1 for line in sh
               if line.startswith("#define SCR_JUKEBOX_TRK") and line.split()[1].endswith("_X"))
    assert len(MUSIC.ORDER) <= trks, \
        f"jukebox screen has {trks} track slots but there are {len(MUSIC.ORDER)} songs"
    with open(os.path.join(OUT, "screens.h"), "w") as f:
        f.write("\n".join(sh) + "\n")
    with open(os.path.join(OUT, "screens.c"), "w") as f:
        f.write("\n".join(sc) + "\n")
    print(f"assets: {len(ui)} ui, {len(objs)} obj, {len(ftiles)} field tiles, {len(maps)} maps")

    if preview:
        render_preview(ui)
        render_field_preview(ftiles, flut)
        render_sprite_preview(objs, objpals)
        render_portrait_preview(ptiles)

# ---------------------------------------------------------------- preview

def tile_px(t):
    """16 halfwords -> 8x8 palette indices"""
    px = [[0] * 8 for _ in range(8)]
    for y in range(8):
        for half in range(2):
            hw = t[y * 2 + half]
            for i in range(4):
                px[y][half * 4 + i] = (hw >> (4 * i)) & 15
    return px

def sheet(tiles, pal255, cols, name, scale=3):
    from pnglib import write_png_scaled
    rows = (len(tiles) + cols - 1) // cols
    w, h = cols * 9, rows * 9
    img = bytearray(w * h * 3)
    for i, t in enumerate(tiles):
        px = tile_px(t)
        ox, oy = (i % cols) * 9, (i // cols) * 9
        for y in range(8):
            for x in range(8):
                r, g, b = pal255[px[y][x]] if px[y][x] else (40, 24, 40)
                off = ((oy + y) * w + ox + x) * 3
                img[off:off + 3] = bytes((r, g, b))
    out = os.path.join(ROOT, "test", "shots", name)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    write_png_scaled(out, w, h, img, scale)
    print(f"preview: {out}")

def p255(pal):
    return [(r * 255 // 31, g * 255 // 31, b * 255 // 31) for (r, g, b) in pal]

def render_preview(tiles):
    sheet(tiles, p255(UI_PAL), 16, "preview_ui.png")

def render_field_preview(tiles, lut):
    """Draw each metatile assembled (2x2), by day and by camp night."""
    for palette, fname in ((FT.PAL, "preview_field.png"),
                           (FT.NIGHT_PAL, "preview_field_night.png")):
        _field_sheet(tiles, lut, palette, fname)

def _field_sheet(tiles, lut, palette, fname):
    from pnglib import write_png_scaled
    pal = p255(palette)
    n = len(lut) // 4
    cols = 8
    rows = (n + cols - 1) // cols
    w, h = cols * 17, rows * 17
    img = bytearray(w * h * 3)
    for m in range(n):
        ox, oy = (m % cols) * 17, (m // cols) * 17
        for q in range(4):
            t = tiles[lut[m * 4 + q] & 0x3FF]
            px = tile_px(t)
            qx, qy = ox + (q % 2) * 8, oy + (q // 2) * 8
            for y in range(8):
                for x in range(8):
                    c = pal[px[y][x]] if px[y][x] else (30, 18, 30)
                    off = ((qy + y) * w + qx + x) * 3
                    img[off:off + 3] = bytes(c)
    out = os.path.join(ROOT, "test", "shots", fname)
    write_png_scaled(out, w, h, img, 3)
    print(f"preview: {out}")

def render_portrait_preview(tiles):
    """Each portrait 6x6 tiles, drawn over a window-blue background."""
    from pnglib import write_png_scaled
    n = len(tiles) // 36
    if not n:
        return
    pal = p255(PT.PAL)
    blue = (48, 48, 200)
    cols = 4
    rows = (n + cols - 1) // cols
    w, h = cols * 50, rows * 50
    img = bytearray()
    for _ in range(w * h):
        img += bytes(blue)
    for p in range(n):
        ox, oy = (p % cols) * 50 + 1, (p // cols) * 50 + 1
        for q in range(36):
            px = tile_px(tiles[p * 36 + q])
            qx, qy = ox + (q % 6) * 8, oy + (q // 6) * 8
            for y in range(8):
                for x in range(8):
                    c = px[y][x]
                    if c:
                        off = ((qy + y) * w + qx + x) * 3
                        img[off:off + 3] = bytes(pal[c])
    out = os.path.join(ROOT, "test", "shots", "preview_portraits.png")
    write_png_scaled(out, w, h, img, 3)
    print(f"preview: {out}")

def render_sprite_preview(tiles, grouppals):
    """16x16 cells, 4 tiles each; palette per group recorded by build_sprites.
    OBJ pal 0 is copied in at runtime (class palette), so preview it as bard."""
    from pnglib import write_png_scaled
    groups = len(tiles) // 4
    cols = 8
    rows = (groups + cols - 1) // cols
    w, h = cols * 17, rows * 17
    img = bytearray(b"\x28\x18\x28" * (w * h))
    for g in range(groups):
        colors = OBJ_PALS[grouppals[g]] if grouppals[g] else pal16(SF.PAL_TAV["bard"])
        pal = [((v & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, ((v >> 10) & 31) * 255 // 31)
               for v in colors]
        ox, oy = (g % cols) * 17, (g // cols) * 17
        for q in range(4):
            px = tile_px(tiles[g * 4 + q])
            qx, qy = ox + (q % 2) * 8, oy + (q // 2) * 8
            for y in range(8):
                for x in range(8):
                    if px[y][x]:
                        c = pal[px[y][x]]
                        off = ((qy + y) * w + qx + x) * 3
                        img[off:off + 3] = bytes(c)
    out = os.path.join(ROOT, "test", "shots", "preview_sprites.png")
    write_png_scaled(out, w, h, img, 3)
    print(f"preview: {out}")

if __name__ == "__main__":
    main()
