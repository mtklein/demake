#!/bin/sh
# The verification gate: everything that must be green before a commit lands.
# usage: test/gate.sh   (or: make gate)
#
# Scenario checks are structural -- no crash, no timeout, battles resolved,
# the right ending marker. Exact xp is deliberately NOT asserted: it shifts
# legitimately whenever timing (music length, dialog text) moves the
# tick-seeded RNG.
set -e
# (callers: never pipe `make gate` through tail/grep without pipefail --
#  a laundered exit status once let two broken commits land)
cd "$(dirname "$0")/.."
mkdir -p test/shots   # gitignored; fresh clones/worktrees lack it and every scenario fails

echo "== build (ROM + runner) =="
make -s all runner

# (assignment form, not `make | tail`: a pipe launders make's exit status
#  under plain set -e, and this file's own header warns callers about that)
echo "== 5e rules suite (native) =="
out=$(make -s test-rules); echo "$out" | tail -1

echo "== AEABI division helpers (native) =="
out=$(make -s test-aeabi); echo "$out" | tail -1

echo "== host sim: real game logic under ASan/UBSan =="
out=$(make -s -C test/host sim); echo "$out" | tail -1

echo "== counterpoint lint =="
python3 tools/music/check_music.py >/dev/null
echo "music lint: OK"

