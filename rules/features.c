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
    r5_pact_rest(c);
}

/* ------------------------------------------------- resource pools (Char 2.0) */

void r5_refill(R5Creature* c) {           /* long rest: pools from class table */
    c->traits &= (uint8_t)~TR_USED_RELENTLESS;
    if (c->cls >= R5C_COUNT) return;
    for (int i = 0; i < R5R_COUNT; i++)
        c->rsrc[i] = r5_classes[c->cls].rsrc[c->level][i];
}

int r5_spend(R5Creature* c, int pool, int n) {
    if (c->rsrc[pool] < n) return 0;
    c->rsrc[pool] = (uint8_t)(c->rsrc[pool] - n);
    return 1;
}

/* --------------------------------------------------------------- barbarian */

int r5_can_rage(const R5Creature* c) {
    return c->cls == R5C_BARBARIAN && c->rsrc[R5R_RAGE] && !(c->conds & C_RAGING);
}
void r5_start_rage(R5Creature* c) {
    c->rsrc[R5R_RAGE]--;
    c->conds |= C_RAGING;
    c->resist |= (1u << DT_BLUDGEONING) | (1u << DT_PIERCING) | (1u << DT_SLASHING);
}
void r5_end_rage(R5Creature* c) {
    c->conds &= (uint16_t)~C_RAGING;
    c->resist &= (uint16_t)~((1u << DT_BLUDGEONING) | (1u << DT_PIERCING) |
                             (1u << DT_SLASHING));
}
/* ------------------------------------------------------------------- monk */

int r5_martial_die(const R5Creature* c) {  /* d4 at levels 1-3 */
    return c->cls == R5C_MONK ? 4 : 0;
}

/* ---------------------------------------------------------------- paladin */

/* smite: spend a slot on a hit, add (1+slot_level)d8 radiant */
R5DiceSpec r5_smite_dice(int slot_level) {
    R5DiceSpec d = { (uint8_t)(1 + slot_level), 8, 0 };
    return d;
}
int r5_lay_hands(R5Creature* pal, R5Creature* t, int amt) {
    if (amt <= 0 || pal->rsrc[R5R_LAY] < amt) return 0;
    pal->rsrc[R5R_LAY] = (uint8_t)(pal->rsrc[R5R_LAY] - amt);
    r5_heal(t, amt);
    return 1;
}

/* ---------------------------------------------------------------- warlock */

int r5_pact_cast(R5Creature* c) {          /* spend one pact slot */
    return r5_spend(c, R5R_PACT, 1);
}
void r5_pact_rest(R5Creature* c) {         /* pact slots return on short rest */
    if (c->cls == R5C_WARLOCK)
        c->rsrc[R5R_PACT] = r5_classes[R5C_WARLOCK].rsrc[c->level][R5R_PACT];
}

/* skill check: d20 + ability mod + (prof) + (prof again if expertise) */
int r5_skill_check(R5RNG* r, const R5Creature* c, int skill, uint32_t prof,
                   uint32_t expert, int* d20out) {
    R5Dice d = r5_d20(r, (c->traits & TR_LUCKY) ? R5F_LUCKY : 0);
    int total = d.total + r5_mod(c->ab[r5_skill_ability[skill]]);
    if (prof & (1u << skill))   total += r5_prof(c);
    if (expert & (1u << skill)) total += r5_prof(c);
    if (d20out) *d20out = d.total;
    return total;
}
