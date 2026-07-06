/* Fake engine: everything below the game-logic line, reimplemented for the
 * host so the REAL menu/battle/creation code runs natively under ASan/UBSan.
 *
 * Faithfulness rules of thumb:
 *   - Same observable semantics as the real engine where game code can see
 *     them (txt_put clamps at column 30, win_clear also clears text, npc
 *     slots are reused when GONE, key_hit is an edge off frame()'s poll).
 *   - Everything else records instead of rendering.
 *   - Out-of-contract calls (text off the tilemap, bad obj slots, unmapped
 *     bus addresses) count as invariant violations and fail the test.
 */
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_engine.h"
#include "engine.h"
#include "audio.h"
#include "field.h"
#include "game.h"
#include "party5.h"

/* ------------------------------------------------------------ fail plumbing */

char sim_fail_msg[512];
static jmp_buf sim_jmp;
static int sim_jmp_armed;

void sim_fail(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sim_fail_msg, sizeof sim_fail_msg, fmt, ap);
    va_end(ap);
    if (sim_jmp_armed) longjmp(sim_jmp, 1);
    fprintf(stderr, "sim_fail with no guard: %s\n", sim_fail_msg);
    abort();
}

/* ------------------------------------------------------------ fake GBA bus */

static u8 ewram[0x40000] __attribute__((aligned(16)));
static u8 iwram[0x8000]  __attribute__((aligned(16)));
static u8 ioreg[0x800]   __attribute__((aligned(16)));
static u8 mgbareg[0x200] __attribute__((aligned(16)));
static u8 palram[0x400]  __attribute__((aligned(16)));
static u8 vram[0x18000]  __attribute__((aligned(16)));
static u8 oamram[0x400]  __attribute__((aligned(16)));

void* host_map(u32 a, u32 len) {
    u32 off = a & 0xFFFFFFu;
    switch (a >> 24) {
        case 2: if (off + len <= sizeof ewram) return ewram + off; break;
        case 3: if (off + len <= sizeof iwram) return iwram + off; break;
        case 4:
            if (off >= 0xFFF600 && off + len <= 0xFFF800)
                return mgbareg + (off - 0xFFF600);
            if (off + len <= sizeof ioreg) return ioreg + off;
            break;
        case 5: if (off + len <= sizeof palram) return palram + off; break;
        case 6: if (off + len <= sizeof vram) return vram + off; break;
        case 7: if (off + len <= sizeof oamram) return oamram + off; break;
        default: break;
    }
    sim_fail("host_map: unmapped GBA address 0x%lx (+%lu)",
             (unsigned long)a, (unsigned long)len);
}

/* ------------------------------------------------------------ frames + keys */

volatile u32 g_frame;
volatile u32 g_irq_pc, g_wd;

static struct {
    u32 frames, budget;
    int pass_on_budget;
    u16 cur, prev;
} clk;

#define QMAX 262144
#define SNAPF 0x4000u
static u16 keyq[QMAX];
static int qlen, qhead;
static u16 (*keygen)(u32 frame);

static void take_snapshot(void);

static void tick(int with_input) {
    clk.frames++;
    g_frame = clk.frames;
    if (clk.frames > clk.budget) {
        if (clk.pass_on_budget && sim_jmp_armed) longjmp(sim_jmp, 2);
        sim_fail("frame budget exceeded (%lu frames): game stuck waiting?",
                 (unsigned long)clk.budget);
    }
    u16 m = 0;
    if (with_input) {
        if (keygen) m = keygen(clk.frames);
        else if (qhead < qlen) {
            m = keyq[qhead++];
            if (m & SNAPF) { take_snapshot(); m = 0; }
        }
    }
    clk.prev = clk.cur;
    clk.cur = m & KEY_ANY;
}

void frame(void) { tick(1); }
void vsync(void) { tick(0); }   /* real vsync doesn't poll keys */
void key_poll(void) {}          /* the poll happens in frame(), as on device */
u16 key_state(void) { return clk.cur; }
u16 key_hit(void) { return (u16)(clk.cur & (u16)~clk.prev); }

