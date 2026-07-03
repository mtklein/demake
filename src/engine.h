#ifndef ENGINE_H
#define ENGINE_H
#include "gba.h"
#include "audio.h"

/* util.c / text.c */
void frame(void);          /* vsync + oam flush + key poll */
void sky_autodrift(int on);

/* video.c */
void vid_init_ui(void);
void fade_out(int frames);
void fade_in(int frames);

/* text.c */
void txt_put(int x, int y, const char* s, int pal);
void txt_put_n(int x, int y, const char* s, int pal, int w);
void txt_clear(int x, int y, int w, int h);
void win_draw(int x, int y, int w, int h);
void win_clear(int x, int y, int w, int h);
void dlg_open(void);
void dlg_close(void);
void dlg_print(const char* s, int pal);
void say(const char* s);
void say_keep(const char* s);
int  choose(int n, const char* const* opts);

/* oam.c */
void oam_init(void);
void oam_flush(void);
void obj_set(int i, int x, int y, int shapesize, int tile, int pal, int prio);
void obj_hide(int i);
void obj_flip(int i, int h, int v);

/* obj slot allocation */
#define OBJ_CURSOR 0
#define OBJ_PLAYER 4    /* field player sprite */
#define OBJ_NPC0   8    /* field npcs 8..23 */
#define OBJ_BATTLE 24   /* battle combatants + popups */

#endif
