# Nautiloid — working conventions

A bare-metal GBA demake of BG3's prologue (FF4 presentation, real 5e SRD
combat, CT-style on-map encounters). No devkitPro/libgba: Homebrew LLVM
(`brew install llvm lld`), custom crt0.s + gba.ld, `tools/fixrom.py` header.
src/aeabi.c is the entire runtime (AEABI division + memory intrinsics).

## The gate

**`make gate` must be green before any commit lands on main.** It builds the
ROM and runner, runs the native 5e rules suite, the AEABI division tests,
and the ASan/UBSan host sim, lints every song, drives the scripted scenario
fleet (full playthroughs, feature checks, a deck-brawl smoke for all 12
classes) with structural assertions, then enforces both coverage ratchets.
Main is always finishable; the gate is what "finishable" means
operationally. Scenario checks assert structure (no crash/timeout, battles
resolved, correct ending), never exact xp — xp shifts legitimately whenever
timing changes move the tick-seeded RNG (even editing dialog text does this).

Main history is linear: one committer controls main, so work lands by
fast-forward from the working branch (rebase the branch first if main
moved). No merge commits.

## Evolution philosophy

Best Simple System for Now (dannorth.net/blog/best-simple-system-for-now):
move stepping-stone to stepping-stone; each commit a checkpoint we'd proudly
call "best for what it's trying to do." Replace rather than retrofit when
understanding changes (the FF4 battle system was deleted, not wrapped).
Deletion is healthy. No speculative generality.

Rigor follows stability, in three named tiers: rules/ is proven ground —
total coverage is the standing bar, ratcheted at COV_FLOOR, because the
SRD never changes under us; the game layer (encounter, menu, game, data,
party5) is load-bearing system — host-tested behaviorally under
sanitizers, ratcheted at HOST_COV_FLOOR; events.c stays cheap-to-rewrite
content, covered end-to-end by the scenario fleet. When code graduates a tier — content hardening into
system — its tests graduate in the same commit, not "later": Character 2.0
landed twelve classes of battle logic on scenario slots alone, and the
behavioral tests had to be back-filled in bulk. Don't repeat that.

## Verification architecture

- `test/runner.c` links Homebrew libmgba: scripted input, screenshots
  (`shot`), memory `peek`/`poke`, `until ADDR VAL MAXF` sync.
- Determinism comes from poked EWRAM demo flags (see src/game.h): G_DEMO
  auto-advances dialogs, G_DEMO_BATTLE picks battle policy, G_FIELD_IDLE and
  G_DONE are until-sync points; a magic cookie at 0x0203FF38 stops gba_init
  from wiping the block. `test/scenario.py setup()` pokes all of it.
- Never redirect runner output to a file during crash hunts (a crashed ROM
  firehoses the log); pipe through `awk '/Illegal opcode/...'` instead.
- The testing ladder: new game LOGIC lands with a host test that pins its
  behavior — test/host is the default home for verification (native,
  deterministic, sanitized). A scenario slot is for what only the ROM can
  prove: input and frame timing, room transitions, demo-flag plumbing, the
  crash screen. Scenarios assert structure, so they are the integration
  net, never the behavior spec — a feature with only a scenario isn't
  tested, it's merely not crashing. Either way: canned and in the gate,
  never an ad-hoc script.
- `test/host/` compiles the REAL game logic (menu, encounter, data, game)
  natively against a fake engine under ASan/UBSan: `make -C test/host sim`
  (real-path tests + seeded menu fuzz), `make -C test/host cov` for llvm-cov.
  Both run inside the gate. Three shipped buffer overflows lived in this
  layer; test the real code path, never a mirror of its logic. Battle tests
  reach encounter internals through the real WISELY tactic brain by stacking
  the deck (composition, tactics, a downed companion); menu rows (WildShape,
  manual Attack) go through eb_gen-style key generators. Stack the deck so
  the story is forced, not scripted.
- Honest counting: tests are counted by test FUNCTION; sampling-loop
  iterations are never a headline number. Two coverage ratchets run inside
  the gate: `make coverage` holds rules/ lines at COV_FLOOR, and
  `make -C test/host cov` holds the five game sources at HOST_COV_FLOOR —
  raise them, never lower.
- Docs state law, never state: no test counts, no coverage percentages, no
  scenario tallies in prose — the gate prints the live numbers every run,
  and the ratchet floors in the Makefiles are the only numbers docs may
  cite (they're enforced, so they can't lie). Design docs carry a Status
  line that gets flipped the day reality changes. (This file said "seven
  scripted playthroughs" while the gate ran twenty-nine.)

## Music

- Source of truth is `tools/music/songs.py` (note data + TITLES); the
  counterpoint linter `tools/music/check_music.py` must pass (FORMS map
  documents each song's structure; ECHO_SONGS waives two-square harmony
  checks for delay-echo arrangements).
- New track: agent/human writes a take (see tools/music/takes/ for the
  format), `tools/music/press.py` validates and registers it. Replacing an
  existing track: `tools/music/recut.py`. Multi-player isolated-session
  work: `tools/music/assemble.py` (the Kind of Azure exemplar).
- Jukebox names generate from TITLES; adding a track needs one songs.py
  entry plus a trk row in tools/ui_screens.py's jukebox screen (a build
  assert catches forgetting either).
- House taste: genre-authentic grammar over pastiche; percussion sits under
  the melody (the ride-fatigue lesson); hymn-grammar passages land hardest.

## Licensing bright lines

- SRD 5.1/5.2.1 content is CC-BY-4.0: keep the attribution block in
  README.md and in release notes that ship a ROM.
- Non-SRD monsters (devourer, cambion, mind flayer) ship ONLY as original
  homebrew stand-ins in tools/srd/overrides.py — never MM stat copies.
- All melodies are original. Genre/idiom imitation is fine; quoting a
  copyrighted tune (So What, Eyes on Me, THAXTED) is not.

## Releases

Session end + gate green → push (with the user's go-ahead) and cut a GitHub
release: dated tag (`YYYY-MM-DD` or `YYYY-MM-DD-<theme>`), `build/nautiloid.gba`
as the asset, player-facing notes, SRD attribution line. Remote is `github`,
not `origin`.
