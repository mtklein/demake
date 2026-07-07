/* Title screen, jukebox, prologue crawl, character creation.
 * All static layout comes from the build-time-constrained screen generator
 * (tools/ui_screens.py -> build/gen/screens.{c,h}). */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "game.h"
#include "party5.h"
#include "rules.h"
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

/* One synced repaint step, shared by the jukebox karaoke and the camp's
 * story mode: find the lyric entry for playback row r and, if it changed
 * from cur, repaint the sheet (current line bright, upcoming line dim).
 * Returns the entry now showing. One painter, two frames around it -- the
 * lyric sync can never drift between the jukebox and the story scene. */
static int karaoke_paint(const Lyric* ly, int n, int r, int cur) {
    int idx = 0;
    for (int i = 0; i < n; i++) {
        if ((int)ly[i].row <= r) idx = i; else break;
    }
    if (idx != cur) {                       /* line changed: repaint */
        txt_clear(2, 8, 26, 6);
        put_center(8, ly[idx].a, 1);        /* current line: bright */
        put_center(9, ly[idx].b, 1);
        if (idx + 1 < n) {                  /* upcoming line: dim */
            put_center(11, ly[idx + 1].a, 2);
            put_center(12, ly[idx + 1].b, 2);
        }
    }
    return idx;
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
        cur = karaoke_paint(ly, n, music_row(), cur);
        if (key_hit() & (KEY_A | KEY_B | KEY_SELECT)) break;
    }
    win_clear(1, 3, 28, 14);
    txt_clear(0, 0, 30, 20);
    scr_jukebox();
}

/* Story-mode karaoke: the camp night plays Under Selune as a scene
 * (docs/under_selune.md -- this moment is what the song was written for).
 * The same synced sheet as the jukebox view with none of its chrome: no
 * window, no title, no BACK hint, no cursor -- the lyric rows paint
 * straight over whatever tableau the caller has on screen. The caller
 * starts the song; this returns when it has played through once (the
 * playback row wraps to the loop point) or when START skips it. Under
 * G_DEMO it bows out once the first sung lines have shown, so attract
 * mode and scenarios keep moving; every painted line logs its row for
 * the lyric-sync asserts. */
