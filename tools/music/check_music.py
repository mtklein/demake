#!/usr/bin/env python3
"""Structural lint for tools/music/songs.py.

Hard checks (exit 1 on failure):
  - every song has exactly 4 channels of equal length
  - ch0/ch1 values in 0..61; ch2 (wave) note values <= 47 (engine adds +12);
    ch3 (noise) values in {0,1,2,3,4, HOLD, REST}
  - loop < rows, or 0xFFFF (play once); VICTORY and CRASH must be play-once
  - strong-beat (row%4==0) square-vs-square both-attack dissonance <= 5%
  - voice crossing (ch1 sounding above ch0) <= 2% of rows where both sound

Reports (informational): form map, weak-beat dissonances that fail to resolve
by step, square spacing wider than a 12th, out-of-preferred-range notes,
pitch-class histogram per 32-row window, loop-seam detail.
"""
import os, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from songs import SONGS, ORDER, HOLD, REST

PC = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
CONSONANT = {0, 3, 4, 5, 7, 8, 9}          # interval classes mod 12
DRUM = {0: 'K', 1: 'S', 2: 'h', 3: 'O', 4: 'R', HOLD: '.', REST: 'x'}

# Form maps: (start_row, label). Kept here so songs.py stays engine-schema only.
# Songs whose ch1 is a time-delayed ECHO of ch0 (surf slapback reverb), not an
# independent harmony voice. The two squares are one voice offset in time, so
# voice-crossing and square-vs-square dissonance checks don't apply: a descending
# lead will always cross its own lagging echo -- that crossing IS the effect.
ECHO_SONGS = {'TADPOLE'}

FORMS = {
    'PRELUDE': [(0, 'A  arps C-C6-Am-Am (C maj)'),
                (64, "A' F-F-G-G: melody + countermelody DUET enters"),
                (128, 'B  lift bVI-bVII (Ab-Bb), soaring melody, weaving counter'),
                (192, 'C  home stretch F-Dm-G-G7'),
                (256, 'TAG C arrival, sus C5->B4 leading-tone cadence -> loop')],
    'EXPLORE': [(0, 'A  Am dread, sparse melody + dyads'),
                (64, "A' counter-melody pass; converge on B/G2 (V of C) @120"),
                (128, 'B  C-major hope, ext: motif in major @152; G7 -> iv-V'),
                (176, 'A  1st phrase; drums thin, bass C#3 -> D chromatic'),
                (208, 'C  Dm "ship alive": wave tenor sings motif inv+aug,'
                      ' ghost dyads, kicks @208/@240; V of Dm @264'),
                (272, 'A  2nd phrase; hats creep in, snare fill @300'),
                (304, 'D  E-Phrygian stabs, cell seq E-F-G-A peaks @336'
                      ' on A-C-B-A, dead air @344'),
                (352, 'E  engine room: bass+drums only on E pedal'),
                (376, "A'' finale 8va + snare; seam sinks home, E7 -> loop")],
    'BATTLE':  [(0, 'INTRO four-on-floor + octave-mantra bass (A min/Dorian)'),
                (32, 'A robot hook x2 + off-beat vocoder stabs, full groove'),
                (96, "A' hook brighter, register up, major splashes"),
                (160, 'BREAK/THE DROP: strip to bass+kick, filter shut'),
                (192, 'BUILD riser -> snare-roll explosion'),
                (224, "A'' hook full; turnaround fill -> loop")],
    'BOSS':    [(0, 'CHUG RIFF A: E drop-tuned gallop, 7/8+5/8 stop-hits -> void'),
                (56, "CHUG RIFF A': gallop varied, octave rip, second void"),
                (112, 'MANIC VERSE: phrygian-dominant tremolo fury (ch0 solo)'),
                (176, "CLEAN BRIDGE: whiplash -- flayer's croon, sword tacet"),
                (240, 'HALF-TIME BREAKDOWN: octave slam + G# grind, crush'),
                (288, 'CHAOTIC CLIMAX: lift E->G (m3), tremolo, rolls; home to E'),
                (336, 'TURNAROUND: snare roll -> crash, unison slam, void -> loop')],
    'VICTORY': [(0, 'fanfare; ch1 countervoice F4->E4 sus resolve, G-A-B-C climb')],
    'CRASH':   [(0, 'chromatic plunge; ch1 shadows m6 below from row 8, resolves A2->D2')],
    'TADPOLE': [(0, 'A: E-hijaz Misirlou riff x8, E5 tremolo cliff @112'),
                (128, "A': riff, ramp, chromatic DIVE BOMB 1 @244 -> crash"),
                (256, 'B: Am-G-F-E surf vamp, echo->backbeat stabs, cliff @368'),
                (384, 'BREAK: kick gallop + 16-row snare roll'),
                (416, "A'': riff hotter, hijaz summit-turn sequence down"),
                (544, 'TURN: E5 cliff, 2-octave DIVE BOMB 2 @564 -> loop row 0')],
    'SELUNE':  [(0, 'INTRO: wave arpeggio + answer-hook, no drums'),
                (128, 'VERSE1: low conversational C-major, answer fills'),
                (256, 'PRECHORUS: rising line'),
                (320, 'CHORUS: melody opens an octave up'),
                (448, 'TURN: instrumental answer-hook feature'),
                (512, 'VERSE2 / (640) PRECHORUS / (704) CHORUS'),
                (832, 'FINAL CHORUS in D: whole-step key change, ride'),
                (960, 'OUTRO: intro material in D, add9 fade -> loop')],
    'BOSSA':   [(0, 'A: Cmaj7 head, ii-V-I, guide-tone comp, straight-8 anticip'),
                (128, "A': same changes, melody varied"),
                (256, 'B: THE JOBIM MOVE -- one note (E5) held while harmony walks'),
                (384, "A'': warmer lower close"),
                (512, 'C: lydian float, G#maj7 (bVI), A#7 (bVII7), seam -> bar 1')],
    'GAIA':    [(0, 'ORBIT: G/Eb/Bb/D drift, high 4ths, susp. cymbal swells'),
                (160, 'TURNS: 5/4 heartbeat ostinato on D pedal (mixolydian)'),
                (400, 'HYMN: G major arch tune, bare then harmonized in 6ths'),
                (656, 'SURGE: hymn 8va, timpani+snare, rolls at cadences'),
                (912, 'DISSOLVE: orbit returns, thins to silence -> loop')],
    'AZURE':   [(0, 'INTRO drums alone, bass enters bar 2 (swing, 3 rows/beat)'),
                (24, 'HEAD 32-bar AABA: bass call, quartal horn stabs; B up to Eb dorian'),
                (408, 'TRUMPET chorus: harmon-thin motifs over walking 4s; sax tacet'),
                (792, 'SAX chorus: enclosure lines, double-time flare in B; tpt tacet'),
                (1176, 'OUT: A with hits, then settling tag -> seam breathes to head @24')],
    'GATES':   [(0, 'DROP-IN: chopped break alone; bass raps the hook rhythm b2'),
                (32, 'VERSE1: rap-cadence hook in Dm, choir answers the gaps'),
                (160, 'WALL1: gospel dyads pump bVI-bVII-i (Bb-C-Dm) in P4s'),
                (224, 'VERSE2: hook stuttered, bass roll doubles'),
                (352, 'WALL2: climb widens Gm-C-Dm; octave-unison riser'),
                (416, 'GATE: full-band tresillo stutter-lock, noise chokes'),
                (448, 'SHRED: row-rate melodic-minor rip, Dm/A sweeps to E6'),
                (512, 'BASS BREAK: wave grabs the hook solo, chips answer'),
                (544, 'HALF-TIME DROP: anthem slams Dm-Bb-C-A, choir pumps'),
                (608, 'BUILD: pulse ladder + kick-anchored snare roll'),
                (640, 'OUT: hook over breathing pads, 8va lap, opens splash'),
                (768, 'TURN: unison slam -> 8-row void, fall, roll-up -> loop 0')],
}

