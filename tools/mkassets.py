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

def obj_tiles():
    tiles = []
    defs = {}
    defs["OBJT_HAND"] = len(tiles)
    tiles += sprite_tiles(HAND, SPR_LEG, 2, 2)
    return tiles, defs

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

    objs, objdefs = obj_tiles()
    objflat = [hw for t in objs for hw in t]

    defs = {
        "TILE_WIN": len(fonts),          # window tile block base
        "UI_TILE_COUNT": len(ui),
        "OBJ_TILE_COUNT": len(objs),
    }
    defs.update(objdefs)

    h = ["#ifndef ASSETS_H", "#define ASSETS_H", '#include "gba.h"', ""]
    c = ['#include "assets.h"', ""]
    for k, v in defs.items():
        h.append(f"#define {k} {v}")
    h += [
        "extern const u16 ui_tiles[%d];" % len(flat),
        "extern const u16 obj_tiles_gfx[%d];" % len(objflat),
        "extern const u16 pal_bg[256];",
        "extern const u16 pal_obj[256];",
    ]
    c.append(emit_u16("ui_tiles", flat))
    c.append(emit_u16("obj_tiles_gfx", objflat))
    c.append(emit_u16("pal_bg", [v for p in BG_PALS for v in p]))
    c.append(emit_u16("pal_obj", [v for p in OBJ_PALS for v in p]))
    h.append("#endif")

    with open(os.path.join(OUT, "assets.h"), "w") as f:
        f.write("\n".join(h) + "\n")
    with open(os.path.join(OUT, "assets.c"), "w") as f:
        f.write("\n".join(c) + "\n")
    print(f"assets: {len(ui)} ui tiles")

    if preview:
        render_preview(ui)

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

def render_preview(tiles):
    from pnglib import write_png_scaled
    pal = [(r * 255 // 31, g * 255 // 31, b * 255 // 31) for (r, g, b) in UI_PAL]
    cols = 16
    rows = (len(tiles) + cols - 1) // cols
    w, h = cols * 9, rows * 9
    img = bytearray(w * h * 3)
    for i, t in enumerate(tiles):
        px = tile_px(t)
        ox, oy = (i % cols) * 9, (i // cols) * 9
        for y in range(8):
            for x in range(8):
                r, g, b = pal[px[y][x]] if px[y][x] else (40, 24, 40)
                off = ((oy + y) * w + ox + x) * 3
                img[off:off + 3] = bytes((r, g, b))
    out = os.path.join(ROOT, "test", "shots", "preview_ui.png")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    write_png_scaled(out, w, h, img, 3)
    print(f"preview: {out}")

if __name__ == "__main__":
    main()
