#ifndef EVENTS_H
#define EVENTS_H

enum { RM_NURSERY, RM_SURGERY, RM_DECK, RM_PODS, RM_HELM };

void room_enter(int id, int sx, int sy, int face);
void intro_wake(void);

#endif
