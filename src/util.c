#include "gba.h"
#include <stdarg.h>

volatile u32 g_frame;
static u16 s_keys, s_prev;

/* called from the IRQ dispatcher every vblank (thumb) */
void vblank_tick(void);
void audio_tick(void);
void vblank_tick(void) { audio_tick(); }

void gba_init(void) {
    REG_WAITCNT = 0x4317;            /* 3/1 ROM waitstates + prefetch */
    REG_DISPSTAT = DSTAT_VBL_IRQ;
    REG_IE = IRQ_VBLANK;
    REG_IF = 0xFFFF;
    REG_IME = 1;
}

void vsync(void) {
    u32 f = g_frame;
    while (g_frame == f)
        __asm volatile("swi 0x02" ::: "r0", "r1", "r2", "r3", "r12", "memory");
}

void key_poll(void) {
    s_prev = s_keys;
    s_keys = (u16)(~REG_KEYINPUT) & KEY_ANY;
}
u16 key_state(void) { return s_keys; }
u16 key_hit(void)   { return s_keys & (u16)~s_prev; }

void memcpy16(vu16* dst, const u16* src, u32 n) {
    while (n--) *dst++ = *src++;
}
void memset16(vu16* dst, u16 val, u32 n) {
    while (n--) *dst++ = val;
}

/* gcc may emit calls to these even freestanding */
void* memcpy(void* d, const void* s, unsigned n);
void* memcpy(void* d, const void* s, unsigned n) {
    u8* dd = d; const u8* ss = s;
    while (n--) *dd++ = *ss++;
    return d;
}
void* memset(void* d, int c, unsigned n);
void* memset(void* d, int c, unsigned n) {
    u8* dd = d;
    while (n--) *dd++ = (u8)c;
    return d;
}

/* xorshift RNG */
static u32 rngs = 2463534242u;
u32 rnd(void) {
    rngs ^= rngs << 13; rngs ^= rngs >> 17; rngs ^= rngs << 5;
    return rngs;
}
int rnd_range(int n) { return n > 0 ? (int)(rnd() % (u32)n) : 0; }

/* --- mGBA debug logging --- */
static void log_flush(int level) { MGBA_LOG_FLAGS = (u16)(level | 0x100); }

void mgba_log(const char* s) {
    MGBA_LOG_ENABLE = 0xC0DE;
    volatile char* p = MGBA_LOG_STR;
    int i = 0;
    while (s[i] && i < 255) { p[i] = s[i]; i++; }
    if (i < 256) p[i] = 0;
    log_flush(2); /* info */
}

static char* fmt_u32(char* p, u32 v, int base) {
    char tmp[12]; int n = 0;
    do { u32 d = v % (u32)base; tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); v /= (u32)base; } while (v);
    while (n) *p++ = tmp[--n];
    return p;
}

void mgba_logf(const char* fmt, ...) {
    char buf[128]; char* p = buf;
    va_list ap; va_start(ap, fmt);
    for (const char* f = fmt; *f && p < buf + 120; f++) {
        if (*f != '%') { *p++ = *f; continue; }
        f++;
        if (*f == 'd') { s32 v = va_arg(ap, s32); if (v < 0) { *p++ = '-'; v = -v; } p = fmt_u32(p, (u32)v, 10); }
        else if (*f == 'x') { p = fmt_u32(p, va_arg(ap, u32), 16); }
        else if (*f == 's') { const char* s = va_arg(ap, const char*); while (*s && p < buf + 120) *p++ = *s++; }
        else *p++ = *f;
    }
    va_end(ap);
    *p = 0;
    mgba_log(buf);
}
