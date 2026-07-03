#ifndef PARTY5_H
#define PARTY5_H
#include "rules.h"

extern R5Creature party5[3];

void party5_refresh(int i);
void party5_refresh_all(void);
void party5_heal_full(void);
int  party5_weapon(int i);
int  party5_default_weapon(int cls);
int  party5_cast_ab(int cls);
int  party5_spell_dc(const R5Creature*);
int  party5_spell_atk(const R5Creature*);

#endif
