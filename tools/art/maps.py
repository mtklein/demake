# Room layouts: ASCII grids of metatiles (see field_tiles.ORDER for names).

MAPLEG = {
    "#": "wall_f", ".": "floor_a", ",": "floor_v", "O": "pod_c", "o": "pod_o",
    "L": "pool", "R": "resto", "C": "console", "K": "console_lit",
    "D": "door_c", "d": "door_o", "c": "corpse", "W": "window", "X": "void",
    "T": "chest",
    "M": "myrnath", "m": "myrnath_empty", "[": "transp_l", "]": "transp_r",
    "%": "mesh", "g": "gith_corpse", "f": "flayer_corpse", "t": "tank",
    "h": "helm_floor", "V": "viewport",
    "s": "sand", "S": "sand_d", "u": "surf", "U": "sea", "k": "wreck",
}

MAPS = {
    # Room 1: the Nursery — ring of pods, central larva pool, resto device.
    "nursery": [
        "##WW####W####W#",
        "#..O.O.,.O.O..#",
        "#.............#",
        "#.....LLL.....#",
        "#..,..LLL..,..#",
        "#..O......c...#",
        "#R............#",
        "#....,...T....#",
        "#.............#",
        "#######D#######",
    ],
    # Room 2: Surgery — Myrnath strapped to the neural apparatus.
    "surgery": [
        "###W#####W#####",
        "#.............#",
        "#......M......#",
        "#.....,.,.....#",
        "#..c..........#",
        "#..........T..#",
        "#R............#",
        "#.............#",
        "#.....%.%.....#",
        "#######D#######",
    ],
    # Room 3: exterior deck — open sky above, burnt githyanki, Lae'zel + imps.
    "deck": [
        "WWWWWWWWWWWWWWWWWWWW",
        "#....g.....g.......#",
        "#..................#",
        "#..................#",
        "#..................#",
        "#..,....,......,...#",
        "#..g........g......#",
        "#..................#",
        "#..............R...#",
        "#.........%........#",
        "##########D#########",
    ],
    # Room 4: ceremorphosis chamber — Shadowheart's pod, consoles, rune thrall.
    "pods": [
        "##W####W####W#####",
        "#................#",
        "#.O.O.O.O........#",
        "#................#",
        "#....C....C......#",
        "#...........c..T.#",
        "#.O.O.O..........#",
        "#..f...........c.#",
        "#R...............#",
        "#.......%........#",
        "########D#########",
    ],
    # Room 5: the Helm — grand viewport, tanks, transponder at west wall.
    "helm": [
        "#VV####VV####VV###",
        "#................#",
        "#.......h........#",
        "#..h...........t.#",
        "#................#",
        "#[]......h.......#",
        "#.....h....h.....#",
        "#.t..............#",
        "#........h.......#",
        "#.......R........#",
        "#................#",
        "##################",
    ],
    # The Ravaged Beach — crash-site epilogue.
    "beach": [
        "ssssssSsssssssssSs",
        "ssSsssssskssssssss",
        "ssssssssssssssSsss",
        "sskssSssssssssssss",
        "ssssssssssSsssksss",
        "sssSssssssssssssSs",
        "sSssssssssssSsssss",
        "uuuuuuuuuuuuuuuuuu",
        "UUUUUUUUUUUUUUUUUU",
        "UUUUUUUUUUUUUUUUUU",
    ],
}
