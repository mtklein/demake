#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "events.h"

void game_title(void);
void game_crawl(void);
int  game_class_select(void);
int  game_origin_choose(int cls);
int  origin_class(int o);
const char* origin_name(int o);
void game_name_entry(char* out);

int main(void) {
    gba_init();
    audio_init();
    oam_init();
    vid_init_ui();
    memcpy16(OBJ_TILES, obj_tiles_gfx, OBJ_TILE_COUNT * 16);

    game_title();
    game_crawl();
    int cls = game_class_select();          /* class first: all twelve */
    int origin = game_origin_choose(cls);   /* then who wears it: Tav, the
                                               class's origin, or the Urge */
    char nm[8];
    if (origin_class(origin) >= 0) {                     /* fixed origin: its name */
        const char* on = origin_name(origin);
        int i = 0; for (; on[i] && i < 7; i++) nm[i] = on[i]; nm[i] = 0;
    } else {
        game_name_entry(nm);                 /* Tav and the Urge name themselves */
    }
    G.origin = (u8)origin;
    party_init(cls, nm);
    {
        void party5_refresh_all(void);
        party5_refresh_all();
    }
    memcpy16(PAL_OBJ, pal_tav_classes[cls], 16);
    mgba_logf("start: origin=%d class=%d sub=%d name=%s", origin, cls,
              G.pm[0].subclass, nm);

    room_enter(RM_NURSERY, 3, 6, 0);
    field_draw();
    intro_wake();
    field_run();    /* the helm finale soft-resets the ROM when done */
    for (;;) frame();
}
