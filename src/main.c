#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"

/* demo event handlers */
void ev_interact(int mx, int my) {
    mgba_logf("interact %d,%d meta=%d", mx, my, field_meta_at(mx, my));
    if (field_meta_at(mx, my) == MT_POOL)
        say("A membrane pool wriggles with tadpoles. One of them was meant for your skull.");
    if (field_meta_at(mx, my) == MT_RESTO) {
        sfx_play(SFX_HEAL);
        say("The restoration pod hums. Wounds knit closed.");
    }
    dlg_close();
}
void ev_step(int mx, int my) {
    if (field_meta_at(mx, my) == MT_DOOR_C) {
        mgba_logf("door at %d,%d", mx, my);
        say("The sphincter door glistens, sealed.");
        dlg_close();
    }
}
void ev_npc(int idx) {
    mgba_logf("npc %d", idx);
    say("US: We are us. We are needed to navigate -- needed to leave this realm.");
    dlg_close();
}

int main(void) {
    gba_init();
    audio_init();
    oam_init();
    vid_init_ui();
    memcpy16(OBJ_TILES, obj_tiles_gfx, OBJ_TILE_COUNT * 16);
    memcpy16(PAL_OBJ, pal_tav_classes[0], 16);   /* bard palette for demo */

    field_load(map_nursery, MAP_NURSERY_W, MAP_NURSERY_H);
    field_spawn(3, 5, 0);
    field_add_npc(10, 6, OBJT_US, 3, 0, NPC_2FRAME);
    field_run();
    for (;;) frame();
}
