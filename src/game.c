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

/* Karaoke: lyric lines keyed to playback rows (see docs/under_selune.md).
 * Each entry is two display rows; blank entries clear the screen during the
 * instrumental intro, turn, and outro. Rows match SELUNE's 16-row bars. */
typedef struct { u16 row; const char* a; const char* b; } Lyric;

static const Lyric selune_ly[] = {
    {   0, "", "" },
    { 128, "I found you by the tide,", "and slept" },
    { 160, "The far sky burned away,", "smoke rose" },
    { 192, "You breathed, you were", "alive - with me" },
    { 224, "and all that's left", "is you..." },
    { 256, "if the morning takes", "my name," },
    { 288, "hold on to me now,", "please..." },
    { 320, "Hold us close,", "under dawn..." },
    { 352, "Selune,", "you know our names;" },
    { 384, "you stayed", "when all was gone," },
    { 416, "you stayed with me.", "" },
    { 448, "", "" },
    { 512, "I never said the things", "I meant to say," },
    { 544, "the fear behind my eyes,", "I kept from you" },
    { 576, "but salt and sand, and", "you asleep at last," },
    { 608, "there's nothing", "left to hide..." },
    { 640, "if the morning takes", "my name," },
    { 672, "hold on to me now,", "please..." },
    { 704, "Hold us close,", "under dawn..." },
    { 736, "Selune,", "you know our names;" },
    { 768, "you stayed", "when all was gone," },
    { 800, "you stayed with me.", "" },
    { 832, "so shine - Selune,", "you know our names," },
    { 896, "what we were remains,", "" },
    { 928, "when all else goes -", "you stay with me." },
    { 960, "", "" },
};

static int song_lyrics(int song, const Lyric** out) {
    if (song == SONG_SELUNE) {
        if (out) *out = selune_ly;
        return (int)(sizeof selune_ly / sizeof selune_ly[0]);
    }
    return 0;
}

static void put_center(int y, const char* s, int pal) {
    int n = 0;
    while (s[n]) n++;
    int x = (30 - n) / 2;
    if (x < 0) x = 0;
    txt_put(x, y, s, pal);
}

