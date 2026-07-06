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
    game_creation(cls, origin);             /* race/background/array + name;
                                               fixed origins skip the flow */
    mgba_logf("start: origin=%d class=%d sub=%d name=%s", origin, cls,
              G.pm[0].subclass, G.pm[0].name);

    if (G_DEMO_BEACH) {
        /* scenario hook: skip the ship and wake on the sand carrying the
         * poked world-state (see test/scenario.py beach_setup) */
        G.flags = G_BEACH_FLAGS;
        mgba_logf("beach boot flags=%x", G.flags);
        beach_wake();
    } else {
        room_enter(RM_NURSERY, 3, 6, 0);
        field_draw();
        intro_wake();
    }
    field_run();
    for (;;) frame();
}
