#include "gba.h"
#include "assets.h"

void vid_init_ui(void) {
    memcpy16(CHARBLOCK(0), ui_tiles, UI_TILE_COUNT * 16);
    memcpy16(PAL_BG, pal_bg, 256);
    memcpy16(PAL_OBJ, pal_obj, 256);
    memset16(SCREENBLOCK(30), 0, 1024);   /* text map  */
    memset16(SCREENBLOCK(31), 0, 1024);   /* window map */
    REG_BG0CNT = BGCNT_CB(0) | BGCNT_SB(30) | BGCNT_PRIO(0);
    REG_BG1CNT = BGCNT_CB(0) | BGCNT_SB(31) | BGCNT_PRIO(1);
    REG_BG0HOFS = 0; REG_BG0VOFS = 0;
    REG_BG1HOFS = 0; REG_BG1VOFS = 0;
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_OBJ | DCNT_OBJ_1D;
}

/* brightness-fade the whole screen to/from black */
void fade_out(int frames) {
    REG_BLDCNT = 0x00FF;                 /* darken BG0-3, OBJ, backdrop */
    for (int i = 0; i <= frames; i++) {
        REG_BLDY = (u16)(i * 16 / frames);
        vsync();
    }
}

void fade_in(int frames) {
    REG_BLDCNT = 0x00FF;
    for (int i = frames; i >= 0; i--) {
        REG_BLDY = (u16)(i * 16 / frames);
        vsync();
    }
    REG_BLDCNT = 0;
}
