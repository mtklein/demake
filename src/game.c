/* Title screen, prologue crawl, character creation. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "game.h"

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

void game_title(void) {
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    sky_full();
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
    REG_BLDCNT = 0x00FF; REG_BLDY = 16;

    music(SONG_PRELUDE);
    win_draw(7, 5, 16, 5);
    txt_put(10, 6, "N A U T I L O I D", 1);
    txt_put(9, 8, "a BG3 demake of the", 2);
    txt_put(9, 9, "mind flayer prologue", 2);

#ifdef OBJT_NAUT
    obj_set(OBJ_PLAYER, 104, 8, OBJS_NAUT == 2 ? 2 : 2, OBJT_NAUT, OBJP_NAUT, 2);
#endif

    fade_in(20);
    u32 t = 0;
    for (;;) {
        frame();
        t++;
        REG_BG3HOFS = (u16)(t >> 3);
        if (t & 32) txt_put(11, 14, "PRESS START", 0);
        else txt_clear(11, 14, 11, 1);
#ifdef OBJT_NAUT
        obj_set(OBJ_PLAYER, 104, 10 + ((t >> 5) & 3), 2, OBJT_NAUT, OBJP_NAUT, 2);
#endif
        if (key_hit() & (KEY_START | KEY_A)) break;
        if (G_DEMO && t >= 90) break;
    }
    sfx_play(SFX_CONFIRM);
    fade_out(20);
    obj_hide(OBJ_PLAYER);
    win_clear(7, 5, 16, 5);
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

int game_class_select(void) {
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
    fade_in(10);

    win_draw(3, 1, 24, 4);
    txt_put(5, 2, "Who were you, before the", 0);
    txt_put(5, 3, "nautiloid took you?", 0);

    win_draw(3, 5, 10, 10);
    for (int i = 0; i < 4; i++) txt_put(6, 7 + i * 2, cls_names[i], 0);
    win_draw(13, 5, 14, 10);

    int sel = 0;
    int demo_hold = G_DEMO ? 60 : 0;
    for (;;) {
        for (int i = 0; i < 4; i++) {
            txt_clear(14, 7, 12, 6);
        }
        /* blurb for selection */
        {
            const char* s = cls_blurb[sel];
            int x = 14, y = 8;
            while (*s) {
                if (*s == '\n') { x = 14; y++; s++; continue; }
                char b[2] = { *s++, 0 };
                txt_put(x++, y, b, 2);
            }
        }
        /* hero preview sprite in blurb pane */
        memcpy16(PAL_OBJ, pal_tav_classes[sel], 16);
        obj_set(OBJ_PLAYER, 152, 96, 1, OBJT_HERO, 0, 0);

        int done = 0;
        for (;;) {
            obj_set(OBJ_CURSOR, 3 * 8 - 6, (7 + sel * 2) * 8 - 4, 1, OBJT_HAND, 7, 0);
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
    win_draw(6, 1, 18, 4);
    txt_put(8, 2, "Name this soul:", 0);
    win_draw(1, 6, 28, 9);
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 13; c++) {
            char b[2] = { rowsrc[r][c], 0 };
            txt_put(3 + c * 2, 8 + r, b, 0);
        }
    txt_put(3, 12, "END", 1);

    int cr = 0, cc = 0;
    for (;;) {
        /* current name display */
        for (int i = 0; i < 6; i++) {
            char b[2] = { i < len ? name[i] : '_', 0 };
            txt_put(15 + i, 2, b, 1);
        }
        int cx = (cr == 4) ? 3 : 3 + cc * 2;
        int cy = (cr == 4) ? 12 : 8 + cr;
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
