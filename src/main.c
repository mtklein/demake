#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "events.h"

void game_title(void);
void game_crawl(void);
int  game_class_select(void);
void game_name_entry(char* out);

int main(void) {
    gba_init();
    audio_init();
    oam_init();
    vid_init_ui();
    memcpy16(OBJ_TILES, obj_tiles_gfx, OBJ_TILE_COUNT * 16);

    game_title();
    game_crawl();
    int cls = game_class_select();
    char nm[8];
    game_name_entry(nm);
    party_init(cls, nm);
    {
        void party5_refresh_all(void);
        party5_refresh_all();
    }
    memcpy16(PAL_OBJ, pal_tav_classes[cls], 16);
    mgba_logf("start: class=%d name=%s", cls, nm);

    room_enter(RM_NURSERY, 3, 6, 0);
    field_draw();
    intro_wake();
    field_run();    /* the helm finale soft-resets the ROM when done */
    for (;;) frame();
}
