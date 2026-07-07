#ifndef EVENTS_H
#define EVENTS_H

enum { RM_NURSERY, RM_SURGERY, RM_DECK, RM_PODS, RM_HELM,
       RM_BEACH, RM_DUNES,
       RM_CHAPEL, RM_CRYPT, RM_OSSUARY, RM_SANCTUM, RM_CAMP, RM_GATES };

void room_enter(int id, int sx, int sy, int face);
void intro_wake(void);
void beach_wake(void);   /* the crash's landing: Tav alone on the sand */

#endif
