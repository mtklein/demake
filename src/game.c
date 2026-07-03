/* Title screen, jukebox, prologue crawl, character creation.
 * All static layout comes from the build-time-constrained screen generator
 * (tools/ui_screens.py -> build/gen/screens.{c,h}). */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "game.h"
#include "screens.h"

static void sky_full(void) {
    /* Avernus sky across the whole screen on BG3 */
    memcpy16(CHARBLOCK(2), sky_tiles_gfx, SKY_TILE_COUNT * 16);
    for (int y = 0; y < 32; y++) {
        int band = y / 8;
        for (int x = 0; x < 32; x++) {
            int v = ((x * 7 + y * 3) >> 2) & 1;
            ((vu16*)SCREENBLOCK(28))[y * 32 + x] = (u16)((v * 4 + band) | (4 << 12));
        }
    }
    REG_BG3CNT = BGCNT_CB(2) | BGCNT_SB(28) | BGCNT_SIZE(0) | BGCNT_PRIO(3);
}

static const char* const jb_names[SONG_COUNT] = {
    "Prelude",           /* SONG_PRELUDE */
    "Fleshy Halls",      /* SONG_EXPLORE */
    "Draw Steel!",       /* SONG_BATTLE  */
    "Commander Zhalk",   /* SONG_BOSS    */
    "Victory",           /* SONG_VICTORY */
    "The Long Fall",     /* SONG_CRASH   */
    "Kind of Azure",     /* SONG_AZURE   */
};