echo "== scenarios =="
fail=0
for scn in bard_full wizard_zhalk rogue_mutilate ranger_full \
           beach_full beach_medicine beach_flayer beach_origin \
           beach_recruits beach_reroute_astarion beach_reroute_gale \
           chapel_fight chapel_parley crypt_withers \
           sneak_strike cone_ambush cone_show helm_sleepz tether_check panic_check \
           wildshape_check levelup_check prepare_check origin_check \
           origin_flow_check durge_check creation_check skill_check \
           audit_check; do
    python3 test/scenario.py "$scn" > build/gate.script
    if ! out=$(./build/runner build/nautiloid.gba build/gate.script 2>&1); then
        echo "FAIL $scn (runner exited nonzero)"; fail=1; continue
    fi
    case "$scn" in
        bard_full|rogue_mutilate|ranger_full) want="result=CONNECTED" ;;
        beach_full)                           want="beach recover laezel" ;;
        beach_medicine)                       want="field-check Medicine" ;;
        beach_flayer)                         want="flayer beat finished" ;;
        beach_origin)                         want="beach reroute shadowheart" ;;
        beach_recruits)                       want="party swap in=LAE'ZEL out=SHADOW." ;;
        beach_reroute_astarion)               want="beach reroute astarion" ;;
        beach_reroute_gale)                   want="beach reroute gale" ;;
        chapel_fight)                         want="tomb door opens" ;;
        chapel_parley)                        want="dark room=8 dim=1" ;;
        crypt_withers)                        want="withers repick" ;;
        tether_check)                         want="tether"          ;;
        cone_show)                            want="cone shown npc=" ;;
        panic_check)                          want="PANIC poked"     ;;
        wildshape_check)                      want="wildshape boar"  ;;
        levelup_check)                        want="subclass pick"   ;;
        prepare_check)                        want="prepare-screen"  ;;
        origin_check|origin_flow_check)       want="origin=4 class=5 sub=16" ;;
        durge_check)                          want="urge-line"       ;;
        # hill-dwarf sage wizard, CON/INT swapped: 8/13/15/14/12/10 + CON+2 WIS+1
        creation_check)                       want="create race=1 bg=3 ab=8/13/17/14/13/10" ;;
        skill_check)                          want="field-check Arcana" ;;
        audit_check)                          want=""                ;;  # golden diff below
        *)                                    want="enc result=WIN"  ;;
    esac
    bad=""
    echo "$out" | grep -q "Illegal opcode" && bad="crash"
    echo "$out" | grep -q "TIMEOUT"        && bad="${bad:+$bad,}timeout"
    case "$scn" in tether_check|panic_check|wildshape_check|levelup_check|prepare_check|origin_check|origin_flow_check|durge_check|creation_check|skill_check|audit_check|beach_medicine|beach_origin|beach_recruits|beach_reroute_astarion|beach_reroute_gale|chapel_parley) ;; *)
        echo "$out" | grep -q "enc result" || bad="${bad:+$bad,}no-battles" ;;
    esac
    # beach scenarios assert the arc's structure, never exact xp: the wake
    # fired, the recoveries landed, the devourer entered the initiative log.
    # Canon-subclass marks are xp-proof too: Shadowheart's Masks fires at
    # cleric's level-1 reveal on recruit; Gale benches at L2+ (wizard's 2).
    case "$scn" in
        bard_full)      extra="subclass canon SHADOW. -> 16" ;;
        beach_full)     extra="beach wake|result=CONNECTED|beach recover shadowheart|init Devourer" ;;
        beach_medicine) extra="beach wake|beach recover shadowheart" ;;
        beach_flayer)   extra="beach recover shadowheart|field-check Arcana|init Devourer|enc result=WIN" ;;
        beach_origin)   extra="beach wake|beach recover laezel" ;;
        # the recruit route: both beats land, the bench fills (5 souls), and
        # the Party-row swap's own log seals it (the want= above)
        beach_recruits) extra="beach wake|beach recover shadowheart|beach recover laezel|beach recruit astarion walk=3 reserve=0|beach recruit gale walk=3 reserve=2|subclass canon SHADOW. -> 16|subclass canon GALE -> 9" ;;
        beach_reroute_astarion) extra="beach wake|boar beat fed" ;;
        beach_reroute_gale)     extra="beach wake|sigil beat rerouted" ;;
        # stone 4: chapel + crypt + darkvision + Withers, structure only
        chapel_fight)   extra="init Bandit|enc result=WIN|looters resolved|tomb door opens" ;;
        chapel_parley)  extra="field-check Persuasion|looters resolved|tomb door opens|dark room=8 dim=1" ;;
        crypt_withers)  extra="dark room=8 dim=1|init Skeleton|enc result=WIN|withers wakes|dark room=10 dim=1|subclass pick Wyll" ;;
        *)              extra="" ;;
    esac
    if [ -n "$extra" ]; then
        IFS='|'
        for pat in $extra; do
            echo "$out" | grep -q "$pat" || bad="${bad:+$bad,}missing:$pat"
        done
        unset IFS
    fi
    if [ "$scn" = audit_check ]; then
        # golden diff, not a want grep: the full per-class ability dump must
        # match test/audit.golden byte-for-byte (cross-class leaks fail here)
        echo "$out" | grep -E '^\[mgba\] (audit|sheet) ' | sed 's/^\[mgba\] //' > build/audit.out
        if ! diff -u test/audit.golden build/audit.out; then
            echo "audit_check: ability audit diverged from test/audit.golden ^^^"
            bad="${bad:+$bad,}audit-golden-diff"
        fi
    else
        echo "$out" | grep -q "$want"      || bad="${bad:+$bad,}missing:$want"
    fi
    if [ "$scn" != panic_check ] && echo "$out" | grep -q "PANIC"; then
        bad="${bad:+$bad,}panic"; echo "$out" | grep -A6 "PANIC" | head -10
    fi
    if [ -n "$bad" ]; then
        echo "FAIL $scn ($bad)"; fail=1
    else
        echo "  ok $scn   $(echo "$out" | grep -oE 'xp=[0-9]+|result=[A-Z]+' | tr '\n' ' ')"
    fi
done

echo "== class smokes (deck brawl x12) =="
for c in 0 1 2 3 4 5 6 7 8 9 10 11; do
    python3 test/scenario.py "smoke_$c" > build/gate.script
    if ! out=$(./build/runner build/nautiloid.gba build/gate.script 2>&1); then
        echo "FAIL smoke_$c (runner)"; fail=1; continue
    fi
    if echo "$out" | grep -q "Illegal opcode"; then echo "FAIL smoke_$c (crash)"; fail=1
    elif echo "$out" | grep -q "TIMEOUT"; then echo "FAIL smoke_$c (timeout)"; fail=1
    elif ! echo "$out" | grep -q "enc result=WIN"; then echo "FAIL smoke_$c (no win)"; fail=1
    else echo "  ok smoke_$c"
    fi
done

if [ "$fail" -eq 0 ]; then echo "GATE: all green"; else echo "GATE: FAILED"; exit 1; fi
