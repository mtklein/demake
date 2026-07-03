#include "rules.h"

void r5_seed(R5RNG* r, uint32_t seed) {
    r->s = seed ? seed : 0x9E3779B9u;
}

uint32_t r5_rand(R5RNG* r) {
    uint32_t s = r->s;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return r->s = s;
}

int r5_die(R5RNG* r, int sides) {
    return (int)(r5_rand(r) % (uint32_t)sides) + 1;
}

R5Dice r5_roll(R5RNG* r, int n, int sides, int mod) {
    R5Dice d;
    d.n = (uint8_t)(n > R5_MAX_DICE ? R5_MAX_DICE : n);
    d.sides = (uint8_t)sides;
    d.mod = (int8_t)mod;
    int sum = 0;
    for (int i = 0; i < d.n; i++) {
        d.rolls[i] = (uint8_t)r5_die(r, sides);
        sum += d.rolls[i];
    }
    d.total = (int16_t)(sum + mod);
    return d;
}

/* d20 with advantage/disadvantage; ADV|DIS cancel (straight roll).
 * rolls[] holds the raw dice; total = the die USED, no modifiers. */
R5Dice r5_d20(R5RNG* r, int flags) {
    R5Dice d;
    d.sides = 20;
    d.mod = 0;
    int adv = (flags & R5F_ADV) != 0, dis = (flags & R5F_DIS) != 0;
    if (adv == dis) {
        d.n = 1;
        d.rolls[0] = (uint8_t)r5_die(r, 20);
        d.total = d.rolls[0];
    } else {
        d.n = 2;
        d.rolls[0] = (uint8_t)r5_die(r, 20);
        d.rolls[1] = (uint8_t)r5_die(r, 20);
        int a = d.rolls[0], b = d.rolls[1];
        d.total = (int16_t)(adv ? (a > b ? a : b) : (a < b ? a : b));
    }
    return d;
}
