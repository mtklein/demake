#include "gba.h"
#include "assets.h"
#include "engine.h"

int main(void) {
    gba_init();
    audio_init();
    oam_init();
    vid_init_ui();
    memcpy16(OBJ_TILES, obj_tiles_gfx, OBJ_TILE_COUNT * 16);

    say("Your eyes crack open. Pain writhes behind one of them -- something alive.");
    say("LAE'ZEL: Tsk'va! You are no thrall. Together, we might survive.");
    say_keep("The exposed brain pulses.\n\"Free us... please.\"");
    static const char* const opts[] = { "Free the brain", "Leave it" };
    int c = choose(2, opts);
    mgba_logf("choice=%d", c);
    say(c == 0 ? "US: We are us. Grateful." : "The brain pulses, abandoned.");
    dlg_close();

    for (;;) frame();
}
