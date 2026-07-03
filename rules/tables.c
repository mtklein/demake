/* TEMPORARY hand-written SRD tables — replaced by generated tables from
 * tools/srd/srd_data.py (tools/mksrd.py) once the SRD extraction lands.
 * Numbers here are from the SRD; the generated version is the source of
 * truth going forward. */
#include "rules.h"

#define D(n, s, m) { n, s, m }
#define NOD { 0, 0, 0 }

const R5Weapon r5_weapons[] = {
    [R5W_DAGGER]      = { "Dagger", D(1, 4, 0), NOD,
                          WP_FINESSE | WP_LIGHT | WP_THROWN, DT_PIERCING },
    [R5W_SHORTSWORD]  = { "Shortsword", D(1, 6, 0), NOD,
                          WP_FINESSE | WP_LIGHT, DT_PIERCING },
    [R5W_LONGSWORD]   = { "Longsword", D(1, 8, 0), D(1, 10, 0),
                          WP_VERSATILE, DT_SLASHING },
    [R5W_GREATSWORD]  = { "Greatsword", D(2, 6, 0), NOD,
                          WP_HEAVY | WP_TWO_HANDED, DT_SLASHING },
    [R5W_RAPIER]      = { "Rapier", D(1, 8, 0), NOD, WP_FINESSE, DT_PIERCING },
    [R5W_MACE]        = { "Mace", D(1, 6, 0), NOD, 0, DT_BLUDGEONING },
    [R5W_QUARTERSTAFF]= { "Quarterstaff", D(1, 6, 0), D(1, 8, 0),
                          WP_VERSATILE, DT_BLUDGEONING },
    [R5W_SHORTBOW]    = { "Shortbow", D(1, 6, 0), NOD,
                          WP_AMMUNITION | WP_TWO_HANDED | WP_RANGED, DT_PIERCING },
    [R5W_LONGBOW]     = { "Longbow", D(1, 8, 0), NOD,
                          WP_AMMUNITION | WP_HEAVY | WP_TWO_HANDED | WP_RANGED,
                          DT_PIERCING },
    [R5W_TRIDENT]     = { "Trident", D(1, 6, 0), D(1, 8, 0),
                          WP_THROWN | WP_VERSATILE, DT_PIERCING },
};

#define SV(a, b) ((1 << (a)) | (1 << (b)))

const R5Class r5_classes[R5C_COUNT] = {
    /*                 hd  saves               prof         slots L1..L3      sneak */
    [R5C_BARD]    = { 8, SV(R5_DEX, R5_CHA), {0, 2, 2, 2},
                      { {0}, {2, 0, 0}, {3, 0, 0}, {4, 2, 0} }, {0} },
    [R5C_ROGUE]   = { 8, SV(R5_DEX, R5_INT), {0, 2, 2, 2},
                      { {0}, {0}, {0}, {0} }, {0, 1, 1, 2} },
    [R5C_RANGER]  = { 10, SV(R5_STR, R5_DEX), {0, 2, 2, 2},
                      { {0}, {0, 0, 0}, {2, 0, 0}, {3, 0, 0} }, {0} },
    [R5C_WIZARD]  = { 6, SV(R5_INT, R5_WIS), {0, 2, 2, 2},
                      { {0}, {2, 0, 0}, {3, 0, 0}, {4, 2, 0} }, {0} },
    [R5C_FIGHTER] = { 10, SV(R5_STR, R5_CON), {0, 2, 2, 2},
                      { {0}, {0}, {0}, {0} }, {0} },
    [R5C_CLERIC]  = { 8, SV(R5_WIS, R5_CHA), {0, 2, 2, 2},
                      { {0}, {2, 0, 0}, {3, 0, 0}, {4, 2, 0} }, {0} },
};

const R5Monster r5_monsters[] = {
    [R5M_IMP] = {
        "Imp", 13, 10, { 6, 17, 13, 11, 12, 14 },
        (1 << DT_COLD), (1 << DT_FIRE) | (1 << DT_POISON),
        { { "Sting", 5, D(1, 4, 3), DT_PIERCING,
            D(3, 6, 0), DT_POISON, R5_CON, 11 } },
        1,
    },
    [R5M_BOAR] = {
        "Boar", 11, 11, { 13, 11, 12, 2, 9, 5 },
        0, 0,
        { { "Tusk", 3, D(1, 6, 1), DT_SLASHING, NOD, 0, 0, 0 } },
        1,
    },
    [R5M_DEVOURER] = {
        "Intellect Devourer", 12, 21, { 6, 14, 13, 12, 11, 10 },
        0, 0,
        { { "Claws", 4, D(1, 6, 2), DT_SLASHING, NOD, 0, 0, 0 } },
        1,
    },
    [R5M_CAMBION] = {
        "Cambion", 19, 82, { 18, 18, 16, 14, 12, 16 },
        (1 << DT_COLD) | (1 << DT_FIRE) | (1 << DT_LIGHTNING) |
        (1 << DT_POISON), 0,
        { { "Spear", 7, D(1, 6, 4), DT_PIERCING, D(1, 6, 0), DT_FIRE, 0, 0 },
          { "Fire Ray", 7, D(3, 6, 0), DT_FIRE, NOD, 0, 0, 0 } },
        2,
    },
    [R5M_FLAYER] = {
        "Mind Flayer", 15, 71, { 11, 12, 12, 19, 17, 17 },
        0, 0,
        { { "Tentacles", 7, D(2, 10, 4), DT_PSYCHIC, NOD, 0, 0, 0 } },
        1,
    },
};
