/* PSG music tracker + SFX driver, ticked from the vblank IRQ. */
#include "gba.h"
#include "audio.h"

/* rate = 2048 - 131072/f, C2..B6 */
static const u16 rate_tab[60] = {
      44,  157,  263,  363,  457,  547,  631,  711,  786,  856,  923,  986,
    1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517,
    1547, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783,
    1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915,
    1923, 1930, 1936, 1943, 1949, 1954, 1959, 1964, 1969, 1974, 1978, 1982,
};

/* triangle-ish bass wave */
static const u32 wave_tri[4] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476 };
/* softer rounded wave for pads */
static const u32 wave_organ[4] = { 0x8ACDEFCA, 0x86432011, 0x8ACDEFCA, 0x86432011 };

static const Song* song;
static u16 row, row_timer;
static u8  sfx_sq2, sfx_noi;          /* frames left of sfx ownership   */
static const u8* sfx_p;               /* sq2 sfx stream                 */
static u8  sfx_step_left;

static void sq2_note(u16 env, u16 rate) { REG_SOUND2CNT_L = env; REG_SOUND2CNT_H = 0x8000 | rate; }
static void sq2_cut(void) { REG_SOUND2CNT_L = 0x0800; REG_SOUND2CNT_H = 0x8000; }

void audio_init(void) {
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0xFF77;
    REG_SOUNDCNT_H = 0x0002;
    /* prime wave channel: load wave, trigger silent */
    REG_SOUND3CNT_L = 0x0040;
    for (int i = 0; i < 4; i++) ((vu32*)WAVE_RAM)[i] = wave_tri[i];
    REG_SOUND3CNT_L = 0x0080;
    REG_SOUND3CNT_H = 0x0000;   /* mute */
    REG_SOUND3CNT_X = 0x8000 | rate_tab[24];
}

void music_play(const Song* s) {
    song = s; row = 0; row_timer = 1;
}

void music(int id) {
    extern const Song songs[];
    if (id < 0) music_stop();
    else music_play(&songs[id]);
}
void music_stop(void) {
    song = 0;
    REG_SOUND1CNT_H = 0x0800; REG_SOUND1CNT_X = 0x8000;
    sq2_cut();
    REG_SOUND3CNT_H = 0;
    REG_SOUND4CNT_L = 0x0800; REG_SOUND4CNT_H = 0x8000;
}

/* --- noise drum presets: kick, snare, hat, open hat --- */
static const u16 drum_l[4] = { 0xF100, 0xE200, 0xB100, 0xB300 };
static const u16 drum_h[4] = { 0x8063, 0x8033, 0x8019, 0x8019 };

static void play_row(void) {
    const u8 n1 = song->ch[0][row], n2 = song->ch[1][row],
             n3 = song->ch[2][row], n4 = song->ch[3][row];
    if (n1 <= NOTE_MAX) {
        REG_SOUND1CNT_L = 0x0008;
        REG_SOUND1CNT_H = song->env1;
        REG_SOUND1CNT_X = 0x8000 | rate_tab[n1];
    } else if (n1 == EV_CUT) {
        REG_SOUND1CNT_H = 0x0800; REG_SOUND1CNT_X = 0x8000;
    }
    if (!sfx_sq2) {
        if (n2 <= NOTE_MAX) sq2_note(song->env2, rate_tab[n2]);
        else if (n2 == EV_CUT) sq2_cut();
    }
    if (n3 <= NOTE_MAX) {
        int idx = n3 + 12;                    /* wave sounds one octave low */
        REG_SOUND3CNT_H = song->wavevol;
        REG_SOUND3CNT_X = 0x8000 | rate_tab[idx > 59 ? 59 : idx];
    } else if (n3 == EV_CUT) {
        REG_SOUND3CNT_H = 0;
    }
    if (!sfx_noi) {
        if (n4 <= 3) { REG_SOUND4CNT_L = drum_l[n4]; REG_SOUND4CNT_H = drum_h[n4]; }
        else if (n4 == EV_CUT) { REG_SOUND4CNT_L = 0x0800; REG_SOUND4CNT_H = 0x8000; }
    }
}

/* --- SFX: byte stream on square 2: [dur, note]*, 0xFF end. --- */
static const u8 sfx_data[][8] = {
    [SFX_CURSOR]  = { 2, 48, 0xFF },
    [SFX_CONFIRM] = { 3, 43, 3, 50, 0xFF },
    [SFX_CANCEL]  = { 3, 38, 3, 31, 0xFF },
    [SFX_HIT]     = { 2, 18, 2, 13, 2, 8, 0xFF },
    [SFX_HEAL]    = { 3, 36, 3, 40, 3, 43, 0xFF },
    [SFX_STEP]    = { 1, 20, 0xFF },
};

void sfx_play(int id) {
    sfx_p = sfx_data[id];
    sfx_step_left = 0;
    sfx_sq2 = 60;              /* own the channel for up to a second */
}

void sfx_noise(int frames) {   /* impact burst */
    REG_SOUND4CNT_L = 0xF200;
    REG_SOUND4CNT_H = 0x8034;
    sfx_noi = (u8)frames;
}

static void sfx_tick(void) {
    if (sfx_noi) sfx_noi--;
    if (!sfx_p) { if (sfx_sq2) sfx_sq2--; return; }
    if (sfx_step_left == 0) {
        if (*sfx_p == 0xFF) { sfx_p = 0; sq2_cut(); sfx_sq2 = 0; return; }
        sfx_step_left = *sfx_p++;
        u8 note = *sfx_p++;
        sq2_note(0xB080, rate_tab[note]);   /* vol 11, decay, 50% duty */
    }
    sfx_step_left--;
}

void audio_tick(void) {
    sfx_tick();
    if (!song) return;
    if (--row_timer == 0) {
        row_timer = song->speed;
        play_row();
        if (++row >= song->rows) {
            if (song->loop == 0xFFFF) music_stop();  /* key off: no held tail */
            else row = song->loop;
        }
    }
}

const u32* audio_wave_tri = wave_tri;
const u32* audio_wave_organ = wave_organ;