u32 sim_frames(void) { return clk.frames; }
void sim_budget(u32 frames, int pass_on_budget) {
    clk.budget = frames;
    clk.pass_on_budget = pass_on_budget;
}

void script_gen(u16 (*fn)(u32 frame)) { keygen = fn; }
int script_left(void) { return qlen - qhead; }

static void qpush(u16 m) {
    if (qlen >= QMAX) sim_fail("script too long");
    keyq[qlen++] = m;
}

void script_keys(const char* s) {
    while (*s) {
        while (*s == ' ') s++;
        if (!*s) break;
        char tok[16];
        int n = 0;
        while (*s && *s != ' ' && n < 15) tok[n++] = *s++;
        tok[n] = 0;
        u16 m = 0;
        if      (!strcmp(tok, "A")) m = KEY_A;
        else if (!strcmp(tok, "B")) m = KEY_B;
        else if (!strcmp(tok, "UP")) m = KEY_UP;
        else if (!strcmp(tok, "DOWN")) m = KEY_DOWN;
        else if (!strcmp(tok, "LEFT")) m = KEY_LEFT;
        else if (!strcmp(tok, "RIGHT")) m = KEY_RIGHT;
        else if (!strcmp(tok, "START")) m = KEY_START;
        else if (!strcmp(tok, "SELECT")) m = KEY_SELECT;
        else if (!strcmp(tok, "L")) m = KEY_L;
        else if (!strcmp(tok, "R")) m = KEY_R;
        else if (!strcmp(tok, ".")) { qpush(0); continue; }
        else if (!strcmp(tok, "SNAP")) { qpush(SNAPF); continue; }
        else if (tok[0] == 'W') {
            int w = atoi(tok + 1);
            while (w-- > 0) qpush(0);
            continue;
        } else sim_fail("script_keys: bad token '%s'", tok);
        qpush(m);   /* pressed for one frame... */
        qpush(0);   /* ...then released, so key_hit sees an edge next time */
    }
}

/* ------------------------------------------------------------ violations */

int sim_violations;
char sim_violation_msg[256];

static void viol(const char* fmt, ...) {
    sim_violations++;
    if (!sim_violation_msg[0]) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(sim_violation_msg, sizeof sim_violation_msg, fmt, ap);
        va_end(ap);
    }
}

/* ------------------------------------------------------------ text grid */

static char grid[GRID_H][GRID_W];
static char rowbuf[GRID_W + 1];

#define MAXSNAP 16
static char snaps[MAXSNAP][GRID_H][GRID_W];
static int nsnap;

static void take_snapshot(void) {
    if (nsnap < MAXSNAP) memcpy(snaps[nsnap++], grid, sizeof grid);
}
int snap_count(void) { return nsnap; }

static const char* row_str(const char g[GRID_H][GRID_W], int y) {
    if (y < 0 || y >= GRID_H) return "";
    memcpy(rowbuf, g[y], GRID_W);
    rowbuf[GRID_W] = 0;
    for (int i = GRID_W - 1; i >= 0 && rowbuf[i] == ' '; i--) rowbuf[i] = 0;
    return rowbuf;
}
const char* grid_row(int y) { return row_str(grid, y); }
const char* snap_row(int s, int y) {
    return (s >= 0 && s < nsnap) ? row_str(snaps[s], y) : "";
}

static int g_contains(const char g[GRID_H][GRID_W], const char* s) {
    char b[GRID_W + 1];
    for (int y = 0; y < GRID_H; y++) {
        memcpy(b, g[y], GRID_W);
        b[GRID_W] = 0;
        if (strstr(b, s)) return 1;
    }
    return 0;
}
int grid_contains(const char* s) { return g_contains(grid, s); }
int snap_contains(int i, const char* s) {
    return (i >= 0 && i < nsnap) && g_contains(snaps[i], s);
}

static void g_dump(const char g[GRID_H][GRID_W]) {
    for (int y = 0; y < 20; y++) fprintf(stderr, "|%.30s|\n", g[y]);
}
void grid_dump(void) { g_dump(grid); }

