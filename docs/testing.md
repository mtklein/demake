# Testing the rules engine and the host game layer

This doc states the testing law. It deliberately carries **no test counts
and no coverage percentages** — those rot within days. `make gate` prints
the live numbers every run; the ratchet floors in the Makefiles are the
only numbers under version control, and they're enforced, so they can't
lie.

## How tests are counted

A "test" is a test function registered in a `tests[]` table and reported
as one `ok <name>` line. Many tests run Monte-Carlo sampling loops
internally; those assertion executions are real checks but they are
**not** tests, and the suite never headlines them. The summary line reads:

    N tests, 0 failures (M assertion executions — sampling loops inflate this figure)

The first number is the honest one. A failing check prints
`FAIL file:line: expr`, marks its test `FAILED`, and the binary exits
nonzero.

## Running

    make test-rules          # rules suite, native
    make coverage            # rules: instrumented build, table, HTML, ratchet
    make -C test/host sim    # game layer under ASan/UBSan (ARGS=substr filters)
    make -C test/host cov    # game layer: table + ratchet

`make gate` runs all of it — suites first (via `test/gate.sh`), both
coverage ratchets after — so every gate run prints the numbers and
enforces the floors.

## Coverage reports

Both coverage targets build with Homebrew LLVM
(`-fprofile-instr-generate -fcoverage-mapping`) and write under `build/`
(gitignored):

- `build/cov/report.txt` + `build/cov/html/` — rules/, scoped to the
  engine sources (`dice.c`, `core.c`, `features.c`); the test file and
  generated SRD tables are compiled in but not measured.
- `build/host/report.txt` — the game layer, scoped to the five game
  sources (`menu.c`, `data.c`, `party5.c`, `encounter.c`, `game.c`).

## The ratchets

`COV_FLOOR` (Makefile, rules/) and `HOST_COV_FLOOR` (test/host/Makefile,
game layer) are floors on TOTAL line coverage. If a report's TOTAL drops
below its floor, the target — and therefore `make gate` — fails loudly:

    COVERAGE RATCHET: lines 93.4% < floor 94%

Rule: **raise the floor as coverage rises; never lower it.** When new
tests push the total up, bump the floor to (measured, rounded down, minus
one) in the same commit. rules/ sits at 100% and its floor holds the
ceiling; the game layer's floor rises as the thickest remaining slice
(encounter.c's tray/bar/camera presentation weave, which the ROM
scenarios exercise end-to-end) gets native tests.

## The host game layer (test/host)

Same doctrine as rules/, one layer up: `make -C test/host sim` compiles
the REAL game sources against a fake engine and runs them under
ASan/UBSan — menu kits and sheets, equip and items, real battles, the
title/jukebox/karaoke/name-entry flows. Two laws, both paid for in
shipped bugs:

- **Test the real code path, never a mirror of its logic.** Expectation
  tables are written against the design docs and the SRD, not read out of
  the code under test. Three shipped buffer overflows lived in code that
  only a mirror was watching.
- **Stack the deck, don't script the story.** Battle tests reach
  encounter internals through the real WISELY tactic brain by arranging
  composition, tactics, and a downed companion so the dispatcher is
  *forced* to take the path under test. Menu rows that only manual play
  reaches (WildShape, Attack targeting) go through eb_gen-style key
  generators with randomized idle gaps (a fixed period phase-locks with
  the menu loop).

Determinism: the fake engine has no wall clock; `rnd()` is seeded, so a
passing battle passes forever until the code (and thus the RNG stream)
changes — at which point the assertions and the ratchet, not luck, decide
whether coverage survived.
