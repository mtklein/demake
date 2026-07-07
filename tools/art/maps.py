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
    "y": "flayer_dying", "Z": "cage", "z": "cage_open", "*": "sigil",
    "x": "scree", "w": "chapel_wall", "q": "tomb_door", "Q": "tomb_door_o",
    "e": "gravestone", "A": "cwall", "F": "cfloor", "N": "sconce",
    "B": "rubble", "b": "bones", "i": "carch", "Y": "sarc_t", "j": "sarc_b",
    "@": "campfire", "=": "bedroll",
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
    # Astarion lurking by the west grass (2,5), devourer patrol (15,2),
    # chest (18,5), Tav wakes at (10,6).
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
    # Landmarks: south gap (9,11)-(10,11) back to the crash site, the
    # cleared north pass (7,0)-(8,0) up to the chapel, Lae'zel's cage (2,2)
    # with scavengers at (1,3) and (3,3), cache chest (14,2), Gale's portal
    # sigil (15,8), devourer patrols (12,7) and (5,9).
    "dunes": [
        "nnnnnnnxxnnnnnnnnn",
        "nsssSssssssssssssn",
        "nsZsssssssssssTssn",
        "nssssssGsssSsssssn",
        "nssssnnnnnnnnGsssn",
        "nSssGnnnnnnnnssSsn",
        "nssssssssssssssssn",
        "nsssSssssGsssssssn",
        "nrssssssssssSss*sn",
        "nssssssSssssssssGn",
        "nsGsssssssssSssssn",
        "nnnnnnnnnssnnnnnnn",
    ],
    # The chapel yard on the bluff — the looter band argues before the tomb
    # door (7,1)-(8,1); gravestones flank the worn path; chest (14,6);
    # scree pass south (7,9)-(8,9) back down to the dunes; east gap (15,4)
    # down into the lee of the bluff, where the camp is (stone 5).
    "chapel": [
        "rrrrrrrrrrrrrrrr",
        "rwwwwwwqqwwwwwwr",
        "rGssessssssessGr",
        "rssssssssssssssr",
        "rGsessssssssesss",
        "rssssssssssssssr",
        "rsesssssssssesTr",
        "rGssssssssssssGr",
        "rssGsssssssssssr",
        "rrrrrrrxxrrrrrrr",
    ],
    # The crypt entry hall (DARK) — pillars, rubble, grave-gift chest
    # (10,4); arch north (6,0) to the ossuary, arch south (6,8) out.
    "crypt": [
        "AANAAAiAAAANA",
        "AFFFFFFFFFFFA",
        "AFAFFBFFFAFFA",
        "AFFFFFFbFFFFA",
        "ABFFFFFFFFTFA",
        "AFFAFFFFAFFFA",
        "AFFFbFFFFFBFA",
        "AFFFFFFFFFFFA",
        "AAAAAAiAAAAAA",
    ],
    # The ossuary (DARK) — the floor is ankle-deep in old bones; three of
    # them stand up (ambush at my<=4); arches north/south.
    "ossuary": [
        "AAAANAiANAAAA",
        "AFFBFFFFFbFFA",
        "AbFFFFFFFFFBA",
        "AFFFbFFFFFFFA",
        "AFFFFFFFFFFbA",
        "ABFFFFbFFFFFA",
        "AFFbFFFFFBFFA",
        "AFFFFFFFFFFFA",
        "AAAAAAiAAAAAA",
    ],
    # The camp (stone 5's night room) — a hollow in the bluff's lee where
    # the survivors ring a driftwood fire; the whole room is one screen
    # (15x10), a single tableau. Landmarks the events layer points at:
    # west gap (0,4) up the worn path to the chapel yard, the campfire
    # (7,4), bedrolls (4,2) (7,2) (5,6), companion spots (6,4) (8,4) (7,5)
    # (6,5); the surf keeps the south edge.
    "camp": [
        "rrrrrrrrrrrrrrr",
        "rGssssssssssGsr",
        "rsss=ss=sssssrr",
        "rGsssssssssssGr",
        "sssssss@ssssssr",
        "rsssssssssssssr",
        "rGsss=ssssrssGr",
        "rssssssssssssGr",
        "uuuuuuuuuuuuuuu",
        "UUUUUUUUUUUUUUU",
    ],
    # The sarcophagus chamber (DARK) — Withers sleeps at (6,2), sconces
    # flanking; arch south (6,8) back to the ossuary.
    "sanctum": [
        "AAAAANANAAAAA",
        "AFFFFFFFFFFFA",
        "AFFFFFYFFFFFA",
        "AFFFFFjFFFFFA",
        "AFBFFFFFFbFFA",
        "AFFFFbFFFFBFA",
        "AFFFFFFFFFFFA",
        "AFFFFFFFFFFFA",
        "AAAAAAiAAAAAA",
    ],
}