static void put_cell(int x, int y, char c) {
    if ((unsigned char)c < 32 || (unsigned char)c > 126) {
        viol("txt: non-printable char %d at (%d,%d)", (unsigned char)c, x, y);
        c = '?';
    }
    grid[y][x] = c;
}

void txt_put(int x, int y, const char* s, int pal) {
    (void)pal;
    if (y < 0 || y >= GRID_H || x < 0) {
        viol("txt_put off tilemap: (%d,%d) \"%s\"", x, y, s);
        return;
    }
    while (*s && x < GRID_W) put_cell(x++, y, *s++);
}

void txt_put_n(int x, int y, const char* s, int pal, int w) {
    (void)pal;
    if (y < 0 || y >= GRID_H || x < 0) {
        viol("txt_put_n off tilemap: (%d,%d) \"%s\"", x, y, s);
        return;
    }
    for (int i = 0; i < w && x + i < GRID_W; i++)
        put_cell(x + i, y, *s ? *s++ : ' ');
}

void txt_clear(int x, int y, int w, int h) {
    if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > GRID_W || y + h > GRID_H) {
        viol("txt_clear off tilemap: (%d,%d) %dx%d", x, y, w, h);
        return;
    }
    for (int j = 0; j < h; j++)
        memset(&grid[y + j][x], ' ', (size_t)w);
}

static int win_draws;
void win_draw(int x, int y, int w, int h) {
    win_draws++;
    if (x < 0 || y < 0 || w < 2 || h < 2 || x + w > GRID_W || y + h > GRID_H)
        viol("win_draw off tilemap: (%d,%d) %dx%d", x, y, w, h);
}
void win_clear(int x, int y, int w, int h) {
    if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > GRID_W || y + h > GRID_H) {
        viol("win_clear off tilemap: (%d,%d) %dx%d", x, y, w, h);
        return;
    }
    txt_clear(x, y, w, h);   /* real win_clear clears the text layer too */
}

/* ------------------------------------------------------------ dialog */

static int dlg_open_p;
void dlg_open(void) { G_FIELD_IDLE = 0; dlg_open_p = 1; }
void dlg_close(void) { dlg_open_p = 0; }
void dlg_set_portrait(int id) { (void)id; }
void dlg_print(const char* s, int pal) { (void)pal; mgba_logf("say: %s", s); }
void say(const char* s) { dlg_open(); dlg_print(s, 0); }
void say_p(int por, const char* s) { (void)por; say(s); }
void say_keep(const char* s) { dlg_open(); dlg_print(s, 0); }

static u8 choiceq[32];
static int nchoice, ichoice;
int choose(int n, const char* const* opts) {
    (void)opts;
    int pick = ichoice < nchoice ? choiceq[ichoice++] : 0;
    if (pick >= n) pick = n - 1;
    mgba_logf("choose->%d", pick);
    return pick;
}

/* ------------------------------------------------------------ oam */

static SimObj objs[128];
const SimObj* sim_obj(int i) { return &objs[i & 127]; }

void oam_init(void) { memset(objs, 0, sizeof objs); }
void oam_flush(void) {}
void obj_set(int i, int x, int y, int shapesize, int tile, int pal, int prio) {
    if (i < 0 || i >= 128 || shapesize < 0 || shapesize > 8 ||
        pal < 0 || pal > 15 || tile < 0 || tile >= 1024) {
        viol("obj_set bad args: i=%d shape=%d tile=%d pal=%d", i, shapesize, tile, pal);
        return;
    }
    objs[i] = (SimObj){ x, y, shapesize, tile, pal, prio, 1 };
}
void obj_hide(int i) {
    if (i < 0 || i >= 128) { viol("obj_hide bad slot %d", i); return; }
    objs[i].shown = 0;
}
void obj_flip(int i, int h, int v) { (void)i; (void)h; (void)v; }

/* ------------------------------------------------------------ video/audio */

void vid_init_ui(void) {}
void sky_autodrift(int on) { (void)on; }
void fade_out(int frames) { while (frames-- >= 0) tick(0); }
void fade_in(int frames) { while (frames-- >= 0) tick(0); }

