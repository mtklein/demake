# Static screen layouts, measured and placed at BUILD TIME.
#
# The generator (mkassets.py) computes window sizes from content, centers
# anything without an explicit x, and hard-asserts: everything fits the
# 30x20 tile screen, rows fit their window interior, windows never overlap.
# Overflow is a build failure, not a bug report.
#
# Items:  ("l", text, pal)   static label (pal: 0 white, 1 yellow, 2 grey)
#         ("s", name, width) slot: reserved cells C fills at SCR_*_X/Y/W
#         ("g", n)           gap of n cells
#         ("m", name)        mark: zero-width position anchor -> SCR_*_X/Y
# Rows:   ("row", align, [items])  align: "l" left, "c" center, "r" right,
#                                  "sp" spread (first left, last right)
#         ("blank",)
# Window: {"id", "y", optional "x" (default centered), optional "minw",
#          optional "pad" (default 1), "rows": [...]}

def l(text, pal=0): return ("l", text, pal)
def s(name, width): return ("s", name, width)
def g(n): return ("g", n)
def m(name): return ("m", name)
def row(align, *items): return ("row", align, list(items))
BLANK = ("blank",)

SCREENS = [
    {
        "name": "title",
        "wins": [
            {"id": "box", "y": 5, "rows": [
                row("c", l("N A U T I L O I D", 1)),
                BLANK,
                row("c", l("a BG3 demake of the", 2)),
                row("c", l("mind flayer prologue", 2)),
            ]},
        ],
        "floats": [
            {"y": 14, "align": "c", "items": [s("start", 11)]},
            {"y": 17, "align": "c", "items": [l("SELECT: JUKEBOX", 2)]},
        ],
    },
    {
        "name": "jukebox",
        "wins": [
            {"id": "box", "y": 2, "rows": [
                row("c", l("-- JUKEBOX --", 1)),
                BLANK,
                row("l", g(2), s("trk0", 15)),
                row("l", g(2), s("trk1", 15)),
                row("l", g(2), s("trk2", 15)),
                row("l", g(2), s("trk3", 15)),
                row("l", g(2), s("trk4", 15)),
                row("l", g(2), s("trk5", 15)),
                BLANK,
                row("c", l("A:PLAY  B:BACK", 2)),
            ]},
        ],
    },
    {
        "name": "classsel",
        "wins": [
            {"id": "prompt", "y": 1, "rows": [
                row("c", l("Who were you, before", 0)),
                row("c", l("the nautiloid took you?", 0)),
            ]},
            {"id": "list", "x": 2, "y": 6, "rows": [
                row("l", g(2), s("c0", 6)),
                BLANK,
                row("l", g(2), s("c1", 6)),
                BLANK,
                row("l", g(2), s("c2", 6)),
                BLANK,
                row("l", g(2), s("c3", 6)),
            ]},
            {"id": "blurb", "x": 14, "y": 6, "rows": [
                row("l", s("b0", 12)),
                row("l", s("b1", 12)),
                row("l", s("b2", 12)),
                row("l", s("b3", 12)),
                BLANK,
                row("c", m("hero")),
                BLANK,
            ]},
        ],
    },
    {
        "name": "namehdr",
        "wins": [
            {"id": "hdr", "y": 1, "rows": [
                row("c", l("Name this soul:", 0), g(1), s("name", 6)),
            ]},
            {"id": "grid", "x": 1, "y": 6, "minw": 25, "rows": [
                BLANK,
                row("l", m("grid")),
                BLANK,
                BLANK,
                BLANK,
                row("l", m("end")),
                BLANK,
            ]},
        ],
    },
    {
        "name": "tally",
        "wins": [
            {"id": "box", "y": 2, "rows": [
                row("c", l("ESCAPE THE NAUTILOID", 1)),
                BLANK,
                row("sp", s("who", 8), g(2), s("cls", 8)),
                BLANK,
                row("sp", l("Us freed", 0), g(2), s("v_us", 6)),
                row("sp", l("Lae'zel", 0), g(2), s("v_lz", 6)),
                row("sp", l("Shadowheart", 0), g(2), s("v_sh", 6)),
                row("sp", l("Cmdr Zhalk", 0), g(2), s("v_zh", 6)),
                row("sp", l("Everburn Blade", 0), g(2), s("v_eb", 6)),
                BLANK,
                row("c", l("THE ADVENTURE BEGINS", 0)),
                row("c", l("...IN BALDUR'S GATE", 0)),
                BLANK,
                row("c", l("PRESS START", 2)),
            ]},
        ],
    },
]
