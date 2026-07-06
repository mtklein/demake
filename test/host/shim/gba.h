/* Host shim for src/gba.h: same macro API, but every absolute GBA address
 * resolves into fake host RAM (fake_engine.c) so real game code runs natively
 * under ASan/UBSan.
 *
 * How the shadowing works: this header claims the real header's include
 * guard (GBA_H). It is force-included first (-include shim/gba.h), so when
 * src/menu.c later does #include "gba.h" -- which quoted-include rules
 * resolve to src/gba.h regardless of -I order -- the real header no-ops.
 *
 * One deliberate typedef change, documented:
 *   u32 is uintptr_t here (64-bit on the host). On the GBA u32 IS the
 *   machine's pointer-sized word, and real game code round-trips pointers
 *   through it ((u32)CHARBLOCK(0) + off in menu.c, (u32)OBJ_TILES + off in
 *   encounter.c). With uint32_t those casts would truncate 64-bit host
 *   pointers to garbage; with uintptr_t they behave exactly as on device.
 *   No host-side game logic depends on u32 wrapping at 2^32 (the rules/
 *   dice RNG uses uint32_t explicitly and is bit-identical).
 */
#ifndef GBA_H
#define GBA_H

#include <stdint.h>

typedef uint8_t  u8;  typedef uint16_t u16; typedef uintptr_t u32;
typedef int8_t   s8;  typedef int16_t  s16; typedef int32_t  s32;
typedef volatile u8 vu8; typedef volatile u16 vu16; typedef volatile u32 vu32;

/* fake_engine.c: map a GBA bus address (0x02/03/04/05/06/07 banks) to host
 * RAM. Out-of-range addresses fail the running test loudly. */
void* host_map(u32 addr, u32 len);

#define REG16(a) (*(vu16*)host_map((a), 2))
#define REG32(a) (*(vu32*)host_map((a), (u32)sizeof(vu32)))

/* --- display --- */
#define REG_DISPCNT   REG16(0x4000000)
#define REG_DISPSTAT  REG16(0x4000004)
#define REG_VCOUNT    REG16(0x4000006)
#define REG_BG0CNT    REG16(0x4000008)
#define REG_BG1CNT    REG16(0x400000A)
#define REG_BG2CNT    REG16(0x400000C)
#define REG_BG3CNT    REG16(0x400000E)
#define REG_BG0HOFS   REG16(0x4000010)
#define REG_BG0VOFS   REG16(0x4000012)
#define REG_BG1HOFS   REG16(0x4000014)
#define REG_BG1VOFS   REG16(0x4000016)
#define REG_BG2HOFS   REG16(0x4000018)
#define REG_BG2VOFS   REG16(0x400001A)
#define REG_BG3HOFS   REG16(0x400001C)
#define REG_BG3VOFS   REG16(0x400001E)
#define REG_MOSAIC    REG16(0x400004C)
#define REG_BLDCNT    REG16(0x4000050)
#define REG_BLDALPHA  REG16(0x4000052)
#define REG_BLDY      REG16(0x4000054)

#define DCNT_MODE0    0x0000
#define DCNT_MODE3    0x0003
#define DCNT_BLANK    0x0080
#define DCNT_BG0      0x0100
#define DCNT_BG1      0x0200
#define DCNT_BG2      0x0400
#define DCNT_BG3      0x0800
#define DCNT_OBJ      0x1000
#define DCNT_OBJ_1D   0x0040

#define BGCNT_PRIO(n)   (n)
#define BGCNT_CB(n)     ((n)<<2)   /* char base, 16K units   */
#define BGCNT_MOSAIC    0x0040
#define BGCNT_8BPP      0x0080
#define BGCNT_SB(n)     ((n)<<8)   /* screen base, 2K units  */
#define BGCNT_SIZE(n)   ((n)<<14)  /* 0:32x32 1:64x32 2:32x64 3:64x64 tiles */

/* --- sound --- */
#define REG_SOUND1CNT_L REG16(0x4000060)
#define REG_SOUND1CNT_H REG16(0x4000062)
#define REG_SOUND1CNT_X REG16(0x4000064)
#define REG_SOUND2CNT_L REG16(0x4000068)
#define REG_SOUND2CNT_H REG16(0x400006C)
#define REG_SOUND3CNT_L REG16(0x4000070)
#define REG_SOUND3CNT_H REG16(0x4000072)
#define REG_SOUND3CNT_X REG16(0x4000074)
#define REG_SOUND4CNT_L REG16(0x4000078)
#define REG_SOUND4CNT_H REG16(0x400007C)
#define REG_SOUNDCNT_L  REG16(0x4000080)
#define REG_SOUNDCNT_H  REG16(0x4000082)
#define REG_SOUNDCNT_X  REG16(0x4000084)
#define REG_SOUNDBIAS   REG16(0x4000088)
#define WAVE_RAM        ((vu16*)host_map(0x4000090, 32))

