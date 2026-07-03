# Battle sprites. Sizes in 8px tiles: party 2x4 (16x32, figure in lower ~24px),
# small enemies 2x2 (16x16), medium 4x4 (32x32), wide 4x2 (32x16),
# boss Zhalk 4x8 (32x64). Frames listed per sprite; art rows are h*8 strings
# of w*8 chars.
#
# Shared legend (palette indices — see sprites_field.py pal() layout):
# . transparent, 1 outline, 2 skin, 3 skin shadow, 4 hair, 5 outfit main,
# 6 outfit shade, 7 trim/accent, 8 legs/secondary, 9 boots/dark, W white/eyes
#
# Party frames face LEFT (toward enemies). Poses per party member:
#   idle, swing1 (windup), swing2 (strike), cast, hurt, ko, victory
# Enemy frames face RIGHT. Poses: idle, act (attack/lunge).

LEG = {".": 0, "1": 1, "2": 2, "3": 3, "4": 4, "5": 5, "6": 6, "7": 7,
       "8": 8, "9": 9, "W": 10}

# name -> {pal, w, h, frames: [framename...]}
SPRITES = {
    "b_imp": {"pal": 4, "w": 2, "h": 2, "frames": ["b_imp0", "b_imp1"]},
}

FRAMES = {
    # lesser imp: bat-winged pest, facing right
    "b_imp0": [
        "...1........1...",
        "..131......131..",
        ".13731....13731.",
        ".1377311113731..",
        "..137222222731..",
        "...12222222221..",
        "...122W22W2221..",
        "...12222222221..",
        "....122333221...",
        ".....1222221....",
        "....122222221...",
        "...12212212221..",
        "...121..121121..",
        "...11....11.11..",
        "................",
        "................",
    ],
    "b_imp1": [
        "................",
        "..1........1....",
        ".131.......131..",
        ".1731111111371..",
        ".1372222222731..",
        "..1222222222 1..".replace(" ", "2"),
        "...122W22W221...",
        "...12222222221..",
        "....122333221...",
        ".....1222221....",
        "....122222221...",
        "...12212212221..",
        "....121121.121..",
        "....11.11...11..",
        "................",
        "................",
    ],
}
