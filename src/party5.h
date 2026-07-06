#ifndef PARTY5_H
#define PARTY5_H
#include "rules.h"

extern R5Creature party5[3];
extern R5Creature bench5[];      /* reserve twins; sheets live in G.reserve */

void party5_refresh(int i);
void party5_refresh_all(void);
void party5_heal_full(void);
void party5_bench_build(int r);  /* fresh full-strength creature for reserve r */
void party5_swap(int i, int r);  /* exchange creature state after party_swap */
int  party5_weapon(int i);
int  party5_default_weapon(int cls);
int  party5_cast_ab(int cls);
int  party5_preset(int cls, int a);
int  party5_spell_dc(const R5Creature*);
int  party5_spell_atk(const R5Creature*);

#endif
