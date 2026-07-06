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
    "G": "sand_g", "r": "rock", "n": "dune", "<": "wreck_l", ">": "wreck_r",
    "y": "flayer_dying", "Z": "cage", "z": "cage_open",
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
        "WWWWWWWWWWDWWWWWWWWW",
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
        "##W####WD###W#####",
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
    # The Ravaged Beach, crash site — the arc's opening room. Landmarks the
    # events layer points at: dune-gap exit (9,0)-(10,0), hull wreck (4,3)-
    # (5,3), the dying mind flayer (5,4), Shadowheart ashore (4,7) / (11,6),
    # devourer patrol (15,2), chest (18,5), Tav wakes at (10,6).
    "beach": [
        "nnnnnnnnnGGnnnnnnnnn",
        "nsssssSsssssssssrssn",
        "nsrssssssssssssssSGn",
        "nsss<>ssssSssssssssn",
        "nSsssyssssssrsssssGn",
        "nsskssssssssssssksTn",
        "nGssssssSsssssssssSn",
        "nssSssssssssssSssssn",
        "uuuuuuuuuuuuuuuuuuuu",
        "UUUUUUUUUUUUUUUUUUUU",
        "UUUUUUUUUUUUUUUUUUUU",
    ],
    # The dune path — inland toward the chapel bluff (stone 4's door).
    # Landmarks: south gap (9,11)-(10,11) back to the crash site, blocked
    # north pass (7,0)-(8,0), Lae'zel's cage (2,2) with scavengers at (1,3)
    # and (3,3), cache chest (14,2), devourer patrols (12,7) and (5,9).
    "dunes": [
        "nnnnnnnrrnnnnnnnnn",
        "nsssSssssssssssssn",
        "nsZsssssssssssTssn",
        "nssssssGsssSsssssn",
        "nssssnnnnnnnnGsssn",
        "nSssGnnnnnnnnssSsn",
        "nssssssssssssssssn",
        "nsssSssssGsssssssn",
        "nrssssssssssSssrsn",
        "nssssssSssssssssGn",
        "nsGsssssssssSssssn",
        "nnnnnnnnnssnnnnnnn",
    ],
}
