# Testing the rules engine

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

## Numbers as of 2026-07-06

21 test functions, 0 failures. `COV_FLOOR := 94`.

| File        | Regions | Functions | Lines   | Branches |
|-------------|---------|-----------|---------|----------|
| dice.c      | 94.59%  | 100.00%   | 100.00% | 90.91%   |
| core.c      | 93.68%  | 100.00%   | 100.00% | 85.92%   |
| features.c  | 72.29%  | 85.00%    | 84.27%  | 58.93%   |
| **TOTAL**   | 88.06%  | 92.86%    | 95.61%  | 79.55%   |

Known gaps (from `llvm-cov report -show-functions` and the branch view;
candidates for targeted tests):

- `r5_skill_check` never called (0%) — live in `src/events.c` field
  checks, so proficiency, expertise, Lucky-on-checks, and the `d20out`
  pointer are untested natively.
- `r5_rage_bonus` and `r5_savage_crit_dice` never called — by tests *or*
  by game code; both effects are implemented inline in
  `r5_weapon_attack`. Test them or delete them.
- `r5_monster_attack` always gets a NULL attacker: the attacker
  `crit_min` path is untested; `r5_prof`'s monster branch likewise.
- Rage damage-bonus exclusions unexercised: raging with a ranged weapon
  and raging with a finesse weapon (DEX > STR) never occur.
- Lucky is only tested via bare `r5_d20`: no Lucky attacker through
  `r5_weapon_attack`, no Lucky saver through `r5_save`.
- `r5_apply_damage`: partial temp-hp absorption (damage smaller than the
  pool) and the Relentless-on-a-monster guard are untested.
- Guard rails never tripped: `r5_roll` dice-count clamp, `r5_seed(0)`
  fallback, level clamps in `r5_prof`/`r5_sneak_dice`, out-of-range
  `r5_spend_slot`, `r5_refill` bad-class guard, `r5_heal(<=0)`,
  `r5_lay_hands(<=0)`.
- Denial paths half-tested: `r5_can_second_wind` on a downed fighter,
  `r5_can_action_surge` on a non-fighter, `r5_can_rage` with an empty
  pool or non-barbarian, `r5_martial_die` on a non-monk.
- `damage_roll`: extra d6s merged into a d6-sided weapon (sneak/mark
  with a shortsword-class die) takes an untested path.