/* Sound-test screen reached with SELECT from the title. */
static void jukebox(void) {
    obj_hide(OBJ_PLAYER);
    win_clear(SCR_TITLE_BOX_X, SCR_TITLE_BOX_Y, SCR_TITLE_BOX_W, SCR_TITLE_BOX_H);
    txt_clear(0, 0, 30, 20);
    scr_jukebox();

    int sel = 0, playing = 0;
    u32 t = 0;
    for (;;) {
        for (int i = 0; i < SONG_COUNT; i++)
            txt_put_n(SCR_JUKEBOX_TRK0_X, SCR_JUKEBOX_TRK0_Y + i,
                      jb_names[i], (i == playing) ? 1 : 0, SCR_JUKEBOX_TRK0_W);
        obj_set(OBJ_CURSOR, SCR_JUKEBOX_TRK0_X * 8 - 14,
                (SCR_JUKEBOX_TRK0_Y + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
        frame();
        REG_BG3HOFS = (u16)((++t) >> 3);
        u16 k = key_hit();
        if (k & KEY_UP)   { sel = (sel + SONG_COUNT - 1) % SONG_COUNT; sfx_play(SFX_CURSOR); }
        if (k & KEY_DOWN) { sel = (sel + 1) % SONG_COUNT; sfx_play(SFX_CURSOR); }
        if (k & KEY_A)    { music(sel); playing = sel; sfx_play(SFX_CONFIRM); }
        if (k & (KEY_B | KEY_SELECT)) { sfx_play(SFX_CANCEL); break; }
    }
    obj_hide(OBJ_CURSOR);
    win_clear(SCR_JUKEBOX_BOX_X, SCR_JUKEBOX_BOX_Y, SCR_JUKEBOX_BOX_W, SCR_JUKEBOX_BOX_H);
    txt_clear(0, 0, 30, 20);
    scr_title();
    music(SONG_PRELUDE);
}

void game_title(void) {
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    sky_full();
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BLDCNT = 0x00FF; REG_BLDY = 16;

    music(SONG_PRELUDE);
    scr_title();

#ifdef OBJT_NAUT
    obj_set(OBJ_PLAYER, 104, 8, 2, OBJT_NAUT, OBJP_NAUT, 2);
#endif

    fade_in(20);
    u32 t = 0;
    for (;;) {
        frame();
        t++;
        REG_BG3HOFS = (u16)(t >> 3);
        if (t & 32) txt_put(SCR_TITLE_START_X, SCR_TITLE_START_Y, "PRESS START", 0);
        else txt_clear(SCR_TITLE_START_X, SCR_TITLE_START_Y, SCR_TITLE_START_W, 1);
        txt_put_n(6, 18, G_DEMO ? "L: ATTRACT MODE ON" : "L: ATTRACT MODE off",
                  G_DEMO ? 1 : 2, 18);
#ifdef OBJT_NAUT
        obj_set(OBJ_PLAYER, 104, 10 + ((t >> 5) & 3), 2, OBJT_NAUT, OBJP_NAUT, 2);
#endif
        u16 k = key_hit();
        if (k & (KEY_START | KEY_A)) break;
        if (k & KEY_SELECT) { jukebox(); t = 0; }
        if (k & KEY_L) { G_DEMO ^= 1; sfx_play(SFX_CONFIRM); }
        if (G_DEMO && t >= 90) break;
    }
    sfx_play(SFX_CONFIRM);
    fade_out(20);
    obj_hide(OBJ_PLAYER);
    win_clear(SCR_TITLE_BOX_X, SCR_TITLE_BOX_Y, SCR_TITLE_BOX_W, SCR_TITLE_BOX_H);
    txt_clear(0, 0, 30, 20);
}

/* prologue crawl: sky scrolls, nautiloid flies, dragons pass, text pages */
void game_crawl(void) {
    sky_full();
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
    sky_autodrift(1);
    fade_in(15);

    static const char* const pages[] = {
        "The city of Baldur's Gate never saw it coming. Out of a clear sky: tentacles.",
        "A nautiloid -- warship of the mind flayers. It took what it wanted. It took YOU.",
        "Cold hands held your eye open. A tadpole, lowered in. A seed of transformation.",
        "Then: dragonfire. Githyanki riders. The ship tore between planes -- and fell, burning, into Hell.",
    };

    for (int p = 0; p < 4; p++) {
#ifdef OBJT_NAUT
        obj_set(OBJ_PLAYER, 40 + p * 34, 26 + ((p & 1) ? 8 : 0), 2, OBJT_NAUT, OBJP_NAUT, 2);
#endif
#ifdef OBJT_DRAGON
        if (p >= 2) obj_set(OBJ_PLAYER + 1, 150 - p * 20, 40 + p * 6, 5, OBJT_DRAGON, OBJP_DRAGON, 2);
#endif
        say(pages[p]);
    }
    dlg_close();
    obj_hide(OBJ_PLAYER);
    obj_hide(OBJ_PLAYER + 1);
    sky_autodrift(0);
    fade_out(20);
}

static const char* const cls_names[4] = { "Bard", "Rogue", "Ranger", "Wizard" };
static const char* const cls_blurb[4] = {
    "Mocks foes,\nmends allies.\nA cutting\nwit.",
    "Strikes from\nshadow for\ntriple\ndamage.",
    "Marks prey,\nsets snares.\nSteady and\nbrutal.",
    "Fire bolt,\nmissiles,\nsleep.\nBrilliant.",
};

static void blurb_draw(const char* s) {
    for (int j = 0; j < 4; j++) {
        char line[16];
        int k = 0;
        while (*s && *s != '\n' && k < 12) line[k++] = *s++;
        if (*s == '\n') s++;
        line[k] = 0;
        txt_put_n(SCR_CLASSSEL_B0_X, SCR_CLASSSEL_B0_Y + j, line, 2, SCR_CLASSSEL_B0_W);
    }
}

int game_class_select(void) {
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
    fade_in(10);

    scr_classsel();
    for (int i = 0; i < 4; i++)
        txt_put_n(SCR_CLASSSEL_C0_X, SCR_CLASSSEL_C0_Y + i * 2,
                  cls_names[i], 0, SCR_CLASSSEL_C0_W);

    int sel = 0;
    int demo_hold = G_DEMO ? 60 : 0;
    for (;;) {
        blurb_draw(cls_blurb[sel]);
        memcpy16(PAL_OBJ, pal_tav_classes[sel], 16);
        obj_set(OBJ_PLAYER, SCR_CLASSSEL_HERO_X * 8 - 8, SCR_CLASSSEL_HERO_Y * 8 - 4,
                1, OBJT_HERO, 0, 0);

        int done = 0;
        for (;;) {
            obj_set(OBJ_CURSOR, SCR_CLASSSEL_LIST_X * 8 - 6,
                    (SCR_CLASSSEL_C0_Y + sel * 2) * 8 - 4, 1, OBJT_HAND, 7, 0);
            frame();
            if (demo_hold) {
                if (--demo_hold == 0) {
                    if (sel < G_DEMO_CLASS) { sel++; demo_hold = 14; }
                    else { sfx_play(SFX_CONFIRM); done = 1; break; }
                }
                continue;
            }
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_DOWN && sel < 3) { sel++; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_A) { sfx_play(SFX_CONFIRM); done = 1; break; }
        }
        if (done) break;
    }
    obj_hide(OBJ_CURSOR);
    obj_hide(OBJ_PLAYER);
    fade_out(10);
    txt_clear(0, 0, 30, 20);
    memset16(SCREENBLOCK(31), 0, 1024);
    return sel;
}

/* FF-style name entry, 6 chars, default TAV */
void game_name_entry(char* out) {
    static const char rowsrc[4][14] = {
        { 'A','B','C','D','E','F','G','H','I','J','K','L','M',0 },
        { 'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0 },
        { 'a','b','c','d','e','f','g','h','i','j','k','l','m',0 },
        { 'n','o','p','q','r','s','t','u','v','w','x','y','z',0 },
    };
    char name[7] = "TAV";
    int len = 3;

    if (G_DEMO) { out[0] = 'T'; out[1] = 'A'; out[2] = 'V'; out[3] = 0; return; }

    fade_in(8);
    scr_namehdr();
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 13; c++) {
            char b[2] = { rowsrc[r][c], 0 };
            txt_put(SCR_NAMEHDR_GRID_X + c * 2, SCR_NAMEHDR_GRID_Y + r, b, 0);
        }
    txt_put(SCR_NAMEHDR_END_X, SCR_NAMEHDR_END_Y, "END", 1);

    int cr = 0, cc = 0;
    for (;;) {
        for (int i = 0; i < 6; i++) {
            char b[2] = { i < len ? name[i] : '_', 0 };
            txt_put(SCR_NAMEHDR_NAME_X + i, SCR_NAMEHDR_NAME_Y, b, 1);
        }
        int cx = (cr == 4) ? SCR_NAMEHDR_END_X : SCR_NAMEHDR_GRID_X + cc * 2;
        int cy = (cr == 4) ? SCR_NAMEHDR_END_Y : SCR_NAMEHDR_GRID_Y + cr;
        obj_set(OBJ_CURSOR, cx * 8 - 7, cy * 8 - 4, 1, OBJT_HAND, 7, 0);
        frame();
        u16 k = key_hit();
        if (k & KEY_UP && cr > 0) { cr--; sfx_play(SFX_CURSOR); }
        if (k & KEY_DOWN && cr < 4) { cr++; sfx_play(SFX_CURSOR); }
        if (k & KEY_LEFT && cc > 0 && cr < 4) { cc--; sfx_play(SFX_CURSOR); }
        if (k & KEY_RIGHT && cc < 12 && cr < 4) { cc++; sfx_play(SFX_CURSOR); }
        if (k & KEY_B && len > 0) { len--; sfx_play(SFX_CANCEL); }
        if (k & KEY_A) {
            if (cr == 4) {
                if (len > 0) { sfx_play(SFX_CONFIRM); break; }
            } else if (len < 6) {
                name[len++] = rowsrc[cr][cc];
                sfx_play(SFX_CURSOR);
            }
        }
        if (k & KEY_START && len > 0) { sfx_play(SFX_CONFIRM); break; }
    }
    name[len] = 0;
    for (int i = 0; i <= len; i++) out[i] = name[i];
    obj_hide(OBJ_CURSOR);
    fade_out(10);
    txt_clear(0, 0, 30, 20);
    memset16(SCREENBLOCK(31), 0, 1024);
}