def nname(v):
    if v == HOLD: return '.'
    if v == REST: return 'x'
    return PC[v % 12] + str(v // 12 + 2)

def sounding(ch):
    """Per-row sounding pitch (None if silent), tracking HOLD/REST."""
    out, cur = [], None
    for v in ch:
        if v <= 59: cur = v
        elif v == REST: cur = None
        out.append(cur)
    return out

def check_song(name, s):
    errs, warns = [], []
    is_echo = name in ECHO_SONGS
    ch = s['ch']
    if len(ch) != 4:
        return ['%s: needs 4 channels, has %d' % (name, len(ch))], []
    rows = len(ch[0])
    for i, c in enumerate(ch):
        if len(c) != rows:
            errs.append('ch%d length %d != %d' % (i, len(c), rows))
    if errs: return errs, warns
    for i in (0, 1, 2):
        bad = [(r, v) for r, v in enumerate(ch[i]) if not (0 <= v <= 61)]
        if bad: errs.append('ch%d out-of-range values: %s' % (i, bad[:4]))
    bad = [(r, v) for r, v in enumerate(ch[2]) if v <= 59 and v > 47]
    if bad: errs.append('ch2 wave notes > 47 (clamp): %s' % [(r, nname(v)) for r, v in bad[:6]])
    bad = [(r, v) for r, v in enumerate(ch[3]) if v not in (0, 1, 2, 3, 4, HOLD, REST)]
    if bad: errs.append('ch3 non-drum values: %s' % bad[:6])
    loop = s['loop']
    if loop != 0xFFFF and not (0 <= loop < rows):
        errs.append('loop %d out of range (rows=%d)' % (loop, rows))
    if name in ('VICTORY', 'CRASH') and loop != 0xFFFF:
        errs.append('%s must stay play-once (loop=0xFFFF)' % name)

    s0, s1 = sounding(ch[0]), sounding(ch[1])

    # strong-beat both-attack consonance
    strong = dissonant = 0
    diss_rows = []
    for r in range(0, rows, 4):
        a, b = ch[0][r], ch[1][r]
        if a <= 59 and b <= 59:
            strong += 1
            if abs(a - b) % 12 not in CONSONANT:
                dissonant += 1
                diss_rows.append((r, nname(a), nname(b)))
    dpct = 100.0 * dissonant / strong if strong else 0.0
    if dpct > 5.0:
        (warns if is_echo else errs).append(
            'strong-beat dissonance %.1f%% > 5%% at %s%s' %
            (dpct, diss_rows[:8], ' (echo: waived)' if is_echo else ''))

    # weak-beat both-attack dissonances: should resolve by step in either voice
    weak_bad = []
    for r in range(rows):
        if r % 4 == 0: continue
        a, b = ch[0][r], ch[1][r]
        if a <= 59 and b <= 59 and abs(a - b) % 12 not in CONSONANT:
            nxt = next(((s0[q], s1[q]) for q in range(r + 1, rows)
                        if s0[q] is not None and s1[q] is not None
                        and (s0[q], s1[q]) != (a, b)), None)
            ok = nxt and (abs(nxt[0] - a) <= 2 or abs(nxt[1] - b) <= 2) \
                     and abs(nxt[0] - nxt[1]) % 12 in CONSONANT
            if not ok: weak_bad.append((r, nname(a), nname(b)))
    if weak_bad:
        warns.append('weak-beat dissonance w/o step resolution: %s' % weak_bad[:6])

    # voice crossing + spacing, over rows where both squares sound
    both = cross = wide = 0
    cross_rows = []
    for r in range(rows):
        if s0[r] is None or s1[r] is None: continue
        both += 1
        if s1[r] > s0[r]:
            cross += 1
            cross_rows.append((r, nname(s0[r]), nname(s1[r])))
        if s0[r] - s1[r] > 19: wide += 1
    cpct = 100.0 * cross / both if both else 0.0
    if cpct > 2.0:
        (warns if is_echo else errs).append(
            'voice crossing %.1f%% > 2%% at %s%s' %
            (cpct, cross_rows[:8], ' (echo: waived)' if is_echo else ''))
    if wide: warns.append('square spacing > a 12th on %d rows' % wide)

    # preferred ranges (informational)
    for i, lo, hi, what in ((0, 12, 55, 'square'), (1, 12, 55, 'square')):
        out = [(r, nname(v)) for r, v in enumerate(ch[i]) if v <= 59 and not (lo <= v <= hi)]
        if out: warns.append('ch%d outside sweet %s range 12..55: %s' % (i, what, out[:6]))
    out = [(r, nname(v)) for r, v in enumerate(ch[2]) if v <= 59 and v > 23]
    if out: warns.append('ch2 bass above B3 on %d rows (first %s)' % (len(out), out[:4]))

    # ---- report ----
    print('%-8s rows=%-3d loop=%s speed=%d  strong-diss %d/%d (%.1f%%)  cross %d/%d (%.1f%%)'
          % (name, rows, 'once' if loop == 0xFFFF else loop, s['speed'],
             dissonant, strong, dpct, cross, both, cpct))
    for r0, label in FORMS.get(name, []):
        print('    @%-3d %s' % (r0, label))
    for w0 in range(0, rows, 32):
        hist = [0] * 12
        for i in (0, 1, 2):
            for v in ch[i][w0:w0 + 32]:
                if v <= 59: hist[v % 12] += 1
        top = sorted(range(12), key=lambda p: -hist[p])[:4]
        print('    rows %3d-%3d pc: %s' % (w0, min(w0 + 31, rows - 1),
              ' '.join('%s:%d' % (PC[p], hist[p]) for p in top if hist[p])))
    if loop != 0xFFFF:
        def rowstr(r):
            return '%3d: %-4s %-4s %-4s %s' % (r, nname(ch[0][r]), nname(ch[1][r]),
                                               nname(ch[2][r]), DRUM.get(ch[3][r], '?'))
        print('    seam (last 4 rows -> loop rows %d..%d):' % (loop, loop + 3))
        for r in range(rows - 4, rows): print('      %s' % rowstr(r))
        print('      ---- loop ----')
        for r in range(loop, min(loop + 4, rows)): print('      %s' % rowstr(r))
        same = all(ch[i][rows - 4:] == ch[i][loop:loop + 4] for i in range(4))
        if same: warns.append('seam: last 4 rows identical to loop rows (dead seam)')
    for w in warns: print('    warn: %s' % w)
    return errs, warns

def main():
    failed = False
    for name in ORDER:
        errs, _ = check_song(name, SONGS[name])
        for e in errs:
            print('    FAIL: %s' % e)
            failed = True
    print('music lint: %s' % ('FAIL' if failed else 'OK'))
    return 1 if failed else 0

if __name__ == '__main__':
    sys.exit(main())
