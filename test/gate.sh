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
           chapel_fight chapel_parley crypt_withers camp_night warryn_check \
           gates_wyll gates_reroute_wyll gates_full \
           sneak_strike cone_ambush cone_show helm_sleepz tether_check panic_check \
           wildshape_check levelup_check prepare_check origin_check \
           origin_flow_check durge_check creation_check skill_check \
           loot_check audit_check; do
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
        # the campfire rest healed a poked-to-1-hp Tav back to full
        camp_night)                           want="camp rest from=1 full=1" ;;
        warryn_check)                         want="warryn lever" ;;
        # the sixth soul: walking party full, so Wyll lands on the bench
        gates_wyll)                           want="gates recruit wyll walk=3 reserve=1" ;;
        gates_reroute_wyll)                   want="gates reroute wyll" ;;
        # the thorough route's ledger, exact and xp-proof: six souls
        # gathered, one camp night slept (fixed by the route, not the dice)
        gates_full)                           want="gates tally souls=6 rests=1" ;;
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
        # a warlock loots + auto-equips the duelist's kit; the marker proves
        # the loot fired, and the crash/PANIC checks below prove find[cls]
        # handed a live weapon, not a wild pointer, to the Equip render
        loot_check)                           want="duelist find w=" ;;
        audit_check)                          want=""                ;;  # golden diff below
        *)                                    want="enc result=WIN"  ;;
    esac
    bad=""
    echo "$out" | grep -q "Illegal opcode" && bad="crash"
    echo "$out" | grep -q "TIMEOUT"        && bad="${bad:+$bad,}timeout"
    case "$scn" in tether_check|panic_check|wildshape_check|levelup_check|prepare_check|origin_check|origin_flow_check|durge_check|creation_check|skill_check|audit_check|beach_medicine|beach_origin|beach_recruits|beach_reroute_astarion|beach_reroute_gale|chapel_parley|camp_night|warryn_check|loot_check) ;; *)
        echo "$out" | grep -q "enc result" || bad="${bad:+$bad,}no-battles" ;;
    esac
    # beach scenarios assert the arc's structure, never exact xp: the wake
    # fired, the recoveries landed, the devourer entered the initiative log.
    # Canon-subclass marks are xp-proof too: Shadowheart's Masks fires at
    # cleric's level-1 reveal on recruit; Gale benches at L2+ (wizard's 2).
    case "$scn" in
        bard_full)      extra="subclass canon SHADOW. -> 16" ;;
        beach_full)     extra="beach wake|result=CONNECTED|beach recover shadowheart|init Devourer" ;;
        beach_medicine) extra="beach wake|beach recover shadowheart|field-die d20=" ;;
        beach_flayer)   extra="beach recover shadowheart|field-check Arcana|field-die d20=|init Devourer|enc result=WIN" ;;
        beach_origin)   extra="beach wake|beach recover laezel" ;;
        # the recruit route: both beats land, the bench fills (5 souls), and
        # the Party-row swap's own log seals it (the want= above)
        beach_recruits) extra="beach wake|beach recover shadowheart|beach recover laezel|beach recruit astarion walk=3 reserve=0|beach recruit gale walk=3 reserve=2|subclass canon SHADOW. -> 16|subclass canon GALE -> 9" ;;
        beach_reroute_astarion) extra="beach wake|boar beat fed" ;;
        beach_reroute_gale)     extra="beach wake|sigil beat rerouted" ;;
        # stone 4: chapel + crypt + darkvision + Withers, structure only
        # (the band is four since stone 6: the masked one fights with it)
        chapel_fight)   extra="init Bandit|enc result=WIN|looters resolved|tomb door opens" ;;
        # stone 6: the masked looter -- noticed, spoken to, levered past
        warryn_check)   extra="beach wake|warryn stirs|looters resolved|tomb door opens" ;;
        # stone 6: the gates -- goblins + warchief in initiative, BOTH allies
        # take real side-2 turns, the assault breaks, the sixth soul lands,
        # and the tally closes the arc (G_DONE=3)
        gates_wyll)     extra="beach wake|gates wyll dueling|init Goblin|init Warchief|init Zevlor|init Wyll|Zevlor side2|Wyll side2|enc song=|enc result=WIN|gates held|gates tally" ;;
        gates_reroute_wyll) extra="beach wake|init Goblin|init Warchief|init Zevlor|Zevlor side2|enc result=WIN|gates held|gates tally" ;;
        # the whole arc, thorough: ship battles + connect, every recovery
        # and recruit, the masked one, three devourers, the band, the dead,
        # the camp night, then the gates -- with the level-3 reveal firing
        # AT the finale (subclass pick TAV is the bard's L3 moment; Wyll
        # joins already at 3 and takes the Fiend through party_add) and six
        # souls at the fire afterward
        gates_full)     extra="result=CONNECTED|init Thrall|beach recover shadowheart|flayer beat finished|beach recruit astarion walk=3 reserve=0|init Devourer|beach recover laezel|beach recruit gale walk=3 reserve=2|party swap in=LAE'ZEL out=ASTAR.|warryn stirs|init Bandit|init Skeleton|camp scene begins|camp rest from=|init Goblin|init Warchief|Zevlor side2|Wyll side2|subclass pick TAV|subclass canon LAE'ZEL|subclass canon WYLL|gates recruit wyll walk=3 reserve=3|gates held|camp souls=6" ;;
        chapel_parley)  extra="field-check Persuasion|field-die d20=|looters resolved|tomb door opens|dark room=8 dim=1" ;;
        crypt_withers)  extra="dark room=8 dim=1|init Skeleton|enc result=WIN|withers wakes|dark room=10 dim=1|subclass pick Wyll" ;;
        # stone 5: three souls reach the camp, the scene fires once with a
        # synced verse (lyric row logged), the demo autoskip moves it along,
        # and a companion stands by the fire with a word in her
        camp_night)     extra="beach wake|camp souls=3|camp scene begins souls=3|lyric 1 @|karaoke autoskip|camp scene played|camp talk SHADOW." ;;
        # the poked skill check: the roll is logged AND a die was shown for it
        skill_check)    extra="field-die d20=" ;;
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
    if [ "$scn" = camp_night ]; then
        # the no-replay property: the scenario re-enters camp after the
        # night, and the Under Selune scene must have fired exactly once
        n=$(echo "$out" | grep -c "camp scene begins")
        [ "$n" -eq 1 ] || bad="${bad:+$bad,}scene-count=$n"
    fi
    if [ "$scn" = chapel_fight ]; then
        # the masked one fights with the band: four bandits enter initiative
        # (`|| true`: grep -c exits 1 at zero matches, and under set -e a
        #  zero count must FAIL the slot, not silently kill the gate)
        n=$(echo "$out" | grep -c "init Bandit" || true)
        [ "$n" -eq 4 ] || bad="${bad:+$bad,}band-count=$n"
    fi
    if [ "$scn" = gates_reroute_wyll ]; then
        # story surgery: the player IS Wyll -- nobody may recruit him
        n=$(echo "$out" | grep -c "gates recruit wyll" || true)
        [ "$n" -eq 0 ] || bad="${bad:+$bad,}wyll-double=$n"
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