void game_story_karaoke(int song) {
    const Lyric* ly;
    int n = song_lyrics(song, &ly);
    if (!n) return;
    int cur = -1, last = -1;
    for (;;) {
        frame();
        int r = music_row();
        if (r < 0 || r < last) { mgba_log("karaoke end"); break; }
        last = r;
        int idx = karaoke_paint(ly, n, r, cur);
        if (idx != cur) { cur = idx; mgba_logf("lyric %d @%d", idx, r); }
        if (key_hit() & KEY_START) { mgba_log("karaoke skip"); break; }
        if (G_DEMO && idx >= 2) { mgba_log("karaoke autoskip"); break; }
    }
    txt_clear(2, 8, 26, 6);
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
/* every line must fit the blurb card (SCR_CLASSSEL_B0_W chars) or
 * blurb_draw splits it mid-word -- "mends allie / s." shipped once.
 * The card cannot widen: the class list owns the rest of the screen. */
static const char* const cls_blurb[CLS_COUNT] = {
    "A cutting\nwit: mocks\nfoes, mends\nallies.",
    "Strikes out\nof shadow:\ntriple\ndamage.",
    "Marks prey,\nsets traps.\nSteady and\nbrutal.",
    "Fire bolt,\nmissiles,\nsleep.\nBrilliant.",
    "Second wind\nand surging\nsteel.\nUnbroken.",
    "Bless, mend\nand smite:\nthe gods\nanswer you.",
    "RAGE. Shrug\noff blades.\nHit very\nhard.",
    "Nature's\nmagic. Wild\nshape: be\nthe beast.",
    "Ki-fueled\nflurries.\nFists like\nhammers.",
    "Smite evil,\nlay hands,\nswear the\noath.",
    "Magic in\nthe blood.\nBends every\nrule.",
    "A patron\nwhispers.\nEldritch\npower owed.",
};

static void blurb_draw(const char* s) {
    for (int j = 0; j < 4; j++) {
        char line[16];
        int k = 0;
        while (*s && *s != '\n' && k < SCR_CLASSSEL_B0_W) line[k++] = *s++;
        if (*s == '\n') s++;
        line[k] = 0;
        txt_put_n(SCR_CLASSSEL_B0_X, SCR_CLASSSEL_B0_Y + j, line, 2, SCR_CLASSSEL_B0_W);
    }
}

/* origin identity: class (-1 = any), portrait, canon subclass (255 = the
 * player's own pick), canon race + background (character2.md's origin
 * table + "Origin sheet identities"; BG3's Charlatan Astarion maps to our
 * Criminal -- nearest of the ten staged), and a short blurb. Timing is the
 * class's SRD 5.1 reveal level, not stored here: cleric/warlock at 1,
 * wizard at 2, the rest at 3. Fixed origins live under their class in the
 * chooser. The Urge and custom Tav row 0s are the PICKER DEFAULTS, not
 * fixed sheets: both walk the full creation flow. */
typedef struct { const char* name; s8 cls; s8 por; u8 sub;
                 u8 race, bg; const char* blurb; } Origin;
static const Origin origins[ORIG_COUNT] = {
    { "Astarion",  CLS_ROGUE,   POR_ASTARION, R5SUB_THE_AMBUSH_ARTIST,
      R5RACE_HIGH_ELF,  R5BG_CRIMINAL,
      "Pale elf. A hunger\nhe hides too well." },
    { "Gale",      CLS_WIZARD,  POR_GALE,     R5SUB_EVOCATION,
      R5RACE_HUMAN,     R5BG_SAGE,
      "A wizard nursing\na very bad secret." },
    { "Karlach",   CLS_BARBARIAN,POR_KARLACH, R5SUB_BERSERKER,
      R5RACE_TIEFLING,  R5BG_OUTLANDER,
      "A tiefling: infernal\nengine for a heart." },
    { "Lae'zel",   CLS_FIGHTER, POR_LAEZEL,   R5SUB_CHAMPION,
      R5RACE_GITHYANKI, R5BG_SOLDIER,
      "Githyanki warrior.\nContempt as armor." },
    { "Shadowheart",CLS_CLERIC, POR_SHADOW,   R5SUB_DOMAIN_OF_MASKS,
      R5RACE_HALF_ELF,  R5BG_ACOLYTE,
      "Cleric of Shar.\nMemories missing." },
    { "Wyll",      CLS_WARLOCK, POR_WYLL,     R5SUB_FIEND,
      R5RACE_HUMAN,     R5BG_FOLK_HERO,
      "Blade of Frontiers.\nA pact regretted." },
    { "Dark Urge", -1,          POR_DURGE,    255,
      R5RACE_DRAGONBORN, R5BG_HAUNTED_ONE,
      "You wake knowing\nsomething used your\nhands, and liked it." },
    { "Custom Tav",-1,          -1,           255,
      R5RACE_NONE,      R5BG_NONE,
      "Someone new.\nChoose your path." },
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
        POR_TAV_FIGHTER, POR_TAV_CLERIC, POR_TAV_BARBARIAN, POR_TAV_DRUID,
        POR_TAV_MONK, POR_TAV_PALADIN, POR_TAV_SORCERER, POR_TAV_WARLOCK };
#else
    static const s8 tavpor[CLS_COUNT] = { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };
#endif
    if (cls < 0 || cls >= CLS_COUNT) cls = 0;
    if (face == ORIG_LAEZEL) {
        L.objt = OBJT_LAEZEL; L.ko = OBJT_LAEZEL_KO; L.pal = 1; L.por = POR_LAEZEL;
    } else if (face == ORIG_SHADOW) {
        L.objt = OBJT_SHADOW; L.ko = OBJT_SHADOW_KO; L.pal = 2; L.por = POR_SHADOW;
    } else if (face == ORIG_WYLL) {
        L.objt = OBJT_WYLL; L.ko = OBJT_WYLL_KO; L.pal = 4; L.por = POR_WYLL;
    } else {
        L.objt = walk[cls]; L.ko = OBJT_HERO_KO; L.pal = 0; L.por = tavpor[cls];
        switch (face) {                       /* origin portraits (sprite stays class) */
            case ORIG_ASTARION: L.por = POR_ASTARION; break;
            case ORIG_GALE:     L.por = POR_GALE;     break;
            case ORIG_KARLACH:  L.por = POR_KARLACH;  break;
            case ORIG_DURGE:    L.por = POR_DURGE;    break;
        }
    }
    return L;
}

int origin_class(int o) { return origins[o].cls; }
const char* origin_name(int o) { return origins[o].name; }
int origin_portrait(int o) { return origins[o].por; }
int origin_race(int o) { return origins[o].race; }
int origin_background(int o) { return origins[o].bg; }
int origin_subclass(int o) { return origins[o].sub; }

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
        if (L.pal == 0) memcpy16(PAL_OBJ, pal_tav_classes[sel], 16);   /* all twelve */
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

