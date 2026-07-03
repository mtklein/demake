#include "gba.h"

void audio_tick(void) {}   /* stub until the audio engine lands */

int main(void) {
    gba_init();

    /* mode 3 hello: prove the ROM boots and IRQs fire */
    REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
    vu16* fb = VRAM;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 240; x++)
            fb[y * 240 + x] = RGB15(x * 31 / 239, y * 31 / 159, 12);

    mgba_log("nautiloid: hello, boot ok");

    for (;;) {
        vsync();
        key_poll();
        /* animated marker square so vblank progress is visible */
        u32 t = (g_frame / 4) % 200;
        for (int y = 70; y < 90; y++)
            for (u32 x = t; x < t + 20; x++)
                fb[y * 240 + x] = RGB15(31, 31, 31);
        if (g_frame % 16 == 0)
            mgba_logf("frame %d keys %x", (int)g_frame, key_state());
    }
}
