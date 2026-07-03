/* Field mode: tilemap rooms, grid walking, camera, NPCs, triggers. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"

static const u8* fmeta;
static int fw, fh;
static int cam_x, cam_y;
static int ppx, ppy;          /* player pixel pos (top-left of 16x16) */
static int pface;             /* 0 down 1 up 2 left 3 right */
static int pmoving, pstep;
static int exit_req;
static u32 ticks;

Npc npcs[FMAX_NPC];
static int nnpc;

static void map_put(int tx, int ty, u16 e) {
    vu16* sb = SCREENBLOCK(24 + (tx >> 5) + ((ty >> 5) << 1));
    sb[(ty & 31) * 32 + (tx & 31)] = e;
}

void field_set_meta(int mx, int my, int id) {
    /* swap a metatile in VRAM (doors opening, consoles lighting up) */
    const u16* e = &metatile_lut[id * 4];
    map_put(mx * 2, my * 2, e[0]);
    map_put(mx * 2 + 1, my * 2, e[1]);
    map_put(mx * 2, my * 2 + 1, e[2]);
    map_put(mx * 2 + 1, my * 2 + 1, e[3]);
}

int field_meta_at(int mx, int my) {
    if (mx < 0 || my < 0 || mx >= fw || my >= fh) return -1;
    return fmeta[my * fw + mx];
}

void field_load(const u8* meta, int w, int h) {
    fmeta = meta; fw = w; fh = h;
    nnpc = 0; exit_req = 0; pmoving = 0;

    memcpy16(CHARBLOCK(1), field_tiles_gfx, FIELD_TILE_COUNT * 16);
    memcpy16(CHARBLOCK(2), sky_tiles_gfx, SKY_TILE_COUNT * 16);

    /* sky map: 4 bands, 2 variants chosen per column */
    for (int y = 0; y < 32; y++) {
        int band = y / 8;
        for (int x = 0; x < 32; x++) {
            int v = ((x * 7 + y * 3) >> 2) & 1;
            ((vu16*)SCREENBLOCK(28))[y * 32 + x] =
                (u16)((v * 4 + band) | (4 << 12));
        }
    }

    /* fill 64x64 with void, then blit the room */
    const u16* voident = &metatile_lut[MT_VOID * 4];
    for (int ty = 0; ty < 64; ty++)
        for (int tx = 0; tx < 64; tx++)
            map_put(tx, ty, voident[((ty & 1) << 1) | (tx & 1)]);
    for (int my = 0; my < h; my++)
        for (int mx = 0; mx < w; mx++) {
            const u16* e = &metatile_lut[meta[my * w + mx] * 4];
            map_put(mx * 2, my * 2, e[0]);
            map_put(mx * 2 + 1, my * 2, e[1]);
            map_put(mx * 2, my * 2 + 1, e[2]);
            map_put(mx * 2 + 1, my * 2 + 1, e[3]);
        }

    REG_BG2CNT = BGCNT_CB(1) | BGCNT_SB(24) | BGCNT_SIZE(3) | BGCNT_PRIO(2);
    REG_BG3CNT = BGCNT_CB(2) | BGCNT_SB(28) | BGCNT_SIZE(0) | BGCNT_PRIO(3);
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_BG3 |
                  DCNT_OBJ | DCNT_OBJ_1D;
}

void field_spawn(int mx, int my, int face) {
    ppx = mx * 16; ppy = my * 16; pface = face; pmoving = 0;
}

int field_add_npc(int mx, int my, int objt, int pal, int face, int flags) {
    Npc* n = &npcs[nnpc];
    n->x = (s16)(mx * 16); n->y = (s16)(my * 16);
    n->objt = (u16)objt; n->pal = (u8)pal; n->face = (u8)face;
    n->flags = (u8)flags; n->id = (u8)nnpc;
    return nnpc++;
}

void field_remove_npc(int idx) { npcs[idx].flags |= NPC_GONE; }
void field_exit(void) { exit_req = 1; }
int  field_player_mx(void) { return (ppx + 8) / 16; }
int  field_player_my(void) { return (ppy + 8) / 16; }
int  field_player_x(void) { return ppx; }
int  field_player_y(void) { return ppy; }
int  field_face(void) { return pface; }
int  field_cam_x(void) { return cam_x; }
int  field_cam_y(void) { return cam_y; }

static int player_hidden;
void field_hide_player(int on) {
    player_hidden = on;
    if (on) obj_hide(OBJ_PLAYER);
}

static int npc_at(int mx, int my) {
    for (int i = 0; i < nnpc; i++) {
        if (npcs[i].flags & NPC_GONE) continue;
        if (npcs[i].x == mx * 16 && npcs[i].y == my * 16) return i;
    }
    return -1;
}

