/* FF4-style text and window engine. Text on BG0 (SB30), chrome on BG1 (SB31). */
#include "gba.h"
#include "assets.h"
#include "engine.h"

#define TXT ((vu16*)SCREENBLOCK(30))
#define WIN ((vu16*)SCREENBLOCK(31))
#define ME(t, h, v, p) ((u16)((t) | ((h) << 10) | ((v) << 11) | ((p) << 12)))

/* window tile block offsets (see tools/mkassets.py window_tiles) */
#define W_CT (TILE_WIN + 0)
#define W_CB (TILE_WIN + 1)
#define W_ET (TILE_WIN + 2)
#define W_EB (TILE_WIN + 3)
#define W_EL(s) (TILE_WIN + 4 + (s))
#define W_IN(s) (TILE_WIN + 9 + (s))
#define W_BLACK (TILE_WIN + 14)
#define GLYPH_MORE 95

static int drift_on;
static u32 drift_t;
void sky_autodrift(int on) { drift_on = on; }

void frame(void) {
    g_wd = 0;                          /* pet the hang watchdog */
    if (G_PANIC_TEST == 1) { G_PANIC_TEST = 2; panic("poked (crash-screen test)", 0); }
    vsync();
    oam_flush();
    key_poll();
    if (drift_on) { drift_t++; REG_BG3HOFS = (u16)(drift_t >> 3); }
}

void txt_put(int x, int y, const char* s, int pal) {
    vu16* p = TXT + y * 32 + x;
    while (*s && x < 30) { *p++ = ME((u8)*s++ - 32, 0, 0, pal); x++; }
}

/* write exactly w cells: truncates long text, space-pads short (so a slot
 * redraw always fully overwrites its previous contents) */
void txt_put_n(int x, int y, const char* s, int pal, int w) {
    vu16* p = TXT + y * 32 + x;
    for (int i = 0; i < w && x + i < 30; i++) {
        char c = *s ? *s++ : ' ';
        *p++ = ME((u8)c - 32, 0, 0, pal);
    }
}

void txt_clear(int x, int y, int w, int h) {
    for (int j = 0; j < h; j++)
        memset16(TXT + (y + j) * 32 + x, 0, (u32)w);
}

static int win_shade(int row, int h) {   /* gradient index 0..4 for interior row */
    if (h <= 3) return 2;
    int s = (row - 1) * 4 / (h - 3);
    return s > 4 ? 4 : s;
}

void win_draw(int x, int y, int w, int h) {
    for (int j = 0; j < h; j++) {
        vu16* p = WIN + (y + j) * 32 + x;
        if (j == 0) {
            p[0] = ME(W_CT, 0, 0, 0);
            for (int i = 1; i < w - 1; i++) p[i] = ME(W_ET, 0, 0, 0);
            p[w - 1] = ME(W_CT, 1, 0, 0);
        } else if (j == h - 1) {
            p[0] = ME(W_CB, 0, 0, 0);
            for (int i = 1; i < w - 1; i++) p[i] = ME(W_EB, 0, 0, 0);
            p[w - 1] = ME(W_CB, 1, 0, 0);
        } else {
            int s = win_shade(j, h);
            p[0] = ME(W_EL(s), 0, 0, 0);
            for (int i = 1; i < w - 1; i++) p[i] = ME(W_IN(s), 0, 0, 0);
            p[w - 1] = ME(W_EL(s), 1, 0, 0);
        }
    }
}

void win_clear(int x, int y, int w, int h) {
    for (int j = 0; j < h; j++)
        memset16(WIN + (y + j) * 32 + x, 0, (u32)w);
    txt_clear(x, y, w, h);
}

/* ---- dialogue box: rows 0..5, text area (2,1)..(27,4) ---- */

#define DLG_Y 0
#define DLG_H 6
#define DLG_TX 2
#define DLG_TY 1
#define DLG_W 26
#define DLG_ROWS 4

static int dlg_on, cx, cy;
static int dlg_por = -1, dlg_por_drawn = -1;   /* active portrait id */
static int dlg_inset;                          /* text x-inset for portrait */

/* portrait tiles staged in CB0 right after the UI tiles */
#define POR_VRAM_TILE UI_TILE_COUNT

static void dlg_draw_portrait(void) {
#if PORTRAIT_COUNT > 0
    if (dlg_por >= 0 && dlg_por != dlg_por_drawn) {
        extern const u16 portrait_tiles[];
        memcpy16((vu16*)((u32)CHARBLOCK(0) + POR_VRAM_TILE * 32),
                 &portrait_tiles[dlg_por * 36 * 16], 36 * 16);
        for (int r = 0; r < 6; r++)
            for (int c = 0; c < 6; c++)
                TXT[(DLG_Y + r) * 32 + 1 + c] =
                    ME(POR_VRAM_TILE + r * 6 + c, 0, 0, 5);
    }
    if (dlg_por < 0 && dlg_por_drawn >= 0) {
        /* portrait removed: clear its text-layer cells */
        txt_clear(1, DLG_Y, 6, 6);
    }
    dlg_por_drawn = dlg_por;
#endif
    dlg_inset = (dlg_por >= 0) ? 6 : 0;
}

/* Clear the dialog's writable interior and home the cursor. Non-inset clears
 * the FULL interior (x=1..28), not just the text region (x=2..27): the combat
 * message bar writes its text at x=1, and that stale leading char would
 * otherwise survive under a dialog opened over the combat scene -- the
 * "ATAV walks a new path" bug ('A' left over from "A new level!"). */
