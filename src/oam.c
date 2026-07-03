#include "gba.h"

typedef struct { u16 a0, a1, a2, pad; } ObjAttr;
static ObjAttr shadow[128];

#define ATTR0_HIDE 0x0200

void oam_init(void) {
    for (int i = 0; i < 128; i++) shadow[i].a0 = ATTR0_HIDE;
}

/* call right after vsync() */
void oam_flush(void) {
    memcpy16(OAM_MEM, (const u16*)shadow, 128 * 4);
}

/* shape/size: 0=8x8 1=16x16 2=32x32 3=64x64 (square); 4=16x8 5=32x8? use table below */
void obj_set(int i, int x, int y, int shapesize, int tile, int pal, int prio) {
    /* attr0 shape (bits 14-15): 0 square, 1 wide, 2 tall; attr1 size (bits 14-15) */
    static const u16 shp[][2] = {
        {0 << 14, 0 << 14},   /* 0: 8x8   */
        {0 << 14, 1 << 14},   /* 1: 16x16 */
        {0 << 14, 2 << 14},   /* 2: 32x32 */
        {0 << 14, 3 << 14},   /* 3: 64x64 */
        {1 << 14, 0 << 14},   /* 4: 16x8  */
        {1 << 14, 2 << 14},   /* 5: 32x16 */
        {2 << 14, 0 << 14},   /* 6: 8x16  */
        {2 << 14, 2 << 14},   /* 7: 16x32 */
        {2 << 14, 3 << 14},   /* 8: 32x64 */
    };
    shadow[i].a0 = (u16)((y & 0xFF) | shp[shapesize][0]);
    shadow[i].a1 = (u16)((x & 0x1FF) | shp[shapesize][1]);
    shadow[i].a2 = (u16)(tile | (prio << 10) | (pal << 12));
}

void obj_hide(int i) { shadow[i].a0 = ATTR0_HIDE; }

void obj_flip(int i, int h, int v) {
    shadow[i].a1 = (u16)((shadow[i].a1 & ~0x3000) | (h ? 0x1000 : 0) | (v ? 0x2000 : 0));
}
