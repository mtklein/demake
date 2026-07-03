# Room layouts: ASCII grids of metatiles (see field_tiles.ORDER for names).

MAPLEG = {
    "#": "wall_f", ".": "floor_a", ",": "floor_v", "O": "pod_c", "o": "pod_o",
    "L": "pool", "R": "resto", "C": "console", "K": "console_lit",
    "D": "door_c", "d": "door_o", "c": "corpse", "W": "window", "X": "void",
    "T": "chest",
}

MAPS = {
    # Room 1: the Nursery — ring of pods, central larva pool, resto device.
    "nursery": [
        "##WW####W####W#",
        "#..O.O.,.O.O..#",
        "#.............#",
        "#.....LLL.....#",
        "#..,..LLL..,..#",
        "#..o......c...#",
        "#R............#",
        "#....,...T....#",
        "#.............#",
        "#######D#######",
    ],
}
