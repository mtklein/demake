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
                row("l", g(2), s("trk6", 15)),
                row("l", g(2), s("trk7", 15)),
                row("l", g(2), s("trk8", 15)),
                row("l", g(2), s("trk9", 15)),
                row("l", g(2), s("trk10", 15)),
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
            {"id": "list", "x": 0, "y": 5, "rows": [
                row("l", g(2), s("c%d" % i, 9)) for i in range(12)
            ]},
            {"id": "blurb", "x": 15, "y": 6, "rows": [
                row("l", s("b0", 11)),
                row("l", s("b1", 11)),
                row("l", s("b2", 11)),
                row("l", s("b3", 11)),
                BLANK,
                row("c", m("hero")),
                BLANK,
            ]},
        ],
    },
    {
        # race picker: base-race list left, name+ASI card right, blurb
        # strip below. The subrace step redraws the same list slots.
        "name": "racesel",
        "wins": [
            {"id": "list", "x": 0, "y": 2, "rows": [
                row("l", g(2), s("r%d" % i, 10)) for i in range(10)
            ]},
            {"id": "card", "x": 16, "y": 2, "rows": [
                row("l", s("rname", 10)),
                BLANK,
                row("l", l("Scores:", 2)),
                row("l", s("rasi0", 7)),
                row("l", s("rasi1", 7)),
                row("l", s("rasi2", 7)),
            ]},
            {"id": "blurb", "y": 14, "rows": [
                row("l", s("rb0", 26)),
                row("l", s("rb1", 26)),
                row("l", s("rb2", 26)),
            ]},
        ],
        "floats": [
            {"y": 0, "align": "c", "items": [l("Whose blood wakes here?", 0)]},
        ],
    },
    {
        # background picker: list left; the strip below names the two
        # granted skills and carries the flavor line
        "name": "bgsel",
        "wins": [
            {"id": "list", "x": 0, "y": 2, "rows": [
                row("l", g(2), s("g%d" % i, 11)) for i in range(10)
            ]},
            {"id": "blurb", "y": 14, "rows": [
                row("l", s("gsk", 26)),
                row("l", s("gb0", 26)),
                row("l", s("gb1", 26)),
                row("l", s("gb2", 26)),
            ]},
        ],
        "floats": [
            {"y": 0, "align": "c", "items": [l("And before all this?", 0)]},
        ],
    },
    {
        # standard-array assignment: six score rows + a Begin row; the
        # D-pad swap UI fills the a* slots live (base, ASI, total, mod)
        "name": "statsel",
        "wins": [
            {"id": "arr", "y": 2, "rows": [
                row("l", g(2), s("a0", 20)),
                row("l", g(2), s("a1", 20)),
                row("l", g(2), s("a2", 20)),
                row("l", g(2), s("a3", 20)),
                row("l", g(2), s("a4", 20)),
                row("l", g(2), s("a5", 20)),
                BLANK,
                row("l", g(2), s("go", 12)),
            ]},
        ],
        "floats": [
            {"y": 0, "align": "c", "items": [l("Arrange your scores", 0)]},
            {"y": 14, "align": "c", "items": [l("A: take/swap   B: back", 2)]},
        ],
    },
    {
        "name": "namehdr",
        "wins": [
            {"id": "hdr", "y": 1, "rows": [
                row("c", l("Name this soul:", 0), g(1), s("name", 6)),
            ]},
            {"id": "gridbox", "x": 1, "y": 6, "minw": 25, "rows": [
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
        # Dormant since the beach arc's stone 2: the crash no longer ends the
        # game, so nothing calls scr_tally today. The accounting returns at
        # the grove-gates finale (stone 6) -- keep the layout warm for it.
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
