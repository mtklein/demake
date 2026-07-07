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
int  dlg_is_open(void);
void dlg_print(const char* s, int pal);
void dlg_set_portrait(int id);   /* POR_* from assets.h, -1 = none */
void say(const char* s);
void say_p(int portrait, const char* s);
void say_keep(const char* s);
int  choose(int n, const char* const* opts);
void ui_portrait(int por, int cx, int cy);

/* oam.c */
void oam_init(void);
void oam_flush(void);
void obj_set(int i, int x, int y, int shapesize, int tile, int pal, int prio);
void obj_hide(int i);
void obj_flip(int i, int h, int v);

/* obj slot allocation */
#define OBJ_CURSOR 0
#define OBJ_ALERT  1    /* field "!" marker (patrol aggro) */
#define OBJ_PLAYER 4    /* field player sprite */
#define OBJ_NPC0   8    /* field npcs 8..19 (FMAX_NPC), 20-23 spare;
                         * battle tether dots + cursor glyphs 24-28
                         * (encounter.c OBJ_TETH), field patrol-cone dots
                         * 29-38 (field.c OBJ_CONE -- field-mode, kept
                         * disjoint from battle garnish on purpose);
                         * field skill-check die 40-42 (OBJ_FDIE, field-mode);
                         * encounter popups/zZ/dice claim 40+ (battle-mode --
                         * shares the range with OBJ_FDIE, safe because field
                         * and battle never draw at once, and the die hides on
                         * every return to the field loop) */
#define OBJ_FDIE   40   /* the field skill-check d20: face + up to two digits */

#endif