/* --- DMA --- */
#define REG_DMA3SAD   REG32(0x40000D4)
#define REG_DMA3DAD   REG32(0x40000D8)
#define REG_DMA3CNT_L REG16(0x40000DC)
#define REG_DMA3CNT_H REG16(0x40000DE)
#define DMA_ENABLE    0x8000
#define DMA_32        0x0400

/* --- timers --- */
#define REG_TM0CNT_L  REG16(0x4000100)
#define REG_TM0CNT_H  REG16(0x4000102)

/* --- input --- */
#define REG_KEYINPUT  REG16(0x4000130)
#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200
#define KEY_ANY    0x03FF

/* --- interrupts / system --- */
#define REG_IE      REG16(0x4000200)
#define REG_IF      REG16(0x4000202)
#define REG_WAITCNT REG16(0x4000204)
#define REG_IME     REG16(0x4000208)
#define IRQ_VBLANK  0x0001
#define DSTAT_VBL_IRQ 0x0008

/* --- memory --- */
#define PAL_BG   ((vu16*)host_map(0x5000000, 0x200))
#define PAL_OBJ  ((vu16*)host_map(0x5000200, 0x200))
#define VRAM     ((vu16*)host_map(0x6000000, 2))
#define OBJ_TILES ((vu16*)host_map(0x6010000, 2))
#define OAM_MEM  ((vu16*)host_map(0x7000000, 0x400))
#define CHARBLOCK(n)   ((vu16*)host_map(0x6000000 + (u32)(n)*0x4000, 2))
#define SCREENBLOCK(n) ((vu16*)host_map(0x6000000 + (u32)(n)*0x800, 2))

#define RGB15(r,g,b) ((u16)((r) | ((g)<<5) | ((b)<<10)))

#define IWRAM_CODE
#define EWRAM_BSS

/* demo/attract flag block, same addresses as on device (poked by tests) */
#define G_DEMO (*(vu8*)host_map(0x0203FF00, 1))
#define G_DEMO_BATTLE (*(vu8*)host_map(0x0203FF02, 1))
#define G_DEMO_CLASS  (*(vu8*)host_map(0x0203FF03, 1))
#define G_FIELD_IDLE  (*(vu8*)host_map(0x0203FF05, 1))
#define G_DONE        (*(vu8*)host_map(0x0203FF06, 1))
#define G_MANUAL_BAT  (*(vu8*)host_map(0x0203FF07, 1))
#define G_DEMO_RACE   (*(vu8*)host_map(0x0203FF08, 1))
#define G_DEMO_BG     (*(vu8*)host_map(0x0203FF09, 1))
#define G_DEMO_AB     (*(vu8*)host_map(0x0203FF0A, 1))
#define G_AB_BUF ((vu8*)host_map(0x0203FF20, 6))
#define G_FLAGS_MAGIC (*(vu32*)host_map(0x0203FF38, (u32)sizeof(vu32)))
#define G_FLAGS_COOKIE 0xC0FFEE01u
#define G_CHOICE_BUF ((vu8*)host_map(0x0203FF10, 16))

/* --- mGBA debug logging --- */
#define MGBA_LOG_ENABLE (*(vu16*)host_map(0x4FFF780, 2))
#define MGBA_LOG_FLAGS  (*(vu16*)host_map(0x4FFF700, 2))
#define MGBA_LOG_STR    ((volatile char*)host_map(0x4FFF600, 256))

/* util.c */
extern volatile u32 g_frame;

/* --- crash reporting (src/panic.c) --- */
void panic(const char* why, u32 pc);
void crumb(int code, int arg);
extern volatile u32 g_irq_pc, g_wd;
enum { CR_ROOM = 1, CR_ENC, CR_TURN, CR_MENU, CR_CAST, CR_RESULT };
#define ASSERT(c) do { if (!(c)) panic("assert " __FILE__, __LINE__); } while (0)
#define PANIC_WD_FRAMES 600            /* ~10s without frame() = hang */
#define G_AUDIT       (*(vu8*)host_map(0x0203FF0B, 1))
#define G_PANIC_TEST  (*(vu8*)host_map(0x0203FF0C, 1))
#define G_DEMO_LEVEL  (*(vu8*)host_map(0x0203FF0D, 1))
#define G_DEMO_ORIGIN (*(vu8*)host_map(0x0203FF0E, 1))
#define G_SKILL_TEST  (*(vu8*)host_map(0x0203FF0F, 1))
void gba_init(void);
void vsync(void);
u16  key_state(void);      /* held keys, 1 = pressed */
u16  key_hit(void);        /* newly pressed this frame */
void key_poll(void);
void memcpy16(vu16* dst, const u16* src, u32 halfwords);
void memset16(vu16* dst, u16 val, u32 halfwords);
void mgba_log(const char* s);
void mgba_logf(const char* fmt, ...);   /* tiny: %d %x %s only */
u32  rnd(void);
int  rnd_range(int n);

#endif