static void dlg_clear_text(void) {
    if (dlg_inset) txt_clear(7, DLG_TY, 21, DLG_ROWS);   /* portrait holds x=1..6 */
    else           txt_clear(1, DLG_TY, 28, DLG_ROWS);
    cx = cy = 0;
}

void dlg_set_portrait(int id) {
    dlg_por = id;
    if (dlg_on) {
        dlg_draw_portrait();
        dlg_clear_text();
    }
}

void dlg_open(void) {
    G_FIELD_IDLE = 0;
    if (!dlg_on) {
        win_draw(0, DLG_Y, 30, DLG_H);
        dlg_por_drawn = -1;
        dlg_draw_portrait();
        dlg_on = 1;
    }
    dlg_clear_text();
}

void dlg_close(void) {
    if (dlg_on) { win_clear(0, DLG_Y, 30, DLG_H); dlg_on = 0; }
    dlg_por = -1;
    dlg_por_drawn = -1;
    dlg_inset = 0;
}

int dlg_is_open(void) { return dlg_on; }   /* the field die draws over the box */

static void dlg_wait_a(int marker) {
    if (marker) TXT[(DLG_TY + DLG_ROWS - 1) * 32 + 28] = ME(GLYPH_MORE, 0, 0, 0);
    int t = 0;
    for (;;) {
        frame();
        if (key_hit() & KEY_A) break;
        t++;
        if (G_DEMO && t >= 40) break;   /* attract mode: auto-advance */
        if (marker)
            TXT[(DLG_TY + DLG_ROWS - 1) * 32 + 28] =
                (t & 16) ? 0 : ME(GLYPH_MORE, 0, 0, 0);
    }
    TXT[(DLG_TY + DLG_ROWS - 1) * 32 + 28] = 0;
}

static void dlg_page(void) {
    dlg_wait_a(1);
    dlg_clear_text();
}

static void dlg_newline(void) {
    cx = 0;
    if (++cy >= DLG_ROWS) dlg_page();
}

static void dlg_putc(char c, int pal) {
    if (c == '\n') { dlg_newline(); return; }
    if (cx >= DLG_W - dlg_inset) dlg_newline();
    TXT[(DLG_TY + cy) * 32 + DLG_TX + dlg_inset + cx] = ME((u8)c - 32, 0, 0, pal);
    cx++;
    if (G_DEMO) return;                             /* instant text in attract mode */
    if (!(key_state() & (KEY_A | KEY_B))) frame();  /* typewriter; hold A/B = fast */
}

/* print with word wrap; blocks until done */
void dlg_print(const char* s, int pal) {
    if (!dlg_on) dlg_open();
    while (*s) {
        if (*s == ' ') {                     /* wrap check at word start */
            int wl = 0;
            while (s[1 + wl] && s[1 + wl] != ' ' && s[1 + wl] != '\n') wl++;
            if (cx + 1 + wl > DLG_W - dlg_inset) { dlg_newline(); s++; continue; }
        }
        dlg_putc(*s++, pal);
    }
}

void say(const char* s) {
    if (dlg_por >= 0) dlg_set_portrait(-1);
    dlg_open();
    dlg_print(s, 0);
    dlg_wait_a(1);
}

void say_p(int portrait, const char* s) {
    dlg_set_portrait(portrait);
    dlg_open();
    dlg_print(s, 0);
    dlg_wait_a(1);
}

/* say without clearing afterward; caller chains a choice */
void say_keep(const char* s) {
    dlg_open();
    dlg_print(s, 0);
}

int choose(int n, const char* const* opts) {
    int wmax = 0;
    for (int i = 0; i < n; i++) {
        int l = 0; while (opts[i][l]) l++;
        if (l > wmax) wmax = l;
    }
    int w = wmax + 4, h = n + 2;
    int x = 29 - w, y = DLG_Y + DLG_H;
    win_draw(x, y, w, h);
    for (int i = 0; i < n; i++) txt_put(x + 3, y + 1 + i, opts[i], 0);

    static int demo_idx;
    int sel = 0;
    if (G_DEMO) {
        int pick = G_CHOICE_BUF[demo_idx++];
        if (pick >= n) pick = n - 1;
        for (int i = 0; i < 24; i++) {   /* animate to the pick for the recording */
            if (i == 12) sel = pick;
            obj_set(OBJ_CURSOR, x * 8 - 6, (y + 1 + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
            frame();
        }
        sfx_play(SFX_CONFIRM);
        obj_hide(OBJ_CURSOR);
        win_clear(x, y, w, h);
        mgba_logf("choose->%d", pick);
        return pick;
    }
    for (;;) {
        obj_set(OBJ_CURSOR, x * 8 - 6, (y + 1 + sel) * 8 - 4, 1, OBJT_HAND, 7, 0);
        frame();
        u16 k = key_hit();
        if (k & KEY_UP && sel > 0) { sel--; sfx_play(SFX_CURSOR); }
        if (k & KEY_DOWN && sel < n - 1) { sel++; sfx_play(SFX_CURSOR); }
        if (k & KEY_A) { sfx_play(SFX_CONFIRM); break; }
    }
    obj_hide(OBJ_CURSOR);
    win_clear(x, y, w, h);
    mgba_logf("choose->%d", sel);
    return sel;
}
