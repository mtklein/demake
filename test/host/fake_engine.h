/* Test-facing API of the fake engine (test/host/fake_engine.c).
 *
 * The fake implements the whole engine/hardware surface (engine.h, field.h,
 * audio.h, util/panic) so the REAL game-logic files -- menu.c, data.c,
 * party5.c, encounter.c, game.c -- link and run natively. Text lands in an
 * assertable character grid, input comes from a deterministic script or a
 * generator function, sprites/audio/logs are recorded. */
#ifndef FAKE_ENGINE_H
#define FAKE_ENGINE_H
#include "gba.h"   /* the shim (already force-included by the build) */

/* -------- lifecycle -------- */
void sim_reset(void);                  /* wipe fake + game state between tests */
void sim_seed(u32 seed);               /* rnd() stream (xorshift, no wall clock) */
void sim_budget(u32 frames, int pass_on_budget);
u32  sim_frames(void);

/* Run fn with failure trapping. 0 = ok, 1 = sim_fail/panic fired,
 * 2 = frame budget reached with pass_on_budget set (fuzzers). */
int  sim_guard(void (*fn)(void));
void sim_fail(const char* fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));
extern char sim_fail_msg[512];

/* -------- scripted input --------
 * Tokens, space-separated: A B UP DOWN LEFT RIGHT START SELECT L R
 * (each = 1 frame pressed + 1 frame released), "." (1 idle frame),
 * "Wn" (n idle frames), "SNAP" (capture the text grid; 1 idle frame). */
void script_keys(const char* s);
int  script_left(void);
/* Generator mode overrides the queue: called once per frame() for the key
 * mask; may inspect/mutate game state (it is host test code). */
void script_gen(u16 (*fn)(u32 frame));

/* -------- text grid: 30 cols x 32 rows (rows 0..19 on-screen) -------- */
#define GRID_W 30
#define GRID_H 32
const char* grid_row(int y);              /* right-trimmed, NUL-terminated */
int  grid_contains(const char* s);        /* substring match on any row */
void grid_dump(void);                     /* to stderr */
int  snap_count(void);
const char* snap_row(int snap, int y);
int  snap_contains(int snap, const char* s);

/* -------- sprite table -------- */
typedef struct { int x, y, shape, tile, pal, prio, shown; } SimObj;
const SimObj* sim_obj(int i);

/* -------- captured mgba log -------- */
int  log_contains(const char* s);
void log_dump(void);

/* -------- records / invariants -------- */
extern int  sim_last_music;               /* last music(id), -1 none */
extern int  sim_violations;               /* fake-engine invariant breaks */
extern char sim_violation_msg[256];
extern int  sim_level_up_choices_calls;   /* events.c stand-in hit count */

/* -------- field fake knobs -------- */
void sim_player_at(int x, int y);         /* field_player_x/y result, px */

#endif