/* ------------------------------------------------------------ creation
 * Race -> background -> standard-array screens for custom Tav and the
 * Dark Urge; fixed origins skip the whole flow (their sheets are canon).
 * Layout is generated (tools/ui_screens.py); every screen auto-advances
 * under G_DEMO and takes poked choices (G_DEMO_RACE / G_DEMO_BG /
 * G_DEMO_AB + G_AB_BUF) so scenarios stay deterministic. */

static char* cnum(char* d, int v) {
    if (v < 0) { *d++ = '-'; v = -v; }
    char t[6]; int n = 0;
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *d++ = t[--n];
    return d;
}

/* word-wrap into rows of w cells, never splitting mid-word (the
 * class-blurb lesson); clears every row it owns first */
static void wrap_put(int x, int y, int w, int rows, const char* s, int pal) {
    txt_clear(x, y, w, rows);
    for (int r = 0; r < rows && *s; r++) {
        while (*s == ' ') s++;
        int n = 0, brk = 0;
        while (s[n] && n <= w) {
            if (s[n] == ' ') brk = n;
            n++;
        }
        if (n <= w) brk = n;             /* the whole tail fits */
        else if (!brk) brk = w;          /* single overlong word: hard cut */
        char line[28];
        int cut = brk < 27 ? brk : 27;
        for (int k = 0; k < cut; k++) line[k] = s[k];
        line[cut] = 0;
        txt_put_n(x, y + r, line, pal, w);
        s += brk;
    }
}