static int walkable(int mx, int my) {
    int m = field_meta_at(mx, my);
    if (m < 0 || metatile_solid[m]) return 0;
    if (npc_at(mx, my) >= 0) return 0;
    return 1;
}

static void draw_walker(int obj, int px, int py, int objt, int pal,
                        int face, int animf, int nframes) {
    int sx = px - cam_x, sy = py - cam_y - 2;   /* feet slightly into tile */
    if (sx < -16 || sx > 240 || sy < -16 || sy > 160) { obj_hide(obj); return; }
    int framei, hf = 0;
    if (nframes == 2) {                 /* two-frame critters: bob */
        framei = animf;
    } else {
        switch (face) {
            default: framei = animf; break;               /* down */
            case 1:  framei = 2 + animf; break;           /* up   */
            case 2:  framei = 4 + animf; break;           /* left */
            case 3:  framei = 4 + animf; hf = 1; break;   /* right */
        }
    }
    obj_set(obj, sx, sy, 1, objt + framei * 4, pal, 2);
    obj_flip(obj, hf, 0);
}

static int shake_t;
void field_shake(int frames) { shake_t = frames; }

static void update_cam(void) {
    cam_x = ppx + 8 - 120; cam_y = ppy + 8 - 80;
    int maxx = fw * 16 - 240, maxy = fh * 16 - 160;
    if (cam_x > maxx) cam_x = maxx;
    if (cam_y > maxy) cam_y = maxy;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (shake_t > 0) {
        shake_t--;
        cam_x += (int)(rnd() & 3) - 1;
        cam_y += (int)(rnd() & 3) - 2;
    }
    REG_BG2HOFS = (u16)cam_x;
    REG_BG2VOFS = (u16)cam_y;
    REG_BG3HOFS = (u16)((cam_x >> 2) + (int)(ticks >> 4));
    REG_BG3VOFS = (u16)(cam_y >> 3);
}

void field_draw(void) {
    ticks++;
    update_cam();
    int panim = pmoving ? ((pstep >> 2) & 1) : 0;
    if (!player_hidden)
        draw_walker(OBJ_PLAYER, ppx, ppy, OBJT_HERO, 0, pface, panim, 6);
    for (int i = 0; i < nnpc; i++) {
        Npc* n = &npcs[i];
        if (n->flags & NPC_GONE) { obj_hide(OBJ_NPC0 + i); continue; }
        int nf = (n->flags & NPC_2FRAME) ? 2 : 6;
        int anim = (n->flags & NPC_2FRAME) ? ((ticks >> 4) & 1) : 0;
        draw_walker(OBJ_NPC0 + i, n->x, n->y, n->objt, n->pal, n->face, anim, nf);
    }
}

/* walk one tile in dir as a cutscene action (player or npc) */
void field_walk_npc(int idx, int dir) {
    static const s8 dx[4] = {0, 0, -1, 1}, dy[4] = {1, -1, 0, 0};
    Npc* n = &npcs[idx];
    n->face = (u8)dir;
    for (int i = 0; i < 8; i++) {
        n->x = (s16)(n->x + dx[dir] * 2);
        n->y = (s16)(n->y + dy[dir] * 2);
        frame();
        field_draw();
    }
}

void field_face_npc(int idx, int dir) { npcs[idx].face = (u8)dir; }

void field_wait(int frames) {
    while (frames--) { frame(); field_draw(); }
}

void field_run(void) {
    static const s8 dx[4] = {0, 0, -1, 1}, dy[4] = {1, -1, 0, 0};
    int tdx = 0, tdy = 0;
    while (!exit_req) {
        frame();
        G_FIELD_IDLE = 1;
        if (!pmoving) {
            u16 held = key_state();
            int dir = -1;
            if (held & KEY_DOWN) dir = 0;
            else if (held & KEY_UP) dir = 1;
            else if (held & KEY_LEFT) dir = 2;
            else if (held & KEY_RIGHT) dir = 3;

            if (dir >= 0) {
                pface = dir;
                int nx = ppx / 16 + dx[dir], ny = ppy / 16 + dy[dir];
                if (walkable(nx, ny)) {
                    pmoving = 1; pstep = 0; tdx = dx[dir]; tdy = dy[dir];
                }
            } else if (key_hit() & KEY_A) {
                int fx = ppx / 16 + dx[pface], fy = ppy / 16 + dy[pface];
                int ni = npc_at(fx, fy);
                if (ni >= 0) ev_npc(ni);
                else ev_interact(fx, fy);
            }
        } else {
            ppx += tdx * 2; ppy += tdy * 2;
            if (++pstep >= 8) {
                pmoving = 0;
                ev_step(ppx / 16, ppy / 16);
            }
        }
        field_draw();
    }
    exit_req = 0;
}