int sim_last_music = -1;
static int sfx_counts[8];
void audio_init(void) {}
void audio_tick(void) {}
void music_play(const Song* s) { (void)s; }
void music_stop(void) { sim_last_music = -1; }
void music(int id) { sim_last_music = id; }
int music_row(void) { return (int)(clk.frames & 1023); }
void sfx_play(int id) { if (id >= 0 && id < 8) sfx_counts[id]++; }
void sfx_noise(int frames) { (void)frames; }

/* ------------------------------------------------------------ util */

void gba_init(void) {}

void memcpy16(vu16* dst, const u16* src, u32 n) { while (n--) *dst++ = *src++; }
void memset16(vu16* dst, u16 val, u32 n) { while (n--) *dst++ = val; }

static u32 rngs = 0x1234567u;
void sim_seed(u32 seed) { rngs = seed ? (seed & 0xFFFFFFFFu) : 1; }
u32 rnd(void) {   /* same xorshift as util.c, held to 32 bits */
    u32 x = rngs & 0xFFFFFFFFu;
    x ^= (x << 13) & 0xFFFFFFFFu;
    x ^= x >> 17;
    x ^= (x << 5) & 0xFFFFFFFFu;
    rngs = x & 0xFFFFFFFFu;
    return rngs;
}
int rnd_range(int n) { return n > 0 ? (int)(rnd() % (u32)n) : 0; }

/* ------------------------------------------------------------ log capture */

#define LOGCAP 262144
static char logbuf[LOGCAP];
static size_t loglen;

void mgba_log(const char* s) {
    size_t n = strlen(s);
    if (loglen + n + 2 < LOGCAP) {
        memcpy(logbuf + loglen, s, n);
        loglen += n;
        logbuf[loglen++] = '\n';
        logbuf[loglen] = 0;
    }
    if (getenv("SIM_VERBOSE")) fprintf(stderr, "[log] %s\n", s);
}

void mgba_logf(const char* fmt, ...) {   /* mirrors util.c: %d %x %s only */
    char buf[256];
    char* p = buf;
    va_list ap;
    va_start(ap, fmt);
    for (const char* f = fmt; *f && p < buf + 240; f++) {
        if (*f != '%') { *p++ = *f; continue; }
        f++;
        if (*f == 'd') p += snprintf(p, 16, "%d", va_arg(ap, int));
        else if (*f == 'x') p += snprintf(p, 16, "%x", va_arg(ap, unsigned));
        else if (*f == 's') {
            const char* s = va_arg(ap, const char*);
            while (*s && p < buf + 240) *p++ = *s++;
        } else *p++ = *f;
    }
    va_end(ap);
    *p = 0;
    mgba_log(buf);
}

int log_contains(const char* s) { return strstr(logbuf, s) != 0; }
void log_dump(void) { fputs(logbuf, stderr); }

/* ------------------------------------------------------------ panic/crumbs */

void panic(const char* why, u32 pc) {
    sim_fail("PANIC: %s (pc/line %lu)", why, (unsigned long)pc);
}
void crumb(int code, int arg) {
    if (getenv("SIM_CRUMBS")) fprintf(stderr, "[crumb] %d:%d f=%lu\n",
                                      code, arg, (unsigned long)clk.frames);
}

/* ------------------------------------------------------------ field fake */

Npc npcs[FMAX_NPC];
static int nnpc;
static int ppx = 120, ppy = 80;
static int cam_ov, cam_x, cam_y;
static int player_hidden;
static u16 hero_objt;
static u8 hero_pal;

void sim_player_at(int x, int y) { ppx = x; ppy = y; }

