#include "rules.h"

int r5_can_second_wind(const R5Creature* c) {
    return c->cls == R5C_FIGHTER && !(c->used & USED_SECOND_WIND) && c->hp > 0;
}

R5Dice r5_second_wind(R5RNG* r, R5Creature* c) {
    R5Dice d = r5_roll(r, 1, 10, c->level);
    c->used |= USED_SECOND_WIND;
    r5_heal(c, d.total);
    return d;
}

int r5_can_action_surge(const R5Creature* c) {
    /* Action Surge arrives at fighter level 2, not level 1 */
    return c->cls == R5C_FIGHTER && c->level >= 2 &&
           !(c->used & USED_ACTION_SURGE);
}

void r5_use_action_surge(R5Creature* c) {
    c->used |= USED_ACTION_SURGE;
}

int r5_sneak_dice(const R5Creature* c) {
    if (c->cls != R5C_ROGUE) return 0;
    int l = c->level < 1 ? 1 : c->level > 3 ? 3 : c->level;
    return r5_classes[R5C_ROGUE].sneak_d6[l];
}

int r5_spend_slot(R5Creature* c, int slot_level) {
    if (slot_level < 1 || slot_level > 3) return 0;
    if (!c->slots[slot_level - 1]) return 0;
    c->slots[slot_level - 1]--;
    return 1;
}

void r5_short_rest(R5Creature* c) {
    c->used = 0;
}
