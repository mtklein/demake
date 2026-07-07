/* Field mode: tilemap rooms, grid walking, camera, NPCs, triggers. */
#include "gba.h"
#include "assets.h"
#include "engine.h"
#include "field.h"
#include "game.h"
#include "rules.h"

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

static void cone_hide(void);     /* vision-cone garnish; block sits by npc_ai */
static u8 cone_seen;             /* one-shot "cone shown" log, per room */

void field_load(const u8* meta, int w, int h) {
    fmeta = meta; fw = w; fh = h;
    nnpc = 0; exit_req = 0; pmoving = 0;
    cone_seen = 0; cone_hide();      /* fresh room: fresh one-shot, no dots */

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
    /* reuse GONE slots so respawn-heavy flows (encounter retries) don't leak */
    int i;
    for (i = 0; i < nnpc; i++)
        if (npcs[i].flags & NPC_GONE) break;
    if (i == nnpc) {
        if (nnpc >= FMAX_NPC) { mgba_log("npc overflow!"); return -1; }
        nnpc++;
    }
    Npc* n = &npcs[i];
    n->x = (s16)(mx * 16); n->y = (s16)(my * 16);
    n->objt = (u16)objt; n->pal = (u8)pal; n->face = (u8)face;
    n->flags = (u8)flags; n->id = (u8)i;
    return i;
}

void field_npc_patrol(int idx, int radius_px) {
    npcs[idx].flags |= NPC_PATROL;
    npcs[idx].hx = npcs[idx].x;
    npcs[idx].hy = npcs[idx].y;
    npcs[idx].aggro_r = (u8)radius_px;
    npcs[idx].chasing = 0;
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
    else pface = 0;   /* resuming field control (post-battle): face DOWN, so
                       * facing is deterministic for the turn-in-place walk */
}

