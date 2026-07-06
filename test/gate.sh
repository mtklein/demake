#!/bin/sh
# The verification gate: everything that must be green before a commit lands.
# usage: test/gate.sh   (or: make gate)
#
# Scenario checks are structural -- no crash, no timeout, battles resolved,
# the right ending marker. Exact xp is deliberately NOT asserted: it shifts
# legitimately whenever timing (music length, dialog text) moves the
# tick-seeded RNG.
set -e
cd "$(dirname "$0")/.."

echo "== build (ROM + runner) =="
make -s all runner

echo "== 5e rules suite (native) =="
make -s test-rules | tail -1

echo "== AEABI division helpers (native) =="
make -s test-aeabi | tail -1

echo "== counterpoint lint =="
python3 tools/music/check_music.py >/dev/null
echo "music lint: OK"

echo "== scenarios =="
fail=0
for scn in bard_full wizard_zhalk rogue_mutilate ranger_full \
           sneak_strike cone_ambush helm_sleepz; do
    python3 test/scenario.py "$scn" > build/gate.script
    if ! out=$(./build/runner build/nautiloid.gba build/gate.script 2>&1); then
        echo "FAIL $scn (runner exited nonzero)"; fail=1; continue
    fi
    case "$scn" in
        bard_full|rogue_mutilate|ranger_full) want="result=CONNECTED" ;;
        *)                                    want="enc result=WIN"  ;;
    esac
    bad=""
    echo "$out" | grep -q "Illegal opcode" && bad="crash"
    echo "$out" | grep -q "TIMEOUT"        && bad="${bad:+$bad,}timeout"
    echo "$out" | grep -q "enc result"     || bad="${bad:+$bad,}no-battles"
    echo "$out" | grep -q "$want"          || bad="${bad:+$bad,}missing:$want"
    if [ -n "$bad" ]; then
        echo "FAIL $scn ($bad)"; fail=1
    else
        echo "  ok $scn   $(echo "$out" | grep -oE 'xp=[0-9]+|result=[A-Z]+' | tr '\n' ' ')"
    fi
done

if [ "$fail" -eq 0 ]; then echo "GATE: all green"; else echo "GATE: FAILED"; exit 1; fi
