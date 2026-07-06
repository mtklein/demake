# Testing the rules engine and the host game layer

## How tests are counted

A "test" is a test function in `rules/test_rules.c`, registered in the
`tests[]` table and reported as one `ok <name>` line. Many tests run
Monte-Carlo sampling loops internally; those assertion executions are
real checks but they are **not** tests, and the suite never headlines
them. The summary line reads:

    21 tests, 0 failures (247726 assertion executions — sampling loops inflate this figure)

The first number is the honest one. A failing check prints
`FAIL file:line: expr`, marks its test `FAILED`, and the binary exits
nonzero.

## Running

    make test-rules     # build + run the native suite
    make coverage       # instrumented build, per-file table, HTML, ratchet

`make gate` runs both (coverage is wired into the Makefile `gate` target,
after `test/gate.sh`), so every gate run prints the numbers and enforces
the floor.

## Coverage reports

`make coverage` compiles the same sources as `test-rules` with Homebrew
LLVM (`-fprofile-instr-generate -fcoverage-mapping -g -O1`) and produces,
under `build/cov/` (gitignored like all of `build/`):

- `report.txt` — the `llvm-cov report` per-file table
  (Regions / Functions / Lines / Branches %), also printed to stdout
- `html/index.html` — browsable line-and-branch detail (`llvm-cov show`)

The table is scoped to the engine sources (`rules/dice.c`, `rules/core.c`,
`rules/features.c`). The test file and the generated SRD data tables are
compiled into the instrumented binary but are not what we measure.

## The ratchet

`COV_FLOOR` in the Makefile is a floor on TOTAL line coverage. If the
report's TOTAL line % drops below it, `make coverage` (and therefore
`make gate`) fails loudly:

    COVERAGE RATCHET: lines 93.4% < floor 94%

Rule: **raise the floor as coverage rises; never lower it.** When new
tests push the total up, bump `COV_FLOOR` to (measured, rounded down,
minus one) in the same commit.

## Rules numbers as of 2026-07-06

32 test functions, 0 failures. `COV_FLOOR := 99`. All three engine files
sit at **100%** regions, functions, lines, and branches; the gap list that
used to live here was closed by the 100%-coverage push (skill checks
proven, dead helpers deleted rather than tested — deletion is healthy).

## The host game layer (test/host)

Same doctrine, one layer up: `make -C test/host sim` compiles the REAL
game sources — `menu.c`, `data.c`, `party5.c`, `encounter.c`, `game.c`,
plus rules/ and the generated tables — against a fake engine and runs
them under ASan/UBSan. 29 test functions: menu kits and sheets, equip and
items, real battles driven through the WISELY tactic brain (the spell-cast
family, allies, the cambion warp, concentration breaking, wild shape),
generator-driven manual menus, and the title/jukebox/karaoke/name-entry
flows. Battle tests stack the deck (composition, tactics, a downed
companion) so the real dispatcher is forced to take the path under test;
nothing mirrors game logic.

`make -C test/host cov` prints the five-file table and ratchets TOTAL line
coverage at `HOST_COV_FLOOR` (same raise-never-lower rule). Both the sim
and the ratchet run inside `make gate`.

Host numbers as of 2026-07-06: **100% functions, 89.9% lines** over the
five game sources (`HOST_COV_FLOOR := 88`); per-file lines: menu 98.6%,
data 100%, party5 100%, encounter 83.7%, game 97.3%. The thickest
remaining slice is encounter.c's presentation weave (tray/bar/camera
choreography), which the ROM scenarios keep exercising end-to-end.
