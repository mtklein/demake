#ifndef FIELD_H
#define FIELD_H
#include "gba.h"

#define FMAX_NPC 12
#define NPC_2FRAME 1
#define NPC_GONE   2
#define NPC_PATROL 4

typedef struct {
    s16 x, y;      /* pixel pos */
    s16 hx, hy;    /* patrol home */
    u16 objt;
    u8 pal, face, flags, id;
    u8 aggro_r;    /* detection radius, px */
    u8 chasing;
} Npc;

extern Npc npcs[FMAX_NPC];

void field_load(const u8* meta, int w, int h);
void field_spawn(int mx, int my, int face);
int  field_add_npc(int mx, int my, int objt, int pal, int face, int flags);
void field_remove_npc(int idx);
void field_set_meta(int mx, int my, int id);
int  field_meta_at(int mx, int my);
void field_run(void);
void field_exit(void);
void field_draw(void);
void field_wait(int frames);
void field_walk_npc(int idx, int dir);
void field_face_npc(int idx, int dir);
void field_shake(int frames);
int  field_player_mx(void);
int  field_player_my(void);
int  field_player_x(void);
int  field_player_y(void);
int  field_face(void);
int  field_cam_x(void);
int  field_cam_y(void);
void field_cam_override(int on, int cx, int cy);
void field_hide_player(int on);
void field_set_hero(int objt, int pal);   /* class walker sheet + palette */

void field_npc_patrol(int idx, int radius_px);

/* provided by events.c (game content) */
void ev_interact(int mx, int my);
void ev_step(int mx, int my);
void ev_npc(int idx);
void ev_aggro(int idx);   /* a patroller reached the party */
void ev_light(void);      /* reapply DARK-room dimming (menu may equip the
                           * Everburn; battles and fades clear BLDCNT) */

#endif
