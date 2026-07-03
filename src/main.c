#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "battle.h"

/* demo event handlers (content pass replaces these) */
void ev_interact(int mx, int my) { (void)mx; (void)my; }
void ev_step(int mx, int my) { (void)mx; (void)my; }
void ev_npc(int idx) { (void)idx; }

int main(void) {
    gba_init();
    audio_init();
    oam_init();
    vid_init_ui();
    memcpy16(OBJ_TILES, obj_tiles_gfx, OBJ_TILE_COUNT * 16);
    memcpy16(PAL_OBJ, pal_tav_classes[3], 16);

    party_init(CLS_WIZARD, "TAV");
    party_add_laezel();
    party_add_shadowheart();
    G.flags |= GF_US_FREED;

    /* need a field behind the battle for the mosaic transition */
    field_load(map_nursery, MAP_NURSERY_W, MAP_NURSERY_H);
    field_spawn(3, 5, 0);
    field_draw();

    int r = battle_run(&form_deck);
    mgba_logf("battle result=%d hp=%d,%d,%d xp=%d", r,
              G.pm[0].hp, G.pm[1].hp, G.pm[2].hp, G.pm[0].xp);

    field_load(map_nursery, MAP_NURSERY_W, MAP_NURSERY_H);
    field_spawn(3, 5, 0);
    fade_in(10);
    field_run();
    for (;;) frame();
}