int field_add_npc(int mx, int my, int objt, int pal, int face, int flags) {
    int i;
    for (i = 0; i < nnpc; i++)
        if (npcs[i].flags & NPC_GONE) break;
    if (i == nnpc) {
        if (nnpc >= FMAX_NPC) { mgba_log("npc overflow!"); return -1; }
        nnpc++;
    }
    Npc* n = &npcs[i];
    n->x = (s16)(mx * 16); n->y = (s16)(my * 16);
    n->hx = n->x; n->hy = n->y;
    n->objt = (u16)objt; n->pal = (u8)pal; n->face = (u8)face;
    n->flags = (u8)flags; n->id = (u8)i;
    n->aggro_r = 0; n->chasing = 0;
    return i;
}
void field_remove_npc(int idx) {
    if (idx < 0 || idx >= FMAX_NPC) { viol("field_remove_npc bad idx %d", idx); return; }
    npcs[idx].flags |= NPC_GONE;
}
void field_set_hero(int objt, int pal) { hero_objt = (u16)objt; hero_pal = (u8)pal; }
void field_draw(void) {}
void field_hide_player(int on) { player_hidden = on; }
int field_player_x(void) { return ppx; }
int field_player_y(void) { return ppy; }
int field_player_mx(void) { return (ppx + 8) / 16; }
int field_player_my(void) { return (ppy + 8) / 16; }
int field_face(void) { return 0; }
int field_cam_x(void) { return cam_ov ? cam_x : 0; }
int field_cam_y(void) { return cam_ov ? cam_y : 0; }
void field_cam_override(int on, int cx, int cy) { cam_ov = on; cam_x = cx; cam_y = cy; }
void field_wait(int frames) { while (frames-- > 0) tick(1); }
void field_shake(int frames) { (void)frames; }
void field_walk_npc(int idx, int dir) { (void)idx; (void)dir; }
void field_face_npc(int idx, int dir) { (void)idx; (void)dir; }
void field_npc_patrol(int idx, int r) { (void)idx; (void)r; }
void field_set_meta(int mx, int my, int id) { (void)mx; (void)my; (void)id; }
int field_meta_at(int mx, int my) { (void)mx; (void)my; return 0; }

/* events.c is game CONTENT (stage 2 for the host build); encounter victory
 * calls this hook, so stand it in and count the calls. */
int sim_level_up_choices_calls;
void level_up_choices(void) { sim_level_up_choices_calls++; }

/* ------------------------------------------------------------ reset + guard */

void sim_reset(void) {
    memset(ewram, 0, sizeof ewram);
    memset(iwram, 0, sizeof iwram);
    memset(ioreg, 0, sizeof ioreg);
    memset(mgbareg, 0, sizeof mgbareg);
    memset(palram, 0, sizeof palram);
    memset(vram, 0, sizeof vram);
    memset(oamram, 0, sizeof oamram);
    memset(grid, ' ', sizeof grid);
    memset(snaps, ' ', sizeof snaps);
    nsnap = 0;
    memset(objs, 0, sizeof objs);
    memset(&clk, 0, sizeof clk);
    clk.budget = 2000000;
    g_frame = 0;
    qlen = qhead = 0;
    keygen = 0;
    nchoice = ichoice = 0;
    loglen = 0;
    logbuf[0] = 0;
    sim_violations = 0;
    sim_violation_msg[0] = 0;
    sim_fail_msg[0] = 0;
    sim_last_music = -1;
    memset(sfx_counts, 0, sizeof sfx_counts);
    win_draws = 0;
    dlg_open_p = 0;
    sim_level_up_choices_calls = 0;
    rngs = 0x1234567u;

    /* field */
    memset(npcs, 0, sizeof npcs);
    nnpc = 0;
    ppx = 120; ppy = 80;
    cam_ov = cam_x = cam_y = 0;
    player_hidden = 0;
    hero_objt = 0; hero_pal = 0;

    /* game state: zeroed party5[] makes party5.c's built[] merge a no-op,
     * so re-running party_init between tests is safe without src changes */
    memset(&G, 0, sizeof G);
    G.origin = ORIG_CUSTOM;
    memset(party5, 0, sizeof(R5Creature) * 3);
    memset(bench5, 0, sizeof(R5Creature) * RESERVE_MAX);
}

int sim_guard(void (*fn)(void)) {
    int rc = setjmp(sim_jmp);
    if (rc == 0) {
        sim_jmp_armed = 1;
        fn();
    }
    sim_jmp_armed = 0;
    return rc;
}