static int npc_at(int mx, int my) {
    for (int i = 0; i < nnpc; i++) {
        if (npcs[i].flags & NPC_GONE) continue;
        if ((npcs[i].x + 8) >> 4 == mx && (npcs[i].y + 8) >> 4 == my)
            return i;   /* center-tile match: patrollers drift off-grid */
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

static u16 hero_objt = OBJT_HERO;
static u8 hero_pal;
void field_set_hero(int objt, int pal) { hero_objt = (u16)objt; hero_pal = (u8)pal; }

static int shake_t;
void field_shake(int frames) { shake_t = frames; }

/* encounters steer the camera to frame the whole fight */
static int cam_ov, cam_ovx, cam_ovy;
void field_cam_override(int on, int cx, int cy) {
    cam_ov = on; cam_ovx = cx; cam_ovy = cy;
}

static void update_cam(void) {
    if (cam_ov) {
        /* encounters may look past the room edge (void border) to keep
         * every combatant clear of the text bars */
        cam_x = cam_ovx; cam_y = cam_ovy;
    } else {
        cam_x = ppx + 8 - 120; cam_y = ppy + 8 - 80;
        int maxx = fw * 16 - 240, maxy = fh * 16 - 160;
        if (cam_x > maxx) cam_x = maxx;
        if (cam_y > maxy) cam_y = maxy;
        if (cam_x < 0) cam_x = 0;
        if (cam_y < 0) cam_y = 0;
    }
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
        draw_walker(OBJ_PLAYER, ppx, ppy, hero_objt, hero_pal, pface, panim, 6);
    for (int i = 0; i < nnpc; i++) {
        Npc* n = &npcs[i];
        if (n->flags & NPC_GONE) { obj_hide(OBJ_NPC0 + i); continue; }
        int nf = (n->flags & NPC_2FRAME) ? 2 : 6;
        int anim = (n->flags & NPC_2FRAME) ? ((ticks >> 4) & 1) : 0;
        draw_walker(OBJ_NPC0 + i, n->x, n->y, n->objt, n->pal, n->face, anim, nf);
    }
    for (int i = nnpc; i < FMAX_NPC; i++)     /* stale OAM from busier rooms */
        obj_hide(OBJ_NPC0 + i);
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

static int alert_npc = -1, alert_t;

/* The party-adjusted detection radius patroller n applies to a point at
 * (ddx,ddy) relative to it. npc_ai's trip test and the vision-cone garnish
 * BOTH call this -- the drawn boundary can never drift from the real one. */
static int sense_r(const Npc* n, int ddx, int ddy) {
    int r = n->aggro_r;
    if (G.pm[0].skills & (1u << SK_STEALTH)) r = r * 2 / 3;   /* trained: quieter */
    if (G.pm[0].expert & (1u << SK_STEALTH)) r = r * 2 / 3;   /* Expertise: quieter still */
    /* sentries watch a cone ahead; flanks and rear are peripheral */
    if (!((n->face == 0 && ddy > 0) || (n->face == 1 && ddy < 0) ||
          (n->face == 2 && ddx < 0) || (n->face == 3 && ddx > 0))) r /= 2;
    return r;
}

/* ---- patrol vision cones ------------------------------------------------
 * When the party skirts a patrolling sentry, its detection boundary draws as
 * marching-ant dots: the Manhattan diamond the trip test actually uses, full
 * radius across the watched forward half-plane (bright gold), half radius
 * across the peripheral rear (dim violet). Nearest non-chasing patroller
 * only; a chaser's cone is moot.
 *
 * OBJ slots 29..38, claimed here. Deliberately NOT shared with battle
 * garnish (tether/glyphs 24-28, popups 40+, zZ 50+, dice 56+) even though
 * cones are field-mode and tether is battle-mode: a dedicated range means a
 * missed cone_hide() can never repaint battle garnish mid-fight. Every exit
 * from the field loop (dialog, menu, battle, room change) hides the dots
 * first, so they cannot linger into those modes. */
#define OBJ_CONE  29
#define CONE_DOTS 10

static void cone_hide(void) {
    for (int i = 0; i < CONE_DOTS; i++) obj_hide(OBJ_CONE + i);
}

static void cone_draw(void) {
    int best = -1, bestd = 0;       /* nearest patrolling, non-chasing sentry */
    for (int i = 0; i < nnpc; i++) {
        Npc* n = &npcs[i];
        if ((n->flags & (NPC_PATROL | NPC_GONE)) != NPC_PATROL || n->chasing)
            continue;
        int adx = ppx - n->x, ady = ppy - n->y;
        if (adx < 0) adx = -adx;
        if (ady < 0) ady = -ady;
        if (best < 0 || adx + ady < bestd) { best = i; bestd = adx + ady; }
    }
    if (best >= 0) {
        Npc* n = &npcs[best];
        static const s8 fxs[4] = { 0, 0, -1, 1 }, fys[4] = { 1, -1, 0, 0 };
        int fx = fxs[n->face], fy = fys[n->face];   /* forward unit */
        int rf = sense_r(n, fx, fy);                /* the SAME oracle npc_ai */
        int rr = sense_r(n, -fx, -fy);              /*   trips on, both halves */
        if (rf < 2 || bestd >= rf * 3 / 2) { cone_hide(); return; }
        if (!cone_seen) { cone_seen = 1; mgba_logf("cone shown npc=%d", best); }
        /* the boundary is a kite: forward diamond half at rf, rear half at
         * rr, flat jogs along the equator where the half-planes meet. Walk
         * its L1 perimeter, dots evenly spaced, phase marching with ticks. */
        int px = -fy, py = fx;                      /* perpendicular unit */
        int vx[6] = { px * rf, fx * rf, -px * rf, -px * rr, -fx * rr, px * rr };
        int vy[6] = { py * rf, fy * rf, -py * rf, -py * rr, -fy * rr, py * rr };
        int len[6] = { 2 * rf, 2 * rf, rf - rr, 2 * rr, 2 * rr, rf - rr };
        int per = 6 * rf + 2 * rr;
        int scx = n->x + 8 - cam_x, scy = n->y + 8 - cam_y;
        for (int k = 0; k < CONE_DOTS; k++) {
            int s = (k * per / CONE_DOTS + (int)((ticks >> 1) % (u32)per)) % per;
            int seg = 0;
            while (seg < 5 && s >= len[seg]) s -= len[seg++];
            int L = len[seg] ? len[seg] : 1;
            int dx = vx[seg] + (vx[(seg + 1) % 6] - vx[seg]) * s / L;
            int dy = vy[seg] + (vy[(seg + 1) % 6] - vy[seg]) * s / L;
            /* tint by asking the oracle again: full radius here = watched
             * (bright), halved = peripheral (dim) -- no hand-kept copy of
             * the half-plane test to drift */
            int watched = sense_r(n, dx, dy) == rf;
            obj_set(OBJ_CONE + k, scx + dx - 4, scy + dy - 4, 0,
                    OBJT_GARN, watched ? 9 : 13, 2);
        }
        return;
    }
    cone_hide();
}

static void npc_ai(void) {
    for (int i = 0; i < nnpc; i++) {
        Npc* n = &npcs[i];
        if ((n->flags & (NPC_PATROL | NPC_GONE)) != NPC_PATROL) continue;
        int ddx = ppx - n->x, ddy = ppy - n->y;
        int adx = ddx < 0 ? -ddx : ddx, ady = ddy < 0 ? -ddy : ddy;
        int dist = adx + ady;
        int r = sense_r(n, ddx, ddy);
        if (!n->chasing) {
            /* lazy hover-drift around home */
            int ph = (int)((ticks + i * 37) >> 3) & 31;
            int off = ph < 16 ? ph - 8 : 23 - ph;
            n->x = (s16)(n->hx + off);
            if (dist < r) {
                n->chasing = 1;
                alert_npc = i; alert_t = 36;
                sfx_play(SFX_CANCEL);
                mgba_logf("aggro npc %d", i);
            }
        } else {
            if (ticks & 1) continue;               /* chase at ~30px/s */
            n->x = (s16)(n->x + (ddx > 0 ? 1 : ddx < 0 ? -1 : 0));
            n->y = (s16)(n->y + (ddy > 0 ? 1 : ddy < 0 ? -1 : 0));
            n->face = (u8)(adx > ady ? (ddx > 0 ? 3 : 2) : (ddy > 0 ? 0 : 1));
            if (dist <= 16) {
                n->chasing = 0;
                cone_hide();                       /* battle: no cone garnish */
                ev_aggro(i);
                return;                            /* npc list may have changed */
            }
        }
    }
}

static void draw_alert(void) {
#ifdef OBJT_ALERT
    if (alert_t > 0 && alert_npc >= 0 && !(npcs[alert_npc].flags & NPC_GONE)) {
        alert_t--;
        obj_set(OBJ_ALERT, npcs[alert_npc].x - cam_x,
                npcs[alert_npc].y - cam_y - 16, 1, OBJT_ALERT, 9, 0);
    } else obj_hide(OBJ_ALERT);
#else
    if (alert_t > 0) alert_t--;
    obj_hide(OBJ_ALERT);
#endif
}

void field_run(void) {
    static const s8 dx[4] = {0, 0, -1, 1}, dy[4] = {1, -1, 0, 0};
    int tdx = 0, tdy = 0;
    while (!exit_req) {
        frame();
        G_FIELD_IDLE = 1;
        if (G_SKILL_TEST) { int sk = G_SKILL_TEST - 1; G_SKILL_TEST = 0;
            cone_hide();
            void ev_skill_test(int, int); ev_skill_test(sk, 12); }
        if (G_AUDIT) { G_AUDIT = 0; cone_hide();
            void ability_audit(void); ability_audit();
            void sheet_audit(void); sheet_audit(); }
        npc_ai();
        draw_alert();
        if (!pmoving) {
            u16 held = key_state();
            int dir = -1;
            if (held & KEY_DOWN) dir = 0;
            else if (held & KEY_UP) dir = 1;
            else if (held & KEY_LEFT) dir = 2;
            else if (held & KEY_RIGHT) dir = 3;

            static u8 turn_t;                 /* tap turns; hold walks */
            if (dir < 0) turn_t = 0;              /* released: window closes */
            if (dir >= 0) {
                if (dir != (int)pface) { pface = (u8)dir; turn_t = 3; }
                else if (turn_t) turn_t--;
                else {
                    int nx = ppx / 16 + dx[dir], ny = ppy / 16 + dy[dir];
                    if (walkable(nx, ny)) {
                        pmoving = 1; pstep = 0; tdx = dx[dir]; tdy = dy[dir];
                    }
                }
            } else if (key_hit() & KEY_A) {
                int fx = ppx / 16 + dx[pface], fy = ppy / 16 + dy[pface];
                int ni = npc_at(fx, fy);
                cone_hide();                       /* dialog: no cone garnish */
                if (ni >= 0) ev_npc(ni);
                else ev_interact(fx, fy);
            } else if (key_hit() & KEY_START) {
                void field_menu(void);
                cone_hide();
                field_menu();
                ev_light();   /* Equip may have (un)sheathed the Everburn */
            }
        } else {
            ppx += tdx * 2; ppy += tdy * 2;
            if (++pstep >= 8) {
                pmoving = 0;
                cone_hide();          /* ev_step may open a door or a fight */
                ev_step(ppx / 16, ppy / 16);
            }
        }
        cone_draw();   /* after any dispatch: a no-op event redraws same frame */
        field_draw();
    }
    exit_req = 0;
}