static void creation_chrome(void) {
    memset16(SCREENBLOCK(30), 0, 1024);
    memset16(SCREENBLOCK(31), 0, 1024);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG3 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void creation_out(void) {
    obj_hide(OBJ_CURSOR);
    fade_out(10);
    txt_clear(0, 0, 30, 20);
    memset16(SCREENBLOCK(31), 0, 1024);
}

/* card copy, ours (SRD race prose is not quoted); each wraps to at most
 * three rows of SCR_RACESEL_RB0_W -- the host picker test proves the
 * words land whole on the grid */
static const char* const race_blurb[R5RACE_BASE_COUNT] = {
    "Stone-patient folk. They shrug off poison; every level digs in deeper.",
    "Fey blood: sharp eyes, and no magic can sing them to sleep.",
    "Small, unhurried, and impossibly lucky: their worst rolls try again.",
    "A little better at everything, remarkable at nothing. Yet.",
    "Dragon blood waits in the lungs. One day it remembers how to burn.",
    "A quick mind is armor: hostile magic slides off gnomish cunning.",
    "Two worlds, neither home. Charm enough for both, and fey-proof dreams.",
    "Too stubborn to die: once a day, death simply misses. Crits hit harder.",
    "The hells left their mark, and their gift: fire finds no purchase here.",
    "Sword-drilled exiles of a stolen people. They trust steel, and little else.",
};

static const char* const ab_names[6] = { "STR", "DEX", "CON", "INT", "WIS", "CHA" };

/* name + fixed-ASI card for the highlighted entry */
static void race_card(int entry) {
    const R5Race* rr = &r5_races[entry];
    txt_put_n(SCR_RACESEL_RNAME_X, SCR_RACESEL_RNAME_Y, rr->name, 1,
              SCR_RACESEL_RNAME_W);
    txt_clear(SCR_RACESEL_RASI0_X, SCR_RACESEL_RASI0_Y, SCR_RACESEL_RASI0_W, 3);
    int all1 = 1;
    for (int a = 0; a < 6; a++) all1 &= (rr->asi[a] == 1);
    if (all1) {
        txt_put(SCR_RACESEL_RASI0_X, SCR_RACESEL_RASI0_Y, "+1 ALL", 0);
        return;
    }
    int line = 0;
    for (int v = 2; v >= 1; v--)                 /* headline bonus first */
        for (int a = 0; a < 6 && line < 3; a++)
            if (rr->asi[a] == v) {
                char b[8]; char* d = b;
                *d++ = '+'; d = cnum(d, v); *d++ = ' ';
                for (const char* s = ab_names[a]; *s; ) *d++ = *s++;
                *d = 0;
                txt_put(SCR_RACESEL_RASI0_X, SCR_RACESEL_RASI0_Y + line, b, 0);
                line++;
            }
}

static void race_list_draw(const char* const* names, int n) {
    for (int i = 0; i < 10; i++)
        txt_put_n(SCR_RACESEL_R0_X, SCR_RACESEL_R0_Y + i,
                  i < n ? names[i] : "", 0, SCR_RACESEL_R0_W);
}

/* Base-race list; where a base carries named subraces, a second list of
 * them (B returns to the bases). Returns the picked R5RACE_* entry.
 * Demo: G_DEMO_RACE = entry to walk to and take; 0 keeps race-none, the
 * pre-Character-2.0 sheet every legacy scenario runs in. */
int game_race_pick(int origin) {
    creation_chrome();
    fade_in(10);
    scr_racesel();

    const char* base_names[R5RACE_BASE_COUNT];
    for (int b = 0; b < R5RACE_BASE_COUNT; b++)
        base_names[b] = r5_race_bases[b].name;
    race_list_draw(base_names, R5RACE_BASE_COUNT);

    int want = G_DEMO ? G_DEMO_RACE : -1;        /* poked target entry */
    if (want >= R5RACE_COUNT) want = 0;
    int wb = 0, wk = 0;                          /* its base + sub row */
    for (int b = 0; b < R5RACE_BASE_COUNT; b++) {
        const R5RaceBase* rb = &r5_race_bases[b];
        if (want >= rb->first && want < rb->first + rb->n) {
            wb = b;
            wk = want - rb->first;
        }
    }

    int sel = 0;
    if (origin == ORIG_DURGE)                    /* the Urge wakes draconic */
        for (int b = 0; b < R5RACE_BASE_COUNT; b++)
            if (r5_race_bases[b].first == origin_race(ORIG_DURGE)) sel = b;
    int sub = -1;                    /* -1 = base list, else base index */
    int picked = R5RACE_NONE;
    int demo_hold = G_DEMO ? 45 : 0;

    for (;;) {
        const R5RaceBase* rb = &r5_race_bases[sub < 0 ? sel : sub];
        int n = sub < 0 ? R5RACE_BASE_COUNT : rb->n;
        int entry = sub < 0 ? r5_race_bases[sel].first : rb->first + sel;
        race_card(entry);
        wrap_put(SCR_RACESEL_RB0_X, SCR_RACESEL_RB0_Y, SCR_RACESEL_RB0_W, 3,
                 race_blurb[sub < 0 ? sel : sub], 2);
        int done = 0;
        for (;;) {
            obj_set(OBJ_CURSOR, SCR_RACESEL_R0_X * 8 - 14,
                    (SCR_RACESEL_R0_Y + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
            frame();
            if (demo_hold) {
                if (--demo_hold == 0) {
                    if (want <= 0) { done = 2; break; }    /* race-none out */
                    int target = sub < 0 ? wb : wk;
                    if (sel < target) { sel++; demo_hold = 8; break; }
                    if (sel > target) { sel--; demo_hold = 8; break; }
                    sfx_play(SFX_CONFIRM); demo_hold = 20; done = 1; break;
                }
                continue;
            }
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_DOWN && sel < n - 1) { sel++; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_A) { sfx_play(SFX_CONFIRM); done = 1; break; }
            if ((k & KEY_B) && sub >= 0) { sfx_play(SFX_CANCEL); done = 3; break; }
        }
        if (done == 2) break;                              /* demo: keep none */
        if (done == 3) {                                   /* back to bases */
            sel = sub;
            sub = -1;
            race_list_draw(base_names, R5RACE_BASE_COUNT);
            continue;
        }
        if (done == 1) {
            if (sub < 0 && r5_race_bases[sel].sub) {       /* descend */
                sub = sel;
                const char* sub_names[4];
                for (int k2 = 0; k2 < r5_race_bases[sub].n && k2 < 4; k2++)
                    sub_names[k2] = r5_races[r5_race_bases[sub].first + k2].name;
                race_list_draw(sub_names, r5_race_bases[sub].n);
                sel = 0;
                continue;
            }
            picked = entry;
            break;
        }
    }
    creation_out();
    return picked;
}

/* Background list; the strip below names the two granted skills and the
 * flavor line. Returns R5BG_*, or -1 on B (back to the race picker).
 * Demo: G_DEMO_BG = background to take; 0 keeps none. */
int game_bg_pick(int origin) {
    creation_chrome();
    fade_in(10);
    scr_bgsel();
    for (int i = 0; i + 1 < R5BG_COUNT; i++)
        txt_put_n(SCR_BGSEL_G0_X, SCR_BGSEL_G0_Y + i,
                  r5_backgrounds[i + 1].name, 0, SCR_BGSEL_G0_W);

    int want = G_DEMO ? G_DEMO_BG : -1;
    if (want >= R5BG_COUNT) want = 0;
    int sel = 0;
    if (origin == ORIG_DURGE)                    /* haunted by default */
        sel = origin_background(ORIG_DURGE) - 1;
    int demo_hold = G_DEMO ? 45 : 0;
    int picked = R5BG_NONE;

    for (;;) {
        const R5Background* bg = &r5_backgrounds[sel + 1];
        {   /* "Insight, Religion" from the skill mask */
            char b[32]; char* d = b;
            for (int s = 0; s < SK_COUNT; s++)
                if (bg->skills & (1u << s)) {
                    if (d != b) { *d++ = ','; *d++ = ' '; }
                    for (const char* p = r5_skill_name[s]; *p; ) *d++ = *p++;
                }
            *d = 0;
            txt_put_n(SCR_BGSEL_GSK_X, SCR_BGSEL_GSK_Y, b, 1, SCR_BGSEL_GSK_W);
        }
        wrap_put(SCR_BGSEL_GB0_X, SCR_BGSEL_GB0_Y, SCR_BGSEL_GB0_W, 3,
                 bg->blurb, 2);
        int done = 0;
        for (;;) {
            obj_set(OBJ_CURSOR, SCR_BGSEL_G0_X * 8 - 14,
                    (SCR_BGSEL_G0_Y + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
            frame();
            if (demo_hold) {
                if (--demo_hold == 0) {
                    if (want <= 0) { done = 2; break; }    /* keep none */
                    if (sel < want - 1) { sel++; demo_hold = 8; break; }
                    if (sel > want - 1) { sel--; demo_hold = 8; break; }
                    sfx_play(SFX_CONFIRM); done = 1; break;
                }
                continue;
            }
            u16 k = key_hit();
            if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_DOWN && sel < R5BG_COUNT - 2) { sel++; sfx_play(SFX_CURSOR); break; }
            if (k & KEY_A) { sfx_play(SFX_CONFIRM); done = 1; break; }
            if (k & KEY_B) { sfx_play(SFX_CANCEL); done = 3; break; }
        }
        if (done == 1) picked = sel + 1;
        if (done == 3) picked = -1;
        if (done) break;
    }
    creation_out();
    return picked;
}

/* Standard-array assignment: the class preset (that array permuted) is
 * offered; A takes a score, A again swaps it into place; racial ASIs and
 * the resulting mods show live. Returns 1 on Begin, 0 on B (back).
 * Demo: G_DEMO_AB = 1 arranges from G_AB_BUF (must be the preset
 * permuted -- anything else is rejected and logged), then confirms. */
int game_stats_assign(int cls, int race, s8 out[6]) {
    creation_chrome();
    fade_in(10);
    scr_statsel();
    txt_put_n(SCR_STATSEL_GO_X, SCR_STATSEL_GO_Y, "Begin", 1, SCR_STATSEL_GO_W);

    s8 arr[6];
    for (int a = 0; a < 6; a++) arr[a] = (s8)party5_preset(cls, a);
    if (G_DEMO && G_DEMO_AB == 1) {
        s8 pk[6], sa[6], sb[6];
        for (int a = 0; a < 6; a++) { pk[a] = (s8)G_AB_BUF[a]; sa[a] = pk[a]; sb[a] = arr[a]; }
        for (int i = 0; i < 6; i++)              /* sort both, compare */
            for (int j = i + 1; j < 6; j++) {
                if (sa[j] < sa[i]) { s8 t = sa[i]; sa[i] = sa[j]; sa[j] = t; }
                if (sb[j] < sb[i]) { s8 t = sb[i]; sb[i] = sb[j]; sb[j] = t; }
            }
        int same = 1;
        for (int a = 0; a < 6; a++) same &= (sa[a] == sb[a]);
        if (same) for (int a = 0; a < 6; a++) arr[a] = pk[a];
        else mgba_log("spread rejected: not the class array");
    }

    const R5Race* rr = &r5_races[race];
    int sel = 0, grab = -1, dirty = 1, ret = -1;
    int demo_hold = G_DEMO ? 60 : 0;
    while (ret < 0) {
        if (dirty) {
            for (int a = 0; a < 6; a++) {
                char b[24]; char* d = b;
                for (const char* s = ab_names[a]; *s; ) *d++ = *s++;
                *d++ = ' ';
                if (arr[a] < 10) *d++ = ' ';
                d = cnum(d, arr[a]);
                *d++ = ' '; *d++ = ' ';
                if (rr->asi[a]) { *d++ = '+'; d = cnum(d, rr->asi[a]); }
                else { *d++ = ' '; *d++ = ' '; }
                *d++ = ' '; *d++ = ' ';
                int tot = arr[a] + rr->asi[a];
                if (tot < 10) *d++ = ' ';
                d = cnum(d, tot);
                int m = r5_mod(tot);
                *d++ = ' '; *d++ = '(';
                *d++ = (char)(m < 0 ? '-' : '+');
                d = cnum(d, m < 0 ? -m : m);
                *d++ = ')'; *d = 0;
                txt_put_n(SCR_STATSEL_A0_X, SCR_STATSEL_A0_Y + a, b,
                          grab == a ? 1 : 0, SCR_STATSEL_A0_W);
            }
            dirty = 0;
        }
        int cy = sel < 6 ? SCR_STATSEL_A0_Y + sel : SCR_STATSEL_GO_Y;
        obj_set(OBJ_CURSOR, SCR_STATSEL_A0_X * 8 - 14, cy * 8 - 4, 1,
                OBJT_HAND, 7, 0);
        frame();
        if (demo_hold) {
            if (--demo_hold == 0) { sfx_play(SFX_CONFIRM); ret = 1; }
            continue;
        }
        u16 k = key_hit();
        if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); }
        if (k & KEY_DOWN && sel < 6) { sel++; sfx_play(SFX_CURSOR); }
        if (k & KEY_A) {
            if (sel == 6) {
                if (grab < 0) { sfx_play(SFX_CONFIRM); ret = 1; }
                else sfx_play(SFX_CANCEL);       /* set the score down first */
            } else if (grab < 0) {
                grab = sel; dirty = 1; sfx_play(SFX_CONFIRM);
            } else {
                s8 t = arr[grab]; arr[grab] = arr[sel]; arr[sel] = t;
                grab = -1; dirty = 1; sfx_play(SFX_CONFIRM);
            }
        }
        if (k & KEY_B) {
            if (grab >= 0) { grab = -1; dirty = 1; sfx_play(SFX_CANCEL); }
            else { sfx_play(SFX_CANCEL); ret = 0; }
        }
    }
    if (ret == 1)
        for (int a = 0; a < 6; a++) out[a] = arr[a];
    creation_out();
    return ret;
}

/* The whole creation flow behind class + origin: fixed origins take
 * their canon sheet and name; custom Tav and the Urge walk race ->
 * background -> array -> name (B backs out one step). Ends with the
 * party built and the 5e twin refreshed. */
void game_creation(int cls, int origin) {
    char nm[8];
    G.origin = (u8)origin;
    int race = R5RACE_NONE, bg = R5BG_NONE;
    s8 ab6[6] = { 0, 0, 0, 0, 0, 0 };
    if (origin_class(origin) >= 0) {             /* fixed sheet: skip */
        const char* on = origin_name(origin);
        int i = 0;
        for (; on[i] && i < 7; i++) nm[i] = on[i];
        nm[i] = 0;
    } else {
        for (int step = 0; step < 3; ) {
            if (step == 0) {
                race = game_race_pick(origin);
                step = 1;
            } else if (step == 1) {
                int r = game_bg_pick(origin);
                if (r < 0) step = 0;
                else { bg = r; step = 2; }
            } else {
                step = game_stats_assign(cls, race, ab6) ? 3 : 1;
            }
        }
        game_name_entry(nm);
    }
    party_init(cls, nm);
    if (origin_class(origin) < 0)
        party_set_identity(race, bg, ab6);
    party5_refresh_all();
    /* every class carries a tav palette now (mkassets asserts all twelve),
     * so bank 0 always wears the played class -- no fallback-blue for the
     * eight that once lacked one. The bound keeps the index in the
     * [CLS_COUNT] table (UBSan once caught a read past the old [4]). */
    if (cls >= 0 && cls < CLS_COUNT) memcpy16(PAL_OBJ, pal_tav_classes[cls], 16);
    mgba_logf("create race=%d bg=%d ab=%d/%d/%d/%d/%d/%d",
              G.pm[0].race, G.pm[0].background,
              party5[0].ab[0], party5[0].ab[1], party5[0].ab[2],
              party5[0].ab[3], party5[0].ab[4], party5[0].ab[5]);
}
