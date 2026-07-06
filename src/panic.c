/* Crash reporting: a panic screen + FATAL mGBA log, breadcrumbs, and a
 * VBlank watchdog that catches main-loop hangs.
 *
 * The GBA BIOS owns the CPU exception vectors, so there is no trap to hook;
 * this is the embedded pattern instead:
 *   - ASSERT()/panic() from code paths that detect impossible states;
 *   - the VBlank IRQ keeps firing while the main loop is hung, so after
 *     PANIC_WD_FRAMES without a frame() the handler panics WITH the
 *     interrupted PC (crt0 stores lr on every IRQ entry) -- feed that
 *     address to llvm-addr2line against build/nautiloid.elf;
 *   - a 16-entry breadcrumb ring (room/encounter/turn/menu/cast events)
 *     tells the story leading in. Codes: see CR_* in gba.h.
 * Everything shown on screen is also emitted through mgba_logf, first line
 * at FATAL level, so the headless runner and mGBA's log viewer capture the
 * whole report; on hardware, the screen is the report (photograph it).
 * The screen loops forever -- a crashed cart reboots by power cycle. */
#include "gba.h"
#include "engine.h"
#include "game.h"
#include "buildid.h"

volatile u32 g_irq_pc;               /* interrupted PC+4, stored by crt0 */
volatile u32 g_wd;                   /* frames since last frame(); IRQ-bumped */

static volatile u16 crumbs[32];      /* (code,arg) pairs, ring of 16 */
static volatile u8 crumb_i;
static u8 in_panic;

void crumb(int code, int arg) {
    int i = crumb_i;
    crumbs[i * 2] = (u16)code;
    crumbs[i * 2 + 1] = (u16)arg;
    crumb_i = (u8)((i + 1) & 15);
}

static char* put_hex(char* p, u32 v) {
    for (int s = 28; s >= 0; s -= 4) {
        u32 d = (v >> s) & 15;
        *p++ = (char)(d < 10 ? '0' + d : 'a' + d - 10);
    }
    return p;
}

void panic(const char* why, u32 pc) {
    if (in_panic) for (;;) {}        /* a panic inside the panic: freeze */
    in_panic = 1;

    /* first line at FATAL so mGBA surfaces it loudly */
    MGBA_LOG_ENABLE = 0xC0DE;
    {
        volatile char* p = MGBA_LOG_STR;
        const char* s = "PANIC ";
        int i = 0;
        while (*s) p[i++] = *s++;
        s = why;
        while (*s && i < 200) p[i++] = *s++;
        p[i] = 0;
        MGBA_LOG_FLAGS = 0x100;      /* level 0 = fatal */
    }
    mgba_logf("build %s pc=%x frame=%d", BUILD_ID, pc, (int)g_frame);
    mgba_logf("flags=%x party=%d hp=%d/%d", G.flags, G.nparty,
              G.nparty ? G.pm[0].hp : 0, G.nparty ? G.pm[0].hpmax : 0);
    for (int i = 0; i < 16; i += 4)
        mgba_logf("crumb %x:%x %x:%x %x:%x %x:%x",
                  crumbs[i*2], crumbs[i*2+1], crumbs[i*2+2], crumbs[i*2+3],
                  crumbs[i*2+4], crumbs[i*2+5], crumbs[i*2+6], crumbs[i*2+7]);

    /* draw the report with the text layer only; touch nothing else */
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
    REG_BLDCNT = 0; REG_BLDY = 0;
    txt_clear(0, 0, 30, 20);
    txt_put(1, 1, "!! IT ALL WENT WRONG !!", 1);
    txt_put(1, 3, why, 0);
    {
        char b[30]; char* p = b;
        const char* s = "pc ";
        while (*s) *p++ = *s++;
        p = put_hex(p, pc); *p = 0;
        txt_put(1, 5, b, 2);
        p = b; s = "build ";
        while (*s) *p++ = *s++;
        s = BUILD_ID;
        while (*s && p < b + 28) *p++ = *s++;
        *p = 0;
        txt_put(1, 6, b, 2);
    }
    txt_put(1, 8, "recent steps:", 2);
    for (int r = 0; r < 8; r++) {
        char b[30]; char* p = b;
        int i = (crumb_i + 16 - 1 - r) & 15;    /* newest first */
        p = put_hex(p, ((u32)crumbs[i*2] << 16) | crumbs[i*2+1]);
        *p = 0;
        txt_put(3 + (r & 1) * 14, 9 + r / 2, b, 0);
    }
    txt_put(1, 15, "please photograph this", 1);
    txt_put(1, 16, "screen and report it!", 1);
    for (;;) {}                       /* IRQ-safe: no vsync, no return */
}