/* Full-screen synced lyric view; returns when the player presses A/B/Select. */
static void karaoke(int song) {
    const Lyric* ly;
    int n = song_lyrics(song, &ly);
    if (!n) return;
    obj_hide(OBJ_CURSOR);
    win_clear(SCR_JUKEBOX_BOX_X, SCR_JUKEBOX_BOX_Y,
              SCR_JUKEBOX_BOX_W, SCR_JUKEBOX_BOX_H);
    txt_clear(0, 0, 30, 20);
    win_draw(1, 3, 28, 14);
    put_center(5, song_names[song], 0);
    txt_put(10, 16, "(B) BACK", 2);

    int cur = -1;
    u32 t = 0;
    for (;;) {
        frame();
        REG_BG3HOFS = (u16)((++t) >> 3);
        int r = music_row();
        int idx = 0;
        for (int i = 0; i < n; i++) {
            if ((int)ly[i].row <= r) idx = i; else break;
        }
        if (idx != cur) {                       /* line changed: repaint */
            cur = idx;
            txt_clear(2, 8, 26, 6);
            put_center(8, ly[idx].a, 1);        /* current line: bright */
            put_center(9, ly[idx].b, 1);
            if (idx + 1 < n) {                  /* upcoming line: dim */
                put_center(11, ly[idx + 1].a, 2);
                put_center(12, ly[idx + 1].b, 2);
            }
        }
        if (key_hit() & (KEY_A | KEY_B | KEY_SELECT)) break;
    }
    win_clear(1, 3, 28, 14);
    txt_clear(0, 0, 30, 20);
    scr_jukebox();
}

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
                      song_names[i], (i == playing) ? 1 : 0, SCR_JUKEBOX_TRK0_W);
        obj_set(OBJ_CURSOR, SCR_JUKEBOX_TRK0_X * 8 - 14,
                (SCR_JUKEBOX_TRK0_Y + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
        frame();
        REG_BG3HOFS = (u16)((++t) >> 3);
        u16 k = key_hit();
        if (k & KEY_UP)   { sel = (sel + SONG_COUNT - 1) % SONG_COUNT; sfx_play(SFX_CURSOR); }
        if (k & KEY_DOWN) { sel = (sel + 1) % SONG_COUNT; sfx_play(SFX_CURSOR); }
        if (k & KEY_A)    {
            music(sel); playing = sel; sfx_play(SFX_CONFIRM);
            if (song_lyrics(sel, 0)) karaoke(sel);   /* sing along */
        }
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

static const char* const cls_names[CLS_COUNT] = {
    "Bard", "Rogue", "Ranger", "Wizard", "Fighter", "Cleric",
    "Barbarian", "Druid", "Monk", "Paladin", "Sorcerer", "Warlock" };
static const char* const cls_blurb[CLS_COUNT] = {
    "Mocks foes,\nmends allies.\nA cutting\nwit.",
    "Strikes from\nshadow for\ntriple\ndamage.",
    "Marks prey,\nsets snares.\nSteady and\nbrutal.",
    "Fire bolt,\nmissiles,\nsleep.\nBrilliant.",
    "Second wind,\nsurging\nsteel.\nUnbroken.",
    "Bless, mend,\nsmite. The\ngods answer\nyou.",
    "RAGE. Shrug\noff blades.\nHit very\nhard.",
    "Wild shape:\nbecome the\nbeast. Green\nmagic.",
    "Ki-fueled\nflurries.\nFists like\nhammers.",
    "Smite evil,\nlay hands,\nswear the\noath.",
    "Magic in\nthe blood.\nBends every\nrule.",
    "A patron\nwhispers.\nEldritch\npower, owed.",
};

static void blurb_draw(const char* s) {
    for (int j = 0; j < 4; j++) {
        char line[16];
        int k = 0;
        while (*s && *s != '\n' && k < 11) line[k++] = *s++;
        if (*s == '\n') s++;
        line[k] = 0;
        txt_put_n(SCR_CLASSSEL_B0_X, SCR_CLASSSEL_B0_Y + j, line, 2, SCR_CLASSSEL_B0_W);
    }
}

/* origin identity: class (-1 = any), portrait, default subclass or 255,
 * and a short blurb. Fixed origins live under their class in the chooser. */
typedef struct { const char* name; s8 cls; s8 por; u8 sub; const char* blurb; } Origin;
static const Origin origins[ORIG_COUNT] = {
    { "Astarion",  CLS_ROGUE,   POR_ASTARION, 255, "Pale elf. A hunger\nhe hides too well." },
    { "Gale",      CLS_WIZARD,  POR_GALE,     255, "A wizard nursing\na very bad secret." },
    { "Karlach",   CLS_BARBARIAN,POR_KARLACH, 255, "A tiefling: infernal\nengine for a heart." },
    { "Lae'zel",   CLS_FIGHTER, POR_LAEZEL,   255, "Githyanki warrior.\nContempt as armor." },
    { "Shadowheart",CLS_CLERIC, POR_SHADOW,   255, "Cleric of Shar.\nMemories missing." },
    { "Wyll",      CLS_WARLOCK, POR_WYLL,     255, "Blade of Frontiers.\nA pact regretted." },
    { "Dark Urge", -1,          POR_DURGE,    255, "You wake knowing\nsomething used your\nhands, and liked it." },
    { "Custom Tav",-1,          -1,           255, "Someone new.\nChoose your path." },
};

/* THE single source of party art. Identity (origin/companion) wins; custom
 * Tav falls back to the class walker. No class-keyed art table survives this. */
MemberLook member_look(int face, int cls) {
    MemberLook L = { OBJT_HERO, OBJT_HERO_KO, 0, -1 };
    static const u16 walk[CLS_COUNT] = {
        OBJT_HERO, OBJT_HERO, OBJT_HERO, OBJT_HERO, OBJT_HERO, OBJT_HERO,
        OBJT_BARB, OBJT_DRUID, OBJT_MONK, OBJT_PALADIN, OBJT_SORC, OBJT_WARLOCK };
#if defined(POR_TAV_BARD)
    static const s8 tavpor[CLS_COUNT] = {
        POR_TAV_BARD, POR_TAV_ROGUE, POR_TAV_RANGER, POR_TAV_WIZARD,
        -1, -1, -1, -1, -1, -1, -1, -1 };
#else
    static const s8 tavpor[CLS_COUNT] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
#endif
    if (cls < 0 || cls >= CLS_COUNT) cls = 0;
    if (face == ORIG_LAEZEL) {
        L.objt = OBJT_LAEZEL; L.ko = OBJT_LAEZEL_KO; L.pal = 1; L.por = POR_LAEZEL;
    } else if (face == ORIG_SHADOW) {
        L.objt = OBJT_SHADOW; L.ko = OBJT_SHADOW_KO; L.pal = 2; L.por = POR_SHADOW;
    } else {
        L.objt = walk[cls]; L.ko = OBJT_HERO_KO; L.pal = 0; L.por = tavpor[cls];
        switch (face) {                       /* origin portraits (sprite stays class) */
            case ORIG_ASTARION: L.por = POR_ASTARION; break;
            case ORIG_GALE:     L.por = POR_GALE;     break;
            case ORIG_KARLACH:  L.por = POR_KARLACH;  break;
            case ORIG_WYLL:     L.por = POR_WYLL;     break;
            case ORIG_DURGE:    L.por = POR_DURGE;    break;
        }
    }
    return L;
}

int origin_class(int o) { return origins[o].cls; }
const char* origin_name(int o) { return origins[o].name; }
int origin_portrait(int o) { return origins[o].por; }

int game_class_select(void) {
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
    fade_in(10);

    scr_classsel();
    for (int i = 0; i < CLS_COUNT; i++)
        txt_put_n(SCR_CLASSSEL_C0_X, SCR_CLASSSEL_C0_Y + i,
                  cls_names[i], 0, SCR_CLASSSEL_C0_W);

    int sel = 0;
    int demo_hold = G_DEMO ? 60 : 0;
    for (;;) {
        blurb_draw(cls_blurb[sel]);
        MemberLook L = member_look(ORIG_CUSTOM, sel);   /* preview the class walker */
        if (L.pal == 0 && sel < 4) memcpy16(PAL_OBJ, pal_tav_classes[sel], 16);
        obj_set(OBJ_PLAYER, SCR_CLASSSEL_HERO_X * 8 - 8, SCR_CLASSSEL_HERO_Y * 8 - 4,
                1, L.objt, L.pal, 0);

        int done = 0;
        for (;;) {
            obj_set(OBJ_CURSOR, SCR_CLASSSEL_LIST_X * 8 - 6,
                    (SCR_CLASSSEL_C0_Y + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
            frame();
            if (demo_hold) {
                /* poked fixed origins (0-5) steer the class pick; everything
                 * else (Urge/custom/origin-row) honors G_DEMO_CLASS */
                int tcls = G_DEMO_ORIGIN < ORIG_DURGE ? origin_class(G_DEMO_ORIGIN)
                                                      : G_DEMO_CLASS;
                if (--demo_hold == 0) {
                    if (sel < tcls) { sel++; demo_hold = 14; }
                    else { sfx_play(SFX_CONFIRM); done = 1; break; }
                }
                continue;
            }
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_DOWN && sel < CLS_COUNT - 1) { sel++; sfx_play(SFX_CURSOR); break; }
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

/* After the class is picked: who wears it? A new Tav, the class's origin
 * companion (rogue Astarion ... warlock Wyll), or the Dark Urge (any class).
 * Demo drive (G_DEMO_ORIGIN): 0-5 = that origin's row, 6 = Dark Urge,
 * 7 = create your own, 8 = "whatever origin this class has" -- with 8 the
 * class->origin mapping is computed HERE, never poked (origin_flow_check
 * pins that the chooser itself yields Shadowheart under cleric). */
int game_origin_choose(int cls) {
    int omatch = -1;
    for (int o = 0; o < ORIG_DURGE; o++)
        if (origins[o].cls == cls) { omatch = o; break; }

    char play[24];                            /* "Play <Origin>" */
    {
        char* d = play;
        for (const char* s = "Play "; *s; )        *d++ = *s++;
        if (omatch >= 0)
            for (const char* s = origins[omatch].name; *s; ) *d++ = *s++;
        *d = 0;
    }
    const char* lab[3];
    int orow[3];                              /* row -> ORIG_* */
    int n = 0;
    lab[n] = "Create your own"; orow[n++] = ORIG_CUSTOM;
    if (omatch >= 0) { lab[n] = play; orow[n++] = omatch; }
    lab[n] = "The Dark Urge";   orow[n++] = ORIG_DURGE;

    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
    fade_in(10);

    win_draw(1, 1, 27, 4);                    /* class name + the question */
    {
        int cl = 0; while (cls_names[cls][cl]) cl++;
        txt_put(2 + (25 - cl) / 2, 2, cls_names[cls], 1);
        txt_put(2, 3, "But whose story is this?", 0);
    }
    win_draw(2, 6, 26, 8);                    /* rows left, portrait right */
    for (int i = 0; i < n; i++) txt_put(5, 7 + i, lab[i], 0);
    win_draw(1, 14, 28, 6);                   /* blurb card */

    int target = 0;                           /* demo row for G_DEMO_ORIGIN */
    if (G_DEMO_ORIGIN == ORIG_DURGE) target = n - 1;
    else if (omatch >= 0 && (G_DEMO_ORIGIN == omatch || G_DEMO_ORIGIN == ORIG_COUNT))
        target = 1;

    int sel = 0, demo_hold = G_DEMO ? 60 : 0;
    for (;;) {
        {   /* blurb + portrait preview for the highlighted row */
            const char* s = origins[orow[sel]].blurb;
            for (int j = 0; j < 4; j++) {
                char line[26]; int k = 0;
                while (*s && *s != '\n' && k < 24) line[k++] = *s++;
                if (*s == '\n') s++;
                line[k] = 0;
                txt_put_n(3, 15 + j, line, 2, 24);
            }
            s8 por = member_look(orow[sel], cls).por;
            if (por >= 0) ui_portrait(por, 21, 7);
            else txt_clear(21, 7, 6, 6);
        }
        int done = 0;
        for (;;) {
            obj_set(OBJ_CURSOR, 2 * 8 - 6, (7 + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
            frame();
            if (demo_hold) {
                if (--demo_hold == 0) {
                    if (sel < target) { sel++; demo_hold = 12; }
                    else { sfx_play(SFX_CONFIRM); done = 1; break; }
                }
                continue;
            }
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_DOWN && sel < n - 1) { sel++; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_A) { sfx_play(SFX_CONFIRM); done = 1; break; }
        }
        if (done) break;
    }
    obj_hide(OBJ_CURSOR);
    fade_out(10);
    txt_clear(0, 0, 30, 20);
    memset16(SCREENBLOCK(31), 0, 1024);
    return orow[sel];
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
