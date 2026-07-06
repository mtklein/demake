# Original chiptunes in the FF4 idiom. Note data compiled to C by mkassets.
# Encoding per row: a note index (0..59, C2..B6), HOLD=60 (sustain), REST=61 (cut).
# Drum channel rows: 0 kick, 1 snare, 2 hat, 3 open-hat, 4 ride, HOLD/REST as above.

HOLD, REST = 60, 61
_S = {'C':0,'C#':1,'D':2,'D#':3,'E':4,'F':5,'F#':6,'G':7,'G#':8,'A':9,'A#':10,'B':11}

def n(name):
    """'C4' -> index; 'A#3' etc. octave 2..6."""
    if name in ('-', '.'): return HOLD
    if name in ('x', 'R'): return REST
    o = int(name[-1]); s = _S[name[:-1]]
    return (o - 2) * 12 + s

def seq(*tokens):
    """Flatten a token list of note names / HOLD / REST into a byte list."""
    out = []
    for t in tokens:
        if isinstance(t, int): out.append(t)
        else: out.append(n(t))
    return out

def rep(pattern, times):
    out = []
    for _ in range(times): out += pattern
    return out

def hold_to(note, total):
    """attack + (total-1) holds"""
    return [n(note)] + [HOLD] * (total - 1)

def pad(lst, length, fill=HOLD):
    return (lst + [fill] * length)[:length]

def bars(text):
    """Whitespace-separated tokens; '|' is a visual bar-line, '.' HOLD, 'x' REST."""
    return [n(t) for t in text.replace('|', ' ').split()]

# Each song: dict(ch=[ch0,ch1,ch2,ch3], loop, speed, env1, env2, wavevol)
# env: SOUNDxCNT_H value. duty bits 6-7, env-step 8-10, dir bit11, init-vol 12-15.
# 0xF080 = vol15 decay-off(step0) 50% duty (sustained). 0xC780 = softer.
SONGS = {}

def envelope(vol=12, step=0, up=0, duty=2):
    return (vol << 12) | (up << 11) | (step << 8) | (duty << 6)

# ------------------------------------------------------------ PRELUDE (title)
# Rising harp arpeggios, FF "Prelude" idiom — 256 rows, 16 bars of 16.
# Form: A (C C6 Am Am) | A' (F F G G, melody enters, ch1 echoes an octave down)
#     | B lift to bVI-bVII (Ab Ab Bb Bb, soaring line) | C home F Dm G G7 -> loop.
# TOTAL_ROWS = 288
# FORM: 0=A(arps C-C6-Am-Am) | 64=A'(F-F-G-G, melody+countermelody DUET enters)
#       128=B(bVI-bVII lift Ab-Bb, soaring melody + weaving counter)
#       192=C(home stretch F-Dm-G-G7) | 256=TAG(C arrival, sus C5->B4 leading-tone cadence -> loop row 0)
# Grid: speed 9, 16 rows/bar, 2 rows/note. ch0 sq1(50%) melody; ch1 sq2(25%) countermelody
#       (always below ch0); ch2 wave arpeggio bed (sounds 8vb); ch3 noise SILENT.
def prelude():
    c0 = (
        bars('x . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . .') +
        bars('A5 . . . G5 . F5 . A5 . . . . . x . x . . . . . . . . . . . . . . . B5 . . . A5 . G5 . B5 . . . . . x . x . . . . . . . . . . . . . . .') +
        bars('C6 . . . . . . . . . . . . . . . A#5 . . . G#5 . . . D#5 . . . . . x . D6 . . . C6 . A#5 . F5 . . . . . . . x . . . . . . . . . . . . . . .') +
        bars('C6 . . . A5 . G5 . F5 . . . . . x . F5 . G5 . A5 . . . D5 . . . . . x . B5 . . . D6 . . . G5 . . . . . . . G5 . F5 . D5 . B4 . D5 . . . . . . .') +
        bars('C6 . . . . . . . G5 . . . E5 . . . E5 . . . . . . . D5 . . . . . . .')
    )
    c1 = (
        bars('x . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . .') +
        bars('F4 . . . G4 . A4 . C5 . . . . . x . A4 . C5 . A4 . F4 . G4 . . . . . x . D4 . . . E4 . F#4 . G4 . . . . . x . G4 . F#4 . E4 . D4 . B3 . . . . . x .') +
        bars('D#4 . F4 . G4 . G#4 . C5 . . . . . . . D#5 . . . C5 . . . G4 . . . . . x . F4 . . . E4 . . . A#3 . . . . . x . F5 . D5 . A#4 . F4 . A#4 . . . . . x .') +
        bars('A4 . . . C5 . . . D5 . C5 . . . x . D4 . F4 . A4 . . . F4 . . . . . x . D4 . G4 . B4 . . . B4 . . . . . . . B3 . A3 . G3 . F3 . G3 . . . . . . .') +
        bars('E4 . F4 . G4 . . . G4 . . . C5 . . . C5 . . . B4 . . . G3 . . . . . . .')
    )
    c2 = (
        bars('C3 . E3 . G3 . C4 . E4 . G4 . C5 . E4 . C3 . E3 . G3 . C4 . E4 . G4 . A4 . E4 . A2 . C3 . E3 . A3 . C4 . E4 . A4 . C4 . A2 . C3 . E3 . A3 . C4 . E4 . A4 . C4 .') +
        bars('F2 . A2 . C3 . F3 . A3 . C4 . F4 . A3 . F2 . A2 . C3 . F3 . A3 . C4 . F4 . A3 . G2 . B2 . D3 . G3 . B3 . D4 . G4 . D4 . G2 . B2 . D3 . G3 . B3 . D4 . G4 . B4 .') +
        bars('G#2 . C3 . D#3 . G#3 . C4 . D#4 . G#4 . D#4 . G#2 . C3 . D#3 . G#3 . C4 . D#4 . C5 . G#4 . A#2 . D3 . F3 . A#3 . D4 . F4 . A#4 . F4 . A#2 . D3 . F3 . A#3 . D4 . F4 . D5 . A#4 .') +
        bars('F2 . A2 . C3 . F3 . A3 . C4 . F4 . A4 . D3 . F3 . A3 . D4 . F4 . A4 . F4 . D4 . G2 . B2 . D3 . G3 . B3 . D4 . G4 . B4 . G2 . B2 . D3 . F3 . G3 . B3 . D4 . F4 .') +
        bars('C3 . E3 . G3 . C4 . E4 . G4 . C5 . E4 . G2 . B2 . D3 . F3 . G3 . B3 . D4 . F4 .')
    )
    c3 = (
        [REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST] +
        [REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST] +
        [REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST] +
        [REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST] +
        [REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST,REST]
    )
    return dict(ch=[c0, c1, c2, c3], loop=0, speed=9,
                env1=envelope(11, 2, 0, 2), env2=envelope(8, 2, 0, 1), wavevol=0x2000)

SONGS['PRELUDE'] = prelude()

# ------------------------------------------------------------ EXPLORE (dungeon)
# The dread-heart of the dying nautiloid. 440 rows, rondo: A A' B A C A D E A''.
# Home is the original A-minor motif (A-C-B-A turn, F-E-D-C descent, G# pull);
# every excursion develops that DNA, and every seam is a different door back.
#   A   (0-63):    Am dread, sparse melody over dyads; Am Am F Am Dm C E Am.
#   A'  (64-127):  counter-melody pass; last bar converges on B over G (V of C).
#   B   (128-175): C-major fragile hope, extended: the motif recolored MAJOR
#                  (C-E-D-C @152) crests at E5, then G7 collapses into iv-V.
#   A   (176-207): first phrase only; drums thin out, bass slides C#3 -> D.
#   C   (208-271): "the ship is alive" -- D minor. Texture inverts: the wave
#                  bass sings the motif INVERTED + AUGMENTED (D-Bb-C-D, then
#                  the descent cell F-E-D-C-Bb augmented) in its tenor range
#                  while the squares breathe staggered ghost dyads above.
#                  Kick every 32 rows only. V-of-Dm (A, C#) pulls back to A.
#   A   (272-303): second phrase of the tune; hats creep in, snare fill -> D.
#   D   (304-351): "something approaches" -- E Phrygian. Off-beat square stabs
#                  in parallel 4ths SEQUENCE the motif cell up E-F-G-A over a
#                  rising octave-pulse bass and 8th-note hats; at the peak the
#                  cell lands on the ORIGINAL A-C-B-A, on the beat, fortissimo
#                  -- then everything cuts dead for 8 rows.
#   E   (352-375): engine room: squares silent, bass + drums alone on an E
#                  pedal (dominant); heartbeat kick returns, G#-B walk -> A''.
#   A'' (376-439): finale: tune + counter-melody an octave up, snare enters;
#                  seam bar sinks back to the old register, E7 pickup -> row 0.
def explore():
    # ---- home material (unchanged DNA) ----
    melA = bars('''A3 . . .  C4 . B3 .  | A3 . . .  x  x E4 .
                 | F4 . . .  E4 . D4 .  | C4 . . .  x  x x  x
                 | A3 . . .  E4 . F4 .  | E4 . D4 . C4 . B3 .
                 | A3 . . .  G#3 . . .  | A3 . . .  x  x x  x''')
    harmA = bars('''E3 . . .  .  . .  .  | C3 . . .  x  x x  x
                  | A3 . . .  .  . .  .  | E3 . . .  x  x x  x
                  | F3 . . .  A3 . .  .  | G3 . . .  .  . .  .
                  | E3 . . .  .  . .  .  | C3 . . .  x  x x  x''')
    bassA = bars('''A2 . . .  E2 . .  .  | A2 . . .  E2 . G2  .
                  | F2 . . .  C3 . .  .  | A2 . . .  E2 . C2  .
                  | D2 . . .  A2 . .  .  | C2 . . .  G2 . .   .
                  | E2 . . .  B2 . .  .  | A2 . . .  E2 . G#2 .''')
    cntr  = bars('''C3 . . .  E3 . D3 .  | C3 . . .  G3 . A3 B3
                  | A3 . . .  G3 . F3 .  | E3 . . .  G3 E3 D3 .
                  | F3 . . .  C4 . D4 .  | C4 . B3 . A3 .  G3 .
                  | C3 . D3 . E3 . .  .  | E3 . . .  G3 .  B3 .''')
    # A' seam: both voices converge on B3 over G2 (V of C) -> door into B.
    melAp  = melA[:56]  + bars('A3 . . . G3 . B3 .')
    bassAp = bassA[:56] + bars('A2 . . . E2 . G2 .')
    # ---- B: fragile hope, extended.  Bars 4-5 quote the A motif in MAJOR
    # (C-E-D-C) at the top of the range, answer with the descent cell; the
    # G7 under it refuses to resolve home and folds into Dm-E (iv-V) instead.
    melB  = bars('''E4 . G4 . C5 . .  .  | B4 . G4 . D5 . .   .
                  | A4 . G4 . F4 . E4 .  | C5 . . . E5 . D5   .
                  | C5 . B4 . A4 . G4 .  | F4 . D4 . B3 . G#3 .''')
    harmB = bars('''C4 . E4 . G4 . .  .  | G4 . D4 . B4 . .   .
                  | F4 . D4 . C4 . .  .  | F4 . . . A4 . .    .
                  | E4 . . . F4 . .   .  | D4 . A3 . E3 . .   .''')
    bassB = bars('''C2 . . .  G2 . .  .  | G2 . . .  B2 . .   .
                  | F2 . . .  C3 . .  .  | F2 . . .  A2 . .   .
                  | G2 . . .  G2 . F2 .  | D2 . . .  E2 . B2  .''')
    # A return #1: bass bar 4 slides chromatically C#3 -> D3 (into C section).
    bassR1 = bassA[:24] + bars('A2 . . . E2 . C#3 .')
    # ---- C: the ship is alive.  Wave bass = A motif inverted (1-3-2-1 flips
    # to 1-b6-b7-1: D-Bb-C-D) and augmented x2 (rhythm 8/4/4/8 = 4/2/2/4 of
    # bar 1 doubled); phrase 2 = the F-E-D-C-Bb descent cell augmented, then
    # V of Dm (A2, C#2 half-step under D2) hands back to A.  Squares:
    # staggered ghost dyads that enter two rows apart and exhale into rests.
    def ghost(hi, lo, sound=12, total=16):
        top = [n(hi)] + [HOLD] * (sound - 1) + [REST] * (total - sound)
        bot = [REST, REST, n(lo)] + [HOLD] * (sound - 3) + [REST] * (total - sound)
        return top, bot
    g1t, g1b = ghost('A4',  'F4')
    g2t, g2b = ghost('G4',  'D4')
    g3t, g3b = ghost('C5',  'F4')
    g4t, g4b = ghost('A#4', 'F4', sound=8)
    ch0C, ch1C = g1t + g2t + g3t + g4t, g1b + g2b + g3b + g4b
    bassC = bars('''D3 . . . D3  . .  . | A#2 . . . C3  . .   .
                  | D3 . . . D3  . .  . | x   x A2 . .  . .   .
                  | F3 . . . .   . .  . | E3  . . . D3  . .   .
                  | C3 . . . A#2 . .  . | A2  . . . A2  . C#2 .''')
    # ---- D: something approaches.  The motif cell (1-3-2) as off-beat stabs
    # in parallel 4ths, sequenced up E-F-G-A with chromatic color; the peak
    # bar is the original A-C-B-A hammered ON the beat, then 8 rows of void.
    melD  = bars('''x  x E4 . x  G4  . F4  | x  x F4 . x G#4 . G4
                  | x  x G4 . x  A#4 . A4  | x  x A4 . x C5  . B4
                  | A4 . C5 . B4 .   A4 .  | x  x x  x x x   x x''')
    harmD = bars('''x  x B3 . x  D4  . C4  | x  x C4 . x D#4 . D4
                  | x  x D4 . x  F4  . E4  | x  x E4 . x G4  . F#4
                  | E4 . G4 . F#4 .  E4 .  | x  x x  x x x   x x''')
    bassD = bars('''E2 . E3 . E2 . E3 .    | F2 . F3 . F2 . F3 .
                  | G2 . G3 . G2 . G3 .    | A2 . A3 . A2 . A3 .
                  | A2 . A2 . A2 . A2 .    | x  x x  x x  x x  x''')
    # ---- E: engine room -- bass and drums only, dominant pedal -> finale.
    bassE = bars('''E2 . . E2 x  E2 E3  .  | E2 . . E2 x E2 D3 .
                  | E2 . . .  E2 .  G#2 B2''')
    # ---- A'': finale reprise an octave up; seam sinks home for the loop.
    def up(ch):
        return [v + 12 if v <= 59 else v for v in ch]
    melF  = up(melA[:56]) + bars('A4 . . . x  x G#3 B3')   # E7 pickup -> A3
    harmF = up(cntr[:56]) + bars('C4 . . . E3 . .   .')
    bassF = bassA[:56]    + bars('A2 . . . E2 . G#2 .')
    # ---- assembly:  A    A'     B      A(1)        C     A(2)        D     E          A'' ----
    mel  = melA + melAp + melB + melA[:32]  + ch0C + melA[32:]  + melD + [REST] * 24 + melF
    harm = harmA + cntr + harmB + harmA[:32] + ch1C + harmA[32:] + harmD + [REST] * 24 + harmF
    bass = bassA + bassAp + bassB + bassR1  + bassC + bassA[32:] + bassD + bassE      + bassF
    R = REST
    heart  = [0,R,R,R, 2,R,R,R, 0,R,R,R, R,R,R,R]  # heartbeat kick, distant hat
    heart2 = [0,R,R,R, 2,R,R,R, 0,R,R,R, 2,R,R,R]  # + answering hat
    pulse8 = [0,R,R,R, 2,R,R,R]
    drive  = [0,R,2,R, 2,R,1,2]                    # D: 8th hats, off-beat snare
    full16 = [0,R,R,R, 2,R,R,R, 0,R,2,R, 1,R,2,R]  # finale: snare enters
    drum = ( rep(heart, 4)                                        # A
           + rep(heart, 2) + rep(heart2, 2)                       # A'
           + rep(pulse8, 5) + [0,R,R,R, 2,R,1,1]                  # B + fill
           + heart + [0,R,R,R, R,R,R,R, 0,R,R,R, R,R,R,R]         # A(1): hat dies
           + [0] + [R]*31 + [0] + [R]*27 + [2,R,2,2]              # C: two kicks
           + heart2 + [0,R,2,R, 2,R,2,R] + [0,R,2,R, 1,1,1,1]     # A(2) + fill
           + rep(drive, 4) + [0,2,2,2, 1,2,1,2] + [R]*8           # D, peak, void
           + pulse8 + [0,R,R,R, 2,R,0,R] + [0,R,2,R, 1,1,1,3]     # E + crash
           + rep(full16, 3) + pulse8 + [0,R,R,R, 2,R,1,1] )       # A'' -> loop
    return dict(ch=[mel, harm, bass, drum], loop=0, speed=10,
                env1=envelope(11, 0, 0, 2), env2=envelope(8, 0, 0, 1),
                wavevol=0x2000)

SONGS['EXPLORE'] = explore()

# ------------------------------------------------------------ BATTLE
# Fast driving D-minor, 128 rows.
#   A (0-63):    original drive (harm bar 6 raised an octave to stay in a 12th).
#   B (64-119):  VI-VII-i riff sprint Bb-C-Dm, A(V), then Gm-A and a scale run.
#   T (120-127): drum-fill turnaround on A7, chromatic bass C#2 -> D2 at loop.
# TOTAL_ROWS = 256
# FORM: 0=INTRO(bass octave-mantra + 4-on-floor establish; kit opens bar 2)
#       32=A(robot hook x2 + vocoder off-beat stabs, full disco groove)
#       96=A'(hook varied/brighter, register up, major-splash turns)
#       160=BREAK/THE DROP(strip to bass+kick, "filter shut": closed hats only)
#       192=BUILD(riser: lead pulses climb, ride shimmer opens, snare roll explodes)
def battle():
    c0 = (
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x A4 x A4 C5 x A4 E5 x D5 x C5 x A4 x G4 x A4 x C5 x E5 x D5 C5 A4 x G4 A4 x E4 x x') +
        bars('A4 x A4 C5 x A4 E5 x D5 x C5 x A4 x G4 x A4 x C5 x E5 x D5 C5 A4 x G4 A4 x E4 x x A4 x E5 x A5 x G5 E5 D5 x C5 D5 x E5 x x A4 x C5 E5 x D5 x C5 A4 x A4 G4 x A4 x x') +
        bars('A4 x E5 x A5 x G5 E5 D5 x C5 D5 x E5 x x A4 x C5 E5 x D5 x C5 A4 x A4 G4 x A4 x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +
        bars('A4 x A4 x A4 x A4 x A4 x A4 x A4 A4 A4 A4 C5 x C5 x E5 x E5 x A5 x A5 A5 A5 A5 A5 A5 A4 x A4 C5 x A4 E5 x D5 x C5 x A4 x G4 x A4 x C5 x E5 x D5 C5 A4 x G4 x E4 x x x')
    )
    c1 = (
        bars('x x x x x x x x x x x x x x x x x E4 x C4 x E4 x x x C4 x E4 x C4 x x x E4 x x x E4 x C4 x E4 x x x E4 x D4 x E4 x C4 x E4 x x x C4 x E4 x C4 x x') +
        bars('x E4 x x x E4 x C4 x E4 x x x E4 x D4 x E4 x C4 x E4 x x x C4 x E4 x C4 x x x A4 x x x G4 x E4 x A4 x x x G4 x E4 x G4 x E4 x A4 x x x E4 x C4 x E4 x x') +
        bars('x A4 x x x G4 x E4 x A4 x x x G4 x E4 x G4 x E4 x A4 x x x E4 x C4 x E4 x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +
        bars('x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x E4 x x x E4 x C4 x E4 x x x E4 x D4 x E4 x C4 x E4 x x x C4 x E4 x E4 x x')
    )
    c2 = (
        bars('A2 A3 x A2 A3 x A2 A3 x A2 A3 x A3 x A2 A3 G2 G3 x G2 G3 x G2 G3 x A2 A3 x A3 x A2 A3 A2 A3 x A2 A3 x A2 A3 x A2 A3 x A3 x A2 A3 G2 G3 x G2 G3 x G2 G3 x A2 A3 x A3 x A2 A3') +
        bars('A2 A3 x A2 A3 x A2 A3 x A2 A3 x A3 x A2 A3 G2 G3 x G2 G3 x G2 G3 x A2 A3 x A3 x A2 A3 A2 A3 x A2 A3 x A2 A3 x A2 A3 x A3 x A2 A3 G2 G3 x G2 G3 x G2 G3 x A2 A3 x A3 x A2 A3') +
        bars('A2 A3 x A2 A3 x A2 A3 x A2 A3 x A3 x A2 A3 G2 G3 x G2 G3 x G2 G3 x A2 A3 x A3 x A2 A3 A2 x A3 x A2 x A3 x A2 x A3 x A2 x A3 x A2 x A3 x A2 x A3 x A2 x A3 x A2 x A3 x') +
        bars('A2 A3 A2 A3 A2 A3 A2 A3 A2 A3 A2 A3 A2 A3 A2 A3 C3 C4 C3 C4 E3 E4 E3 E4 A3 A4 A3 A4 A3 A4 A3 A4 A2 A3 x A2 A3 x A2 A3 x A2 A3 x A3 x A2 A3 G2 G3 x G2 G3 x G2 G3 x A2 A3 x A3 x A2 A3')
    )
    c3 = (
        [0,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2] +
        [0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2] +
        [0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2,0,2,2,2] +
        [0,4,3,4,0,4,3,4,0,4,3,4,0,4,3,4,0,2,1,2,0,1,2,1,0,1,1,1,1,1,1,1,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,0,2,3,2,1,1,1,1]
    )
    return dict(ch=[c0, c1, c2, c3], loop=0, speed=7,
                env1=envelope(12, 0, 0, 1), env2=envelope(11, 2, 0, 2), wavevol=0x2000)

SONGS['BATTLE'] = battle()

# ------------------------------------------------------------ BOSS (Zhalk)
# E-minor menace, 128 rows.
#   A (0-63):    original riff (bars 2-4 revoiced: consonant, within a 12th).
#   B (64-95):   semitone lift to Fm then Ab; ch0 (sword) and ch1 (tentacle)
#                trade two-bar phrases; kick-run "tom" fill into the climb.
#   C (96-111):  chromatic bass grind Bb->F, squares trade rising stabs.
#   D (112-127): B7 peak in octaves, chromatic crash G-F#-F-E; V seam -> loop.
# TOTAL_ROWS = 352
# FORM (speed 5, 4 rows/beat; ch0=Zhalk sword 75% duty, ch1=mind-flayer 50% hollow,
#       ch2=drop-tuned bass wave sounds 8vb -- pitches written are what you HEAR,
#       ch3=noise 0kick 1snare 2hat 3crash 4ride; E tonic, E phrygian-dominant E F G# A B C D):
#   @0   CHUG RIFF A   -- bars: 16, 16, 14 (7/8), 10 (5/8).  Odd pair sums to 24 so row 56 realigns.
#                         Each odd bar ends on a unison STOP-HIT into an 'x' VOID.
def boss():
    c0 = (
        bars('E3 E3 E3 G#3 E3 E3 E3 F3 E3 E3 G#3 E3 E3 F3 E3 E3 E3 E3 E3 E3 G#3 E3 A3 E3 C4 E3 D4 E3 B3 G#3 F3 E3 E3 E3 E3 G#3 E3 E3 G#4 x x x E3 E3 G#3 F3 E3 E3 G#3 F3 E3 x x x x x E3 E3 G#3 E3 E3 F3 E3 E3') +
        bars('E3 G#3 E3 E3 F3 E3 G#3 E3 E3 E3 E3 G#3 A3 B3 C4 D4 E4 E3 D4 C4 B3 A3 G#3 F3 E3 E3 E3 F3 E3 E3 G#4 x x x E3 G#3 B3 G#3 E3 E3 G#3 F3 E4 x x x x x E4 F4 G#4 A4 B4 C5 D5 C5 B4 A4 G#4 A4 B4 C5 B4 A4') +
        bars('G#4 G#4 A4 A4 B4 B4 A4 A4 G#4 G#4 F4 F4 G#4 A4 B4 C5 D5 E5 D5 C5 B4 A4 G#4 F4 E4 F4 G#4 A4 B4 C5 D5 E5 E5 E5 D5 D5 C5 C5 B4 B4 A4 G#4 F4 E4 D4 C4 B3 G#3 x x x x x x x x x x x x x x x x') +
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x E3 . . . x x x x E3 . . . G#3 . F3 E3') +
        bars('E3 . . . x x x x G#3 . . . . . F3 E3 E3 . . . x x E3 E3 G#3 . A3 B3 C4 B3 A3 G#3 G4 G4 A#4 A#4 C5 C5 D5 D5 A#4 A#4 C5 D5 D#5 D5 C5 A#4 D#5 D5 C5 A#4 G4 A#4 C5 D5 D#5 D5 C5 A#4 G4 G4 A#4 C5') +
        bars('D5 C5 B4 A4 G#4 F4 E4 G#4 B4 E5 E5 E5 E5 D5 C5 B4 E4 x x x E4 x x x G#4 x B4 x E5 . x x')
    )
    c1 = (
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x B4 . . . C5 . B4 . G#4 . . . A4 . . .') +
        bars('G4 . F4 . E4 . . . F4 . G4 . B4 . . . C5 . . . B4 . A4 . G#4 . . . E4 . . . F4 . G4 . F4 . E4 . E4 . . . . . . . x x x x x x x x x x x x x x x x') +
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x')
    )
    c2 = (
        bars('E2 E2 E2 G#2 E2 E2 E2 F2 E2 E2 G#2 E2 E2 F2 E2 E2 E2 E2 E2 E2 G#2 E2 A2 E2 C3 E2 D3 E2 B2 G#2 F2 E2 E2 E2 E2 G#2 E2 E2 G#3 x x x E2 E2 G#2 F2 E2 E2 G#2 F2 E2 x x x x x E2 E2 G#2 E2 E2 F2 E2 E2') +
        bars('E2 G#2 E2 E2 F2 E2 G#2 E2 E2 E2 E2 G#2 A2 B2 C3 D3 E3 E2 D3 C3 B2 A2 G#2 F2 E2 E2 E2 F2 E2 E2 G#3 x x x E2 G#2 B2 G#2 E2 E2 G#2 F2 E3 x x x x x E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2') +
        bars('E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 . . . . . . . . . . . . . . .') +
        bars('C2 . . . . . . . . . . . . . . . A2 . . . . . . . . . . . . . . . B2 . . . . . . . B2 . . . . . . . E2 . . . x x x x E2 . . . G#2 . F2 E2') +
        bars('E2 . . . x x x x G#2 . . . . . F2 E2 E2 . . . x x E2 E2 G#2 . A2 B2 C3 B2 A2 G#2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2') +
        bars('E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 E2 x x x E2 x x x G#2 x B2 x E2 . x x')
    )
    c3 = (
        [3,0,2,0,1,0,0,2,0,0,2,0,1,0,0,2,0,0,2,0,1,0,0,2,0,2,0,2,1,0,0,0,3,0,2,0,1,0,3,REST,REST,REST,0,0,1,0,0,0,1,0,3,REST,REST,REST,REST,REST,3,0,2,0,1,0,0,2] +
        [0,0,2,0,1,0,2,0,0,0,2,0,1,0,0,0,0,0,0,0,1,0,0,0,3,0,2,0,1,0,3,REST,REST,REST,0,0,1,1,0,0,1,0,3,REST,REST,REST,REST,REST,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1] +
        [0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,3,4,REST,REST,REST,REST,REST,REST,REST,4,REST,REST,REST,REST,REST,REST,REST] +
        [4,REST,REST,REST,REST,REST,REST,REST,4,REST,REST,REST,REST,REST,REST,REST,4,REST,REST,REST,REST,REST,REST,REST,4,REST,REST,REST,REST,REST,REST,REST,4,REST,REST,REST,REST,REST,REST,REST,4,REST,REST,REST,REST,REST,REST,REST,3,REST,REST,REST,REST,REST,REST,REST,1,REST,REST,REST,REST,REST,REST,REST] +
        [3,REST,REST,REST,REST,REST,REST,REST,1,REST,REST,REST,REST,REST,REST,REST,3,REST,REST,REST,REST,REST,0,0,1,REST,0,0,1,0,1,0,0,1,0,1,1,1,1,1,0,1,0,1,1,1,1,3,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,3] +
        [0,1,0,1,1,1,1,1,0,1,1,1,1,1,1,3,1,1,1,1,1,1,1,3,0,REST,0,REST,3,REST,REST,REST]
    )
    return dict(ch=[c0, c1, c2, c3], loop=0, speed=5,
                env1=envelope(13, 0, 0, 3), env2=envelope(10, 0, 0, 2), wavevol=0x2000)

SONGS['BOSS'] = boss()

# ------------------------------------------------------------ VICTORY (fanfare)
# Triumphant major fanfare — plays once (loop past intro flourish).
def victory():
    lead = seq('G4','G4','G4','G4','C5',HOLD,HOLD,'G4',
               'C5',HOLD,'E5',HOLD,'G5',HOLD,HOLD,HOLD,
               'E5','C5','G4','C5','E5',HOLD,'D5',HOLD,
               'C5',HOLD,HOLD,HOLD,HOLD,HOLD,REST,REST)
    # countervoice: 4-3 suspension (F4->E4 over C), rising G-A-B-C against the
    # held G5, then the G7 seventh (F4) resolving into a spread final C-E-C.
    harm = seq('E4','E4','E4','E4','F4',HOLD,'E4',HOLD,
               'G4',HOLD,'A4',HOLD,'B4',HOLD,'C5',HOLD,
               'C5','G4','E4','G4','G4',HOLD,'F4',HOLD,
               'E4',HOLD,HOLD,HOLD,HOLD,HOLD,REST,REST)
    bass = seq('C3','C2','C3','C2','C3',HOLD,HOLD,'G2',
               'C3',HOLD,'C3',HOLD,'C3',HOLD,HOLD,HOLD,
               'C3','C3','C3','C3','G2',HOLD,'G2',HOLD,
               'C3',HOLD,'G2',HOLD,'C3',HOLD,REST,REST)
    drum = seq(1,1,1,1,0,REST,REST,1, 0,REST,0,REST,0,REST,REST,REST,
               0,0,0,0,0,REST,1,REST, 0,REST,0,REST,0,REST,REST,REST)
    return dict(ch=[lead, harm, bass, drum], loop=0xFFFF, speed=7,
                env1=envelope(13, 0, 0, 2), env2=envelope(10, 0, 0, 1),
                wavevol=0x2000)

SONGS['VICTORY'] = victory()

# ------------------------------------------------------------ CRASH (sting)
# Short dissonant plunge as the nautiloid falls — plays once.
def crash():
    lead = seq('D5','C#5','C5','B4','A#4','A4','G#4','G4',
               'F#4','F4','E4','D#4','D4','C#4','C4','B3',
               'A#3','A3','G#3','G3','F#3','F3','E3','D#3',
               'D3',HOLD,HOLD,HOLD,'A2',HOLD,'D2',HOLD)
    # ch1 shadows the plunge a minor 6th below from row 8, then resolves
    # G2 -> A2 (P4 under D3) and lands on the final low D with the lead.
    harm = [REST] * 8 + seq('A#3','A3','G#3','G3','F#3','F3','E3','D#3',
                            'D3','C#3','C3','B2','A#2','A2','G#2','G2',
                            'A2',HOLD,HOLD,HOLD,'D2',HOLD,HOLD,HOLD)
    bass = seq('D3',HOLD,'C3',HOLD,'A#2',HOLD,'G#2',HOLD,
               'F#2',HOLD,'E2',HOLD,'D2',HOLD,'C2',HOLD,
               'A#2',HOLD,'G#2',HOLD,'F#2',HOLD,'E2',HOLD,
               'D2',HOLD,HOLD,HOLD,'D2',HOLD,'D2',HOLD)
    drum = seq(0,REST,0,REST,0,REST,0,REST, 0,REST,0,REST,0,0,0,0,
               3,3,3,3,3,3,3,3, 0,0,0,0,0,0,0,0)
    return dict(ch=[lead, harm, bass, drum], loop=0xFFFF, speed=6,
                env1=envelope(13, 0, 0, 2), env2=envelope(9, 0, 0, 2),
                wavevol=0x2000)

SONGS['CRASH'] = crash()


# ------------------------------------------------------------ AZURE (jukebox)
# "Kind of Azure" -- modal jazz cut the Kind of Blue way: the leader sketched a
# 32-bar AABA (A = D dorian, B a half-step up in Eb dorian), notated only the
# quartal answer-stabs, and four session players tracked their parts without
# hearing each other. Swing grid: 3 rows/beat at speed 9 (~133 BPM); swung
# eighths land on beat-rows 0 and 2. Form: 2-bar drum/bass intro, head,
# trumpet chorus (sax tacet), sax chorus (tpt tacet), 16-bar out; loop -> 24.
def azure():
    tpt = (
        # INTRO
        bars('x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 1-2
        # HEAD
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 3-6
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 7-10
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 11-14
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 15-18
        bars('x x x x x x x x x x x x x x x x x x x x x D#4 . F4 x x x x x x x x x x x x x x x x x x x x x D#4 . F4') +  # bars 19-22
        bars('x x x x x x x x x x x x x x x x x x x x x D#4 . F4 x x x x x x x x x x x x x x x x x x x x x D#4 . F4') +  # bars 23-26
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 27-30
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 31-34
        # SOLO_TPT
        bars('x x x x D4 . F4 . . E4 . . . . . x x x x x x x x x x x x C4 . D4 . . . A3 . . . . . . . . x x x x x x') +  # bars 35-38
        bars('x x x x x x x x x x x x x x x C#4 D4 . . . . F4 . . . . . E4 . . . . . x x x x x x x x x x x x x x x') +  # bars 39-42
        bars('x x x x x x x x x x x x x x x G4 . B4 . . . A4 . . . . . . . . x x x x x x x x x x x x x x x x x x') +  # bars 43-46
        bars('x C5 . . . . A4 . . G4 . . . . . E4 . . . . . . . . . . . x x x x x x x x x x x x x x x x x x x x x') +  # bars 47-50
        bars('x x x x D#4 . F#4 . . F4 . . . . . x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 51-54
        bars('x x x x x x G#4 A#4 C5 D#5 . . . . . . . . . . . . . . . . . x x x x x x x x x x x x x x x x x x x x x') +  # bars 55-58
        bars('x x x E4 . C4 . . . D4 . . . . . . . . x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 59-62
        bars('x x x x F4 . D4 . . E4 . . . . . . . . . . . . . . . . . . . . x x x x x x x x x x x x x x x x x x') +  # bars 63-66
        # SOLO_SAX
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 67-70
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 71-74
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 75-78
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 79-82
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 83-86
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 87-90
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 91-94
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 95-98
        # OUT
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 99-102
        bars('x x x x x x x x x x x x x x x x x x x x x D4 . E4 x x x x x x x x x x x x x x x x x x x x x D4 . E4') +  # bars 103-106
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x E4 . . . . . . . . . . . . . . . . . . . . . .') +  # bars 107-110
        bars('. . . . . . . . . . . . . . . . . . x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x')    # bars 111-114
    )
    sax = (
        # INTRO
        bars('x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 1-2
        # HEAD
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 3-6
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 7-10
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 11-14
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 15-18
        bars('x x x x x x x x x x x x x x x x x x x x x A#3 . C4 x x x x x x x x x x x x x x x x x x x x x A#3 . C4') +  # bars 19-22
        bars('x x x x x x x x x x x x x x x x x x x x x A#3 . C4 x x x x x x x x x x x x x x x x x x x x x A#3 . C4') +  # bars 23-26
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 27-30
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 31-34
        # SOLO_TPT
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 35-38
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 39-42
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 43-46
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 47-50
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 51-54
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 55-58
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 59-62
        bars('x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x') +  # bars 63-66
        # SOLO_SAX
        bars('x x x A3 . C4 D4 . C4 A3 . G3 A3 . . . . . x x x x x x x x D4 C4 . A3 G3 G#3 A3 C4 . D4 E4 . D4 . . . x x x x x x') +  # bars 67-70
        bars('F3 . G3 A3 . C4 D4 . E4 D#4 . C#4 D4 . . . . . x x x x x x x x x C4 . D4 F4 . D4 C4 . A3 G3 . A3 . . . x x x x x x') +  # bars 71-74
        bars('x x D4 E4 . F4 G4 . A4 . . G4 F4 . E4 D4 . C4 A3 G#3 G3 A3 . C4 D4 . . . . x x x x G3 . A3 C4 . D4 E4 . F4 G4 G#4 A4 B4 . A4') +  # bars 75-78
        bars('G4 . F4 E4 . D4 C4 . A3 . . x x x x x x F3 G3 . A3 C4 . D4 E4 . F4 G4 . A4 A#4 . G#4 A4 . . x x x x x x x x x C4 . C#4') +  # bars 79-82
        bars('D#4 . . . . C#4 A#3 . G#3 F3 . G#3 A#3 . . . . . x x x x x C#4 D#4 F4 F#4 G#4 A#4 G#4 F#4 F4 D#4 C#4 C4 A#3 C4 C#4 D#4 C#4 C4 A#3 G#3 F3 F#3 G#3 A#3 C4') +  # bars 83-86
        bars('C#4 . . . . . . . x x x x x x x A#3 . G#3 F#3 . F3 D#3 . . . . . . . x x x x x x x x x x x x x x x x x x x') +  # bars 87-90
        bars('C#4 D4 . . . F4 D4 . C4 A3 . . . . x x x x x x x x x x x x C#4 D4 . . F4 E4 D4 C4 . A3 G3 . A3 . . . x x x x x x') +  # bars 91-94
        bars('x x x x x x D3 . F3 G3 . G#3 A3 . C4 D4 . C4 A3 . . G3 . . A3 . . . . . . . . . . . . . . . . . . . . . . .') +  # bars 95-98
        # OUT
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 99-102
        bars('x x x x x x x x x x x x x x x x x x x x x A3 . B3 x x x x x x x x x x x x x x x x x x x x x A3 . B3') +  # bars 103-106
        bars('x x x x x x x x x x x x x x x x x x x x x x x x D3 . . . . . . . . . . . . . . . . . . . . . . .') +  # bars 107-110
        bars('. . . . . . . . . . . . x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x')    # bars 111-114
    )
    bass = (
        # INTRO
        bars('x x x x x x x x x x x x D3 . . A2 . . B2 . . C#3 . .') +  # bars 1-2
        # HEAD
        bars('D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . E3 . . C3 . . A2 . .') +  # bars 3-6
        bars('D3 . . E3 . . F3 . . G3 . . F3 . . E3 . . C3 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . .') +  # bars 7-10
        bars('D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . E3 . . C3 . . A2 . .') +  # bars 11-14
        bars('D3 . . E3 . . F3 . . G3 . . F3 . . E3 . . D3 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . D3') +  # bars 15-18
        bars('D#3 . . A#2 . . C3 . . D3 . . D#3 . . C#3 . . C3 . . A#2 . . D#3 . . A#2 . . C3 . . D3 . . D#3 . . F3 . . C#3 . . A#2 . .') +  # bars 19-22
        bars('F#3 . . F3 . . D#3 . . D3 . . D#3 . . C#3 . . C3 . . A#2 . . D#3 . . A#2 . . C3 . . D3 . . D#3 . . C#3 . . C3 . . A#2 . C#3') +  # bars 23-26
        bars('D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . E3 . . C3 . . A2 . .') +  # bars 27-30
        bars('D3 . . E3 . . F3 . . G3 . . F3 . . E3 . . C3 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . C#3') +  # bars 31-34
        # SOLO_TPT
        bars('D3 . . C3 . . B2 . . A2 . . G2 . . F2 . . E2 . . G2 . . A2 . . B2 . . C3 . . C#3 . . D3 . . E3 . . F3 . . G3 . .') +  # bars 35-38
        bars('F3 . . E3 . . D3 . . C3 . . A2 . . G2 . . F2 . . E2 . . F2 . . G2 . . A2 . . B2 . . C3 . . D3 . . E3 . . C#3 . .') +  # bars 39-42
        bars('D3 . . E3 . . D3 . . C3 . . B2 . . A2 . . G2 . . F2 . . E2 . . F2 . . G2 . . G#2 . . A2 . . B2 . . C3 . . C#3 . .') +  # bars 43-46
        bars('D3 . . E3 . . F3 . . G3 . . F3 . . D3 . . E3 . . C3 . . B2 . . A2 . . G2 . . E2 . . A2 . . B2 . . C3 . . D3 . .') +  # bars 47-50
        bars('D#3 . . C#3 . . C3 . . A#2 . . G#2 . . F#2 . . F2 . . G#2 . . A#2 . . C3 . . C#3 . . D3 . . D#3 . . F3 . . F#3 . . D#3 . .') +  # bars 51-54
        bars('F#3 . . F3 . . D#3 . . C#3 . . C3 . . A#2 . . G#2 . . F#2 . . F2 . . F#2 . . G#2 . . A#2 . . C3 . . C#3 . . D#3 . . C#3 . .') +  # bars 55-58
        bars('D3 . . A2 . . B2 . . C3 . . D3 . . E3 . . F3 . . G3 . . F3 . . E3 . . D3 . . C3 . . B2 . . A2 . . G2 . . F2 . .') +  # bars 59-62
        bars('E2 . . F2 . . G2 . . A2 . . B2 . . C3 . . C#3 . . D3 . . E3 . . F3 . . E3 . . D3 . . C3 . . B2 . . A2 . . C#3 . .') +  # bars 63-66
        # SOLO_SAX
        bars('D3 . . C#3 . . D3 . . E3 . . F3 . . G3 . . F3 . . D3 . . C3 . . B2 . . A2 . . G2 . . E2 . . F2 . . G2 . . G#2 . .') +  # bars 67-70
        bars('A2 . . B2 . . C3 . . C#3 . . D3 . . C3 . . B2 . . A2 . . G2 . . A2 . . B2 . . C3 . . D3 . . E3 . . F3 . . E3 . .') +  # bars 71-74
        bars('F3 . . E3 . . D3 . . C3 . . B2 . . A2 . . G2 . . F2 . . E2 . . F2 . . G2 . . A2 . . B2 . . C3 . . C#3 . . D3 . .') +  # bars 75-78
        bars('F3 . . G3 . . F3 . . E3 . . D3 . . C3 . . B2 . . A2 . . G2 . . F2 . . E2 . . G2 . . A2 . . B2 . . C#3 . . D3 . .') +  # bars 79-82
        bars('D#3 . . F3 . . F#3 . . F3 . . D#3 . . C#3 . . C3 . . A#2 . . G#2 . . F#2 . . F2 . . F#2 . . G#2 . . A#2 . . C3 . . C#3 . .') +  # bars 83-86
        bars('D#3 . . F3 . . D#3 . . C#3 . . C3 . . A#2 . . G#2 . . F#2 . . F2 . . F#2 . . G#2 . . A#2 . . C3 . . A#2 . . C3 . . C#3 . .') +  # bars 87-90
        bars('D3 . . E3 . . F3 . . G3 . . F3 . . E3 . . D3 . . C3 . . B2 . . C3 . . D3 . . E3 . . F3 . . D3 . . B2 . . G2 . .') +  # bars 91-94
        bars('A2 . . G2 . . F2 . . E2 . . F2 . . G2 . . A2 . . B2 . . C3 . . C#3 . . D3 . . E3 . . F3 . . E3 . . D3 . . C#3 . .') +  # bars 95-98
        # OUT
        bars('D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . E3 . . C3 . . A2 . .') +  # bars 99-102
        bars('D3 . . E3 . . F3 . . G3 . . F3 . . E3 . . C3 . . A2 . . D3 . . A2 . . B2 . . C#3 . . D3 . . C3 . . B2 . . A2 . .') +  # bars 103-106
        bars('B2 . . A2 . . G2 . . F2 . . G2 . . F2 . . E2 . . F2 . . G2 . . A2 . . G2 . . F2 . . E2 . . F2 . . G2 . . G#2 . .') +  # bars 107-110
        bars('A2 . . . . . F2 . . . . . G2 . . . . . E2 . . . . . F2 . . . . . E2 . . . . . D2 . . . . . D2 . . . . .')    # bars 111-114
    )
    drums = (
        # INTRO
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,3,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4] +  # bars 1-2
        # HEAD
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 3-6
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,1,1,1,1,HOLD,1] +  # bars 7-10
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 11-14
        [4,HOLD,HOLD,2,4,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4,4,HOLD,HOLD,1,1,1,4,HOLD,HOLD,1,1,1] +  # bars 15-18
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,1,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 19-22
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,4,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,1,1,HOLD,1,1,1,1,1,1,1] +  # bars 23-26
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 27-30
        [4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4,1,1,HOLD,1,1,HOLD,1,1,HOLD,1,1,1] +  # bars 31-34
        # SOLO_TPT
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,0,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 35-38
        [4,HOLD,1,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,1,4,4,HOLD,HOLD,1,1,1,0,HOLD,1,1,HOLD,0] +  # bars 39-42
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,0,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 43-46
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,1,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,0,4,HOLD,HOLD,2,HOLD,4,1,HOLD,1,1,1,1] +  # bars 47-50
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4] +  # bars 51-54
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,0,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,1,2,HOLD,4,4,HOLD,1,1,1,HOLD,1,1,HOLD,1,1,1] +  # bars 55-58
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,1,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,0,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 59-62
        [4,HOLD,HOLD,2,HOLD,4,4,1,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,4,2,1,4,4,HOLD,HOLD,1,1,1,3,HOLD,REST,1,1,1] +  # bars 63-66
        # SOLO_SAX
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,0,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4] +  # bars 67-70
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,1,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,4,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,1,HOLD,1,1,1,1,0,HOLD,1] +  # bars 71-74
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,0,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,1,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 75-78
        [4,HOLD,HOLD,2,1,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,0,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,1,1,HOLD,1,1,1] +  # bars 79-82
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,1,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,0,2,HOLD,4] +  # bars 83-86
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,1,2,HOLD,4,4,1,HOLD,2,HOLD,4,1,1,1,HOLD,1,1,HOLD,1,1,1,1,1] +  # bars 87-90
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,0,2,HOLD,4,4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,HOLD,2,HOLD,4] +  # bars 91-94
        [4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,0,2,1,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,0,HOLD,1,1,1,HOLD,0,HOLD,1,1,1,1] +  # bars 95-98
        # OUT
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,1,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4] +  # bars 99-102
        [4,HOLD,HOLD,4,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,1,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,1,1,1,HOLD] +  # bars 103-106
        [3,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,4,4,HOLD,HOLD,2,HOLD,HOLD,4,HOLD,HOLD,2,HOLD,HOLD,4,HOLD,HOLD,2,HOLD,HOLD] +  # bars 107-110
        [4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD,4,HOLD,HOLD]    # bars 111-114
    )
    return dict(ch=[tpt, sax, bass, drums], loop=24, speed=9,
                env1=envelope(12, 6, 0, 1),      # harmon-thin 25% duty, slow bloom-fade
                env2=envelope(13, 7, 0, 2),      # tenor 50% duty, fuller
                wavevol=0x2000)

SONGS['AZURE'] = azure()


# ------------------------------------------------------------ GAIA (jukebox)
# TOTAL_ROWS = 1072
# FORM: 0 ORBIT / 160 TURNS / 400 HYMN / 656 SURGE / 912 DISSOLVE
# Meter: bars 1-10 ORBIT 4/4 (16 rows) | bars 11-22 TURNS 5/4 (20 rows) | bars 23-64 4/4 (16 rows)
# Key: G major (hymn); TURNS = D-mixolydian heartbeat (dominant pedal); ORBIT drifts G / Eb / Bb / D
def gaia():
    c0 = (
        bars('x . . . . . . . . . . . . . . . D6 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . D#6 . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . D6 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . E6 . . . . . . . D6 . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . A4 . . . D5 . . . . . . . E5 . . . D5 . . . . . . . . . . . . . . . . . . . x . . . A4 . . . D5 . . . E5 . . . F#5 . . .') +
        bars('E5 . . . . . . . . . . . . . . . x . . . . . . . D5 . . . E5 . . . F#5 . . . E5 . . . D5 . . . E5 . . . F#5 . . . G5 . . . F#5 . . . E5 . . .') +
        bars('F#5 . . . G5 . . . A5 . . . G5 . . . F#5 . . . A5 . . . . . . . . . . . . . . . . . . . A5 . . . B5 . . . A5 . . . G5 . . . F#5 . . . . . . .') +
        bars('. . . . . . . . . . . . x . . . B4 . . . A4 . . . G4 . . . A4 . . . B4 . . . C5 . . . D5 . . . . . . . E5 . . . D5 . . . C5 . . . B4 . . .') +
        bars('A4 . . . . . . . . . . . . . . . B4 . . . A4 . . . G4 . . . A4 . . . B4 . . . C5 . . . D5 . . . . . . . C5 . . . B4 . . . A4 . . . . . . .') +
        bars('G4 . . . . . . . . . . . . . . . A4 . . . B4 . . . C5 . . . . . . . B4 . . . C5 . . . D5 . . . . . . . C5 . . . D5 . . . E5 . . . . . . .') +
        bars('D5 . . . . . . . C5 . . . A4 . . . B4 . . . A4 . . . G4 . . . A4 . . . B4 . . . C5 . . . D5 . . . E5 . . . D5 . . . . . . . C5 . . . A4 . . .') +
        bars('G4 . . . . . . . . . . . . . . . B5 . . . A5 . . . G5 . . . A5 . . . B5 . . . C6 . . . D6 . . . . . . . E6 . . . D6 . . . C6 . . . B5 . . .') +
        bars('A5 . . . . . . . . . . . . . . . B5 . . . A5 . . . G5 . . . A5 . . . B5 . . . C6 . . . D6 . . . . . . . C6 . . . B5 . . . A5 . . . . . . .') +
        bars('G5 . . . . . . . . . . . . . . . A5 . . . B5 . . . C6 . . . . . . . B5 . . . C6 . . . D6 . . . . . . . C6 . . . D6 . . . E6 . . . . . . .') +
        bars('D6 . . . . . . . C6 . . . A5 . . . B5 . . . A5 . . . G5 . . . A5 . . . B5 . . . C6 . . . D6 . . . E6 . . . D6 . . . . . . . C6 . . . A5 . . .') +
        bars('G5 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . D6 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('D#6 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . D6 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('E6 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . .')
    )
    c1 = (
        bars('x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . G5 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('A#5 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . A5 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . D4 . . . G4 . . . . . . . A4 . . . G4 . . . x . . . . . . . . . . . . . . .') +
        bars('. . . . D4 . . . G4 . . . A4 . . . B4 . . . A4 . . . B4 . . . C5 . . . D5 . . . C5 . . . B4 . . . C5 . . . D5 . . . E5 . . . D5 . . . C5 . . .') +
        bars('D5 . . . E5 . . . F#5 . . . E5 . . . D5 . . . D5 . . . . . . . . . . . . . . . . . . . F#5 . . . G5 . . . F#5 . . . E5 . . . D5 . . . . . . .') +
        bars('. . . . . . . . . . . . x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . C4 . . . D4 . . . E4 . . . . . . . G4 . . . A4 . . . B4 . . . . . . . A4 . . . B4 . . . C5 . . . . . . .') +
        bars('A4 . . . . . . . . . . . F#4 . . . G4 . . . F#4 . . . E4 . . . F#4 . . . G4 . . . A4 . . . B4 . . . C5 . . . A4 . . . . . . . . . . . F#4 . . .') +
        bars('D4 . . . . . . . . . . . . . . . G5 . . . F#5 . . . E5 . . . F#5 . . . G5 . . . A5 . . . B5 . . . . . . . C6 . . . B5 . . . A5 . . . G5 . . .') +
        bars('F#5 . . . . . . . . . . . . . . . D5 . . . C5 . . . B4 . . . C5 . . . D5 . . . E5 . . . F#5 . . . . . . . E5 . . . D5 . . . C5 . . . . . . .') +
        bars('B4 . . . . . . . . . . . . . . . C5 . . . D5 . . . E5 . . . . . . . G5 . . . A5 . . . B5 . . . . . . . A5 . . . B5 . . . C6 . . . . . . .') +
        bars('A5 . . . . . . . . . . . F#5 . . . G5 . . . F#5 . . . E5 . . . F#5 . . . G5 . . . A5 . . . B5 . . . C6 . . . A5 . . . . . . . . . . . F#5 . . .') +
        bars('B4 . . . . . . . . . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . . . . . . . . . . G5 . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . A#5 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . G5 . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . .')
    )
    c2 = (
        bars('G2 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . G2 . . . . . . . . . . . . . . . D#2 . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . A#2 . . . . . . . . . . . . . . . D2 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('D2 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . D2 . D2 D2 D2 . . . D3 . . . D2 . D2 . D2 . . . D2 . D2 D2 D2 . . . D3 . . .') +
        bars('D2 . D2 . D2 . . . D2 . D2 D2 D2 . . . D3 . . . D2 . D2 . D2 . . . D2 . D2 D2 D2 . . . D3 . . . A2 . C3 . D2 . . . D2 . D2 D2 D2 . . . D3 . . . D2 . D2 .') +
        bars('D2 . . . D2 . D2 D2 D2 . . . D3 . . . A2 . C3 . D2 . . . D2 . D2 D2 D2 . . . D3 . . . D2 . D2 . D2 . . . D2 . D2 D2 D2 . . . D3 . . . A2 . C3 . D2 . . .') +
        bars('D2 . D2 D2 D2 . . . D3 . . . D2 . D2 . D2 . . . D2 . D2 D2 D2 . . . D3 . . . A2 . C3 . D2 . . . D2 . D2 D2 D2 . . . D3 . . . D2 . D2 . D2 . . . D2 . D2 D2') +
        bars('D2 . . . C3 . . . B2 . . . A2 . A2 . G2 . . . . . . . . . . . . . . . G2 . . . A2 . . . B2 . . . . . . . C3 . . . B2 . . . A2 . . . G2 . . .') +
        bars('D3 . . . . . . . D2 . . . . . . . G2 . . . . . . . . . . . . . . . G2 . . . A2 . . . B2 . . . . . . . C3 . . . . . . . D3 . . . . . . .') +
        bars('G2 . . . . . . . . . . . . . . . A2 . . . . . . . . . . . . . . . B2 . . . . . . . . . . . . . . . C3 . . . . . . . . . . . . . . .') +
        bars('D3 . . . . . . . D2 . . . . . . . G2 . . . . . . . . . . . . . . . G2 . . . A2 . . . B2 . . . C3 . . . D3 . . . . . . . D2 . . . . . . .') +
        bars('G2 . . . . . . . . . . . . . . . G2 . . . B2 . . . D3 . . . B2 . . . G2 . . . A2 . . . B2 . . . G2 . . . C3 . . . B2 . . . A2 . . . G2 . . .') +
        bars('D3 . . . A2 . . . D2 . . . . . . . G2 . . . B2 . . . D3 . . . B2 . . . G2 . . . A2 . . . B2 . . . G2 . . . C3 . . . . . . . D3 . . . D2 . . .') +
        bars('G2 . . . . . . . B2 . . . D3 . . . A2 . . . . . . . C3 . . . E3 . . . B2 . . . . . . . D3 . . . G2 . . . C3 . . . . . . . E3 . . . C3 . . .') +
        bars('D3 . . . . . . . D2 . . . . . . . G2 . . . B2 . . . D3 . . . B2 . . . G2 . . . A2 . . . B2 . . . C3 . . . D3 . . . . . . . D2 . . . . . . .') +
        bars('G2 . . . . . . . . . . . . . . . G2 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . G2 . . . . . . . . . . . . . . .') +
        bars('D#2 . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . A#2 . . . . . . . . . . . . . . . G2 . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . x . . . . . . . . . . . . . . .')
    )
    c3 = (
        [4,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,4,4,4,4] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,4,4,4,4,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,2,HOLD,2,2,0,HOLD,2,2,2,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD] +
        [2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,2,HOLD,2,HOLD] +
        [2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD] +
        [0,HOLD,2,2,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,2,2,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,1,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,2,2] +
        [2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,1,HOLD,2,2,2,2,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,2,HOLD,2,HOLD,4,4,4,4,3,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,2,HOLD,2,HOLD] +
        [0,HOLD,HOLD,HOLD,2,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,1,1,1,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,1,1] +
        [0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,1,1,1,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,1,1] +
        [0,HOLD,HOLD,HOLD,1,HOLD,1,HOLD,0,HOLD,HOLD,HOLD,1,1,1,1,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,1,1,1,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,1,1,1,1,1,1,1,1] +
        [3,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [4,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,REST,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD]
    )
    return dict(ch=[c0, c1, c2, c3], loop=0, speed=8,
                env1=envelope(12, 0, 0, 2), env2=envelope(10, 0, 0, 1), wavevol=0x2000)

SONGS['GAIA'] = gaia()


# ------------------------------------------------------------ TADPOLE (jukebox)
# TOTAL_ROWS = 592
# FORM: 0 A (E-hijaz riff x8, cliff-hold @112) | 128 A' (riff x6, ramp, DIVE @244)
#     | 256 B (Am-G-F-E vamp, echo->stabs, cliff-hold @368) | 384 BREAK (drums roll)
#     | 416 A'' (riff hotter, summit turns) | 544 TURN (E5 cliff, 2-octave DIVE @564 -> loop row 0)
def tadpole():
    c0 = (
        bars('E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 E4 E4 F4 E4 D#4 D#4 E4 D#4 E4 E4 E4 E4 E4 E4 E4 E4') +
        bars('E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 A4 A4 A4 A4 B4 B4 A4 B4 C5 C5 B4 C5 D#5 D#5 C5 D#5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5') +
        bars('E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 E4 E4 F4 E4 D#4 D#4 E4 D#4 E4 E4 E4 E4 E4 E4 E4 E4') +
        bars('E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 E4 E4 F4 F4 G#4 G#4 A4 A4 B4 B4 C5 C5 D#5 D#5 E5 E5 E5 E5 E5 E5 D#5 D5 C#5 C5 B4 A#4 A4 G#4 G4 F#4 F4 E4') +
        bars('E5 E5 E5 E5 C5 C5 C5 C5 A4 A4 A4 A4 B4 B4 C5 C5 B4 B4 B4 B4 A4 A4 B4 B4 C5 C5 B4 B4 A4 A4 A4 A4 D5 D5 D5 D5 B4 B4 B4 B4 G4 G4 G4 G4 A4 A4 B4 B4 A4 A4 A4 A4 G4 G4 A4 A4 B4 B4 A4 A4 G4 G4 G4 G4') +
        bars('C5 C5 C5 C5 A4 A4 A4 A4 F4 F4 F4 F4 G4 G4 A4 A4 G4 G4 G4 G4 F4 F4 G4 G4 A4 A4 G4 G4 F4 F4 F4 F4 E4 E4 E4 E4 G#4 G#4 G#4 G#4 B4 B4 B4 B4 D#5 D#5 D#5 D#5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5') +
        bars('x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4') +
        bars('E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 C5 C5 D#5 D#5 E5 E5 D#5 D#5 C5 C5 B4 B4 A4 G#4 E4 E4 F4 E4 D#4 D#4 E4 F4 G#4 A4 G#4 F4 E4 F4 G#4 A4 E5 E5 E5 E5 D#5 D#5 E5 D#5 C5 C5 D#5 C5 B4 B4 C5 B4') +
        bars('A4 A4 B4 A4 G#4 G#4 A4 G#4 F4 F4 G#4 F4 E4 E4 F4 E4 E4 E4 E4 E4 F4 F4 G#4 G#4 A4 A4 B4 B4 C5 C5 D#5 D#5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 D#5 D5 C#5 C5 B4 A#4 A4 G#4 G4 F#4 F4 E4') +
        bars('D#4 D4 C#4 C4 B3 A#3 A3 G#3 G3 F#3 F3 E3 E3 E3 E3 E3')
    )
    c1 = (
        bars('x x x E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 E4 E4 F4 E4 D#4 D#4 E4 D#4 E4 E4 E4 E4 E4') +
        bars('E4 E4 E4 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 A4 A4 A4 A4 B4 B4 A4 B4 C5 C5 B4 C5 D#5 D#5 C5 D#5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5') +
        bars('E5 E5 E5 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 E4 E4 F4 E4 D#4 D#4 E4 D#4 E4 E4 E4 E4 E4') +
        bars('E4 E4 E4 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4 G#4 F4 F4 E4 E4 F4 F4 G#4 G#4 A4 A4 B4 B4 C5 C5 D#5 D#5 E5 E5 E5 E5 E5 E5 D#5 D5 C#5 C5 B4 A#4 A4 G#4 G4') +
        bars('F#4 F4 E4 x A3 . x . . . . . E4 . x . . . . . C4 . x . . . . . E4 . x . . . . . D4 . x . . . . . D4 . x . . . . . B3 . x . . . . . D4 . x .') +
        bars('. . . . C4 . x . . . . . C4 . x . . . . . A3 . x . . . . . C4 . x . . . . . E4 . x . . . . . G#4 . x . . . . . G#4 . x . . . . . G#4 . x .') +
        bars('x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 B4 B4 C5 C5 B4 C5 B4 B4 A4 A4 G#4') +
        bars('G#4 F4 F4 E4 E4 E4 E4 F4 F4 E4 F4 G#4 G#4 F4 G#4 A4 A4 G#4 A4 B4 B4 C5 C5 D#5 D#5 E5 E5 D#5 D#5 C5 C5 B4 B4 A4 G#4 E4 E4 F4 E4 D#4 D#4 E4 F4 G#4 A4 G#4 F4 E4 F4 G#4 A4 E5 E5 E5 E5 D#5 D#5 E5 D#5 C5 C5 D#5 C5 B4') +
        bars('B4 C5 B4 A4 A4 B4 A4 G#4 G#4 A4 G#4 F4 F4 G#4 F4 E4 E4 F4 E4 E4 E4 E4 E4 F4 F4 G#4 G#4 A4 A4 B4 B4 C5 C5 D#5 D#5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 E5 D#5 D5 C#5 C5 B4 A#4 A4 G#4 G4') +
        bars('F#4 F4 E4 D#4 D4 C#4 C4 B3 A#3 A3 G#3 G3 F#3 F3 E3 E3')
    )
    c2 = (
        bars('E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . A2 . B2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . D#2 . E2 . F2 .') +
        bars('E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . A2 . B2 . E2 . E2 . B2 . E2 . A2 . A2 . B2 . B2 . E2 . F2 . G#2 . A2 . B2 . C3 . D#3 . E3 .') +
        bars('E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . A2 . B2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . D#2 . E2 . F2 .') +
        bars('E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . E2 . A2 . B2 . E2 . E2 . B2 . E2 . A2 . A2 . B2 . B2 . E3 . D#3 . C3 . B2 . A2 . G#2 . F2 . E2 .') +
        bars('A2 . A2 . E3 . A2 . A2 . A2 . E3 . A2 . A2 . A2 . E3 . A2 . A2 . A2 . B2 . A2 . G2 . G2 . D3 . G2 . G2 . G2 . D3 . G2 . G2 . G2 . D3 . G2 . G2 . G2 . A2 . G2 .') +
        bars('F2 . F2 . C3 . F2 . F2 . F2 . C3 . F2 . F2 . F2 . C3 . F2 . F2 . F2 . G2 . F2 . E2 . E2 . B2 . E2 . E2 . E2 . B2 . E2 . E2 . F2 . G#2 . A2 . B2 . C3 . D#3 . E3 .') +
        bars('x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 .') +
        bars('E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 . E2 . E2 . B2 . E2 . E2 . D#2 . E2 . F2 . E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 .') +
        bars('E2 . E2 E2 B2 . E2 . E2 . E2 E2 B2 . E2 . E2 . F2 . G#2 . A2 . B2 . C3 . D#3 . E3 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 . E2 .') +
        bars('E2 . F2 . G#2 . A2 . B2 . C3 . D#3 . E3 .')
    )
    c3 = (
        [3,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,1,1,1] +
        [0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,1,1,1,1,1,1,1,1] +
        [3,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,1,1,1] +
        [0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,1,1,1,0,HOLD,2,HOLD,1,1,1,1,1,1,1,1,1,1,1,1] +
        [3,HOLD,4,HOLD,1,HOLD,0,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,0,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,0,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,0,HOLD,0,HOLD,4,HOLD,1,1,1,1] +
        [0,HOLD,4,HOLD,1,HOLD,0,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,0,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,0,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,0,HOLD,1,1,1,1,1,1,1,1] +
        [0,HOLD,0,HOLD,1,HOLD,0,HOLD,0,HOLD,0,HOLD,1,HOLD,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,3,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD] +
        [0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,1,1,1,3,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD] +
        [0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,0,HOLD,0,HOLD,2,HOLD,1,1,1,1,0,HOLD,2,HOLD,1,HOLD,0,HOLD,1,1,1,1,1,1,1,1,0,HOLD,2,HOLD,1,1,1,1,1,1,1,1,0,HOLD,0,HOLD] +
        [1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0]
    )
    return dict(ch=[c0, c1, c2, c3], loop=0, speed=5,
                env1=envelope(14, 0, 0, 1), env2=envelope(9, 0, 0, 1), wavevol=0x2000)

SONGS['TADPOLE'] = tadpole()


# ------------------------------------------------------------ SELUNE (jukebox)
# ch0 is a wordless square-wave "voice"; a lyric fitted to this exact line
# (one syllable per attack) lives in docs/under_selune.md.
# TOTAL_ROWS = 1088
# FORM: 0 INTRO / 128 VERSE1 / 256 PRECHORUS / 320 CHORUS / 448 TURN / 512 VERSE2 / 640 PRECHORUS / 704 CHORUS / 832 FINAL_CHORUS_D / 960 OUTRO
def selune():
    c0 = (
        bars('x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . G3 .') +
        bars('C4 . D4 . E4 . . . . . D4 . C4 . C4 . . . B3 . . . x . . . . . . . A3 . C4 . D4 . E4 . . . G4 . . . D4 . F4 . . . E4 . . . x . . . . . . . A3 .') +
        bars('A3 . . . C4 . D4 . C4 . . . A3 . D4 . . . C4 . . . x . . . . . . . E4 . F4 . . . E4 . D4 . . . C4 . D4 . . . . . . . . . . . x . . . . . E4 .') +
        bars('F4 . G4 . A4 . . . . . . . G4 . G4 . . . . . . . A4 . B4 . . . . . . . . . A4 . . . . . G4 . A4 . B4 . C5 . . . . . . . . . D5 . . . . . . .') +
        bars('. . C5 . . . . . . . . . A4 . C5 . . . B4 . . . . . . . A4 . x . E5 . . . . . . . . . D5 . B4 . . . C5 . . . . . . . . . B4 . A4 . . . D5 .') +
        bars('. . . . . . C5 . . . A4 . . . . . . . G4 . . . . . A4 . B4 . . . C5 . . . . . . . . . . . D5 . E5 . . . . . . . D5 . . . C5 . . . . . x .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . G3 .') +
        bars('C4 . D4 . E4 . F4 E4 . . D4 . C4 . C4 . . . B3 . . . x . . . . . . . A3 . C4 . D4 . E4 . . . G4 . . . E4 D4 F4 . . . E4 . . . x . . . . . . . A3 .') +
        bars('A3 . . . C4 . D4 . C4 . A3 . C4 . D4 . . . C4 . . . x . . . . . . . E4 . F4 . . . E4 . D4 . E4 . C4 . D4 . . . . . . . . . . . x . . . . . E4 .') +
        bars('F4 . G4 . A4 . . . . . . . G4 . G4 . . . . . . . A4 . B4 . . . . . . . . . A4 . . . . . G4 . A4 . B4 . C5 . . . . . . . . . D5 . . . . . . .') +
        bars('. . C5 . . . . . . . . . A4 . C5 . . . B4 . . . . . . . A4 . x . E5 . . . . . . . . . D5 . B4 . . . C5 . . . . . . . . . B4 . A4 . . . D5 .') +
        bars('. . . . . . C5 . . . A4 . . . . . . . G4 . . . . . A4 . B4 . . . C5 . . . . . . . . . . . D5 . E5 . . . . . . . x . . . A4 . B4 . C#5 . D5 .') +
        bars('. . . . . . . . . . . . B4 . D5 . . . C#5 . . . . . . . B4 . x . F#5 . . . . . . . . . E5 . C#5 . . . D5 . . . . . . . . . C#5 . B4 . . . E5 .') +
        bars('. . . . . . D5 . . . B4 . . . . . . . A4 . . . . . B4 . C#5 . . . D5 . . . . . . . . . . . E5 . F#5 . . . . . . . E5 . . . D5 . . . . . . .') +
        bars('. . . . . . x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .')
    )
    c1 = (
        bars('x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . E4 . A4 . C5 . E5 . . . . . . . D5 . C5 . . . . . A4 . . . B4 . . . . . E5 .') +
        bars('. . D5 . C5 . . . . . A4 . . . G4 . . . . . . . A4 . B4 . . . . . . . C5 . . . . . A4 . G4 . . . . . . . F4 . . . E4 . D4 . G4 . . . . . x .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . D4 . E4 . G4 . x . . . . . . . . . . . . . . . . . . . . . . . . . E4 . D4 . C4 . x .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . C4 . D4 . E4 . x . . . . . . . . . . . . . . . . . . . . . . . . . D4 . E4 . F4 . x .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . D4 . . . . . . . . . . . . . . .') +
        bars('F4 . . . . . . . . . . . . . . . D4 . . . . . . . . . . . . . . . G4 . . . . . . . . . . . . . . . E4 . . . . . . . . . . . . . . .') +
        bars('F4 . . . . . . . . . . . . . . . D4 . . . . . . . . . . . . . . . E4 . . . . . . . . . . . . . . . G4 . . . . . . . . . . . . . G4 .') +
        bars('C5 . D5 . E5 . . . . . D5 . C5 . . . . . B4 . . . . . G4 . A4 . B4 . . . A4 . C5 . E5 . . . . . D5 . C5 . A4 . . . . . . . G4 . B4 . . . D5 . x .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . D4 . E4 . A4 . x . . . . . . . . . . . . . . . . . . . . . . . . . E4 . D4 . C4 . x .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . C4 . D4 . E4 . x . . . . . . . . . . . . . . . . . . . . . . . . . D4 . E4 . F4 . x .') +
        bars('. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . D4 . . . . . . . . . . . . . . .') +
        bars('F4 . . . . . . . . . . . . . . . D4 . . . . . . . . . . . . . . . G4 . . . . . . . . . . . . . . . E4 . . . . . . . . . . . . . . .') +
        bars('F4 . . . . . . . . . . . . . . . D4 . . . . . . . . . . . . . . . E4 . . . . . . . . . . . . . . . E4 . . . . . . . . . . . x . . .') +
        bars('G4 . . . . . . . . . . . . . . . E4 . . . . . . . . . . . . . . . A4 . . . . . . . . . . . . . . . F#4 . . . . . . . . . . . . . . .') +
        bars('G4 . . . . . . . . . . . . . . . E4 . . . . . . . . . . . . . . . F#4 . . . . . . . . . . . . . . . F#4 . . . . . . . . . . . . . . .') +
        bars('. . . . . . . . x . . . . . . . . . . . . . . . . . . . . . F#4 . B4 . D5 . F#5 . . . . . . . E5 . D5 . . . . . B4 . . . C#5 . . . . . D5 .') +
        bars('. . . . . . x . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . F#4 . . . . . . . . . . . . . . . . .')
    )
    c2 = (
        bars('C3 . G3 . C4 . D4 . E4 . D4 . C4 . G3 . B2 . D3 . G3 . B3 . D4 . B3 . G3 . D3 . A2 . E3 . A3 . C4 . E4 . C4 . A3 . E3 . F2 . C3 . A3 . C3 . G2 . D3 . B3 . D3 .') +
        bars('C3 . G3 . C4 . D4 . E4 . D4 . C4 . G3 . B2 . D3 . G3 . B3 . D4 . B3 . G3 . D3 . F2 . C3 . A3 . C3 . E2 . C3 . G3 . C3 . D2 . A2 . F3 . A2 . G2 . D3 . C4 . B3 .') +
        bars('C3 . G3 . E4 . G3 . C4 . G3 . E4 . G3 . B2 . D3 . G3 . D3 . B3 . D3 . G3 . D3 . A2 . E3 . C4 . E3 . A3 . E3 . G3 . E3 . G2 . E3 . C4 . E3 . G3 . E3 . C4 . E3 .') +
        bars('F2 . C3 . A3 . C3 . F3 . C3 . A3 . C3 . E2 . C3 . G3 . C3 . E3 . C3 . G3 . C3 . D2 . A2 . F3 . A2 . D3 . A2 . C3 . A2 . G2 . D3 . C4 . D3 . G3 . D3 . B3 . D3 .') +
        bars('F2 . C3 . A3 . C3 . E3 . C3 . A3 . C3 . E2 . B2 . G3 . B2 . A2 . E3 . C4 . E3 . D2 . A2 . F3 . A2 . D3 . A2 . C3 . A2 . G2 . D3 . C4 . D3 . G3 . D3 . B3 . D3 .') +
        bars('F2 . C3 . A3 . C3 . E3 . C3 . A3 . C3 . G2 . D3 . B3 . D3 . G3 . D3 . B3 . D3 . E2 . B2 . G3 . B2 . E3 . B2 . D3 . B2 . A2 . E3 . C4 . E3 . A3 . E3 . G3 . E3 .') +
        bars('D2 . A2 . F3 . A2 . D3 . A2 . C3 . A2 . G2 . D3 . B3 . D3 . G3 . D3 . B3 . D3 . C3 . G3 . E4 . G3 . C4 . G3 . E4 . G3 . C3 . G3 . E4 . G3 . G2 . D3 . B3 . D3 .') +
        bars('C3 . G3 . E4 . G3 . C4 . G3 . E4 . G3 . B2 . D3 . G3 . D3 . B3 . D3 . G3 . D3 . A2 . E3 . C4 . E3 . A3 . E3 . G3 . E3 . F2 . C3 . A3 . C3 . G2 . D3 . B3 . D3 .') +
        bars('C3 . G3 . E4 . G3 . C4 . G3 . E4 . G3 . B2 . D3 . G3 . D3 . B3 . D3 . G3 . D3 . A2 . E3 . C4 . E3 . A3 . E3 . G3 . E3 . G2 . E3 . C4 . E3 . G3 . E3 . C4 . E3 .') +
        bars('F2 . C3 . A3 . C3 . F3 . C3 . A3 . C3 . E2 . C3 . G3 . C3 . E3 . C3 . G3 . C3 . D2 . A2 . F3 . A2 . D3 . A2 . C3 . A2 . G2 . D3 . C4 . D3 . G3 . D3 . B3 . D3 .') +
        bars('F2 . C3 . A3 . C3 . E3 . C3 . A3 . C3 . E2 . B2 . G3 . B2 . A2 . E3 . C4 . E3 . D2 . A2 . F3 . A2 . D3 . A2 . C3 . A2 . G2 . D3 . C4 . D3 . G3 . D3 . B3 . D3 .') +
        bars('F2 . C3 . A3 . C3 . E3 . C3 . A3 . C3 . G2 . D3 . B3 . D3 . G3 . D3 . B3 . D3 . E2 . B2 . G3 . B2 . E3 . B2 . D3 . B2 . A2 . E3 . C4 . E3 . A3 . E3 . G3 . E3 .') +
        bars('D2 . A2 . F3 . A2 . D3 . A2 . C3 . A2 . G2 . D3 . B3 . D3 . G3 . D3 . B3 . D3 . C3 . G3 . E4 . G3 . C4 . G3 . E4 . G3 . A2 . E3 . D4 . E3 . A3 . E3 . C#4 . E3 .') +
        bars('G2 . D3 . B3 . D3 . F#3 . D3 . B3 . D3 . A2 . E3 . C#4 . E3 . A3 . E3 . C#4 . E3 . F#2 . C#3 . A3 . C#3 . F#3 . C#3 . E3 . C#3 . B2 . F#3 . D4 . F#3 . B3 . F#3 . A3 . F#3 .') +
        bars('E2 . B2 . G3 . B2 . E3 . B2 . D3 . B2 . A2 . E3 . C#4 . E3 . A3 . E3 . C#4 . E3 . D3 . A3 . F#4 . A3 . D4 . A3 . F#4 . A3 . D3 . A3 . F#4 . A3 . D4 . A3 . F#4 . E4 .') +
        bars('D3 . A3 . D4 . E4 . F#4 . E4 . D4 . A3 . C#3 . E3 . A3 . E3 . C#4 . E3 . A3 . E3 . B2 . F#3 . B3 . D4 . F#4 . D4 . B3 . F#3 . G2 . D3 . B3 . D3 . A2 . E3 . C#4 . E3 .') +
        bars('D3 . . . A3 . . . D4 . . . E4 . . . B2 . . . F#3 . . . B3 . . . D4 . . . E2 . . . B2 . . . E3 . . . G3 . . . G2 . B2 . D3 . F#3 . G3 . . . . . . .')
    )
    c3 = (
        [REST,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,1,HOLD] +
        [3,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD] +
        [0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,1,HOLD] +
        [0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,1,HOLD] +
        [0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD] +
        [0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD] +
        [0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,1,1] +
        [3,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD] +
        [0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,1,HOLD,1,1] +
        [3,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD] +
        [0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,3,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,4,HOLD,0,HOLD,4,HOLD,1,HOLD,HOLD,HOLD] +
        [0,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,0,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD] +
        [HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD]
    )
    return dict(ch=[c0, c1, c2, c3], loop=0, speed=12,
                env1=envelope(13, 0, 0, 2), env2=envelope(11, 0, 0, 1), wavevol=0x2000)

SONGS['SELUNE'] = selune()


# ------------------------------------------------------------ BOSSA (jukebox)
# TOTAL_ROWS = 640
# FORM: 0-127 A | 128-255 A' | 256-383 B (Jobim one-note, harmony walks) | 384-511 A'' | 512-639 C/turnaround+seam
#   Harmony: | Cmaj7 | Cmaj7 | Dm7 | G7(9) | Em7 | A7(b9)->Dm7 | Dm7 G7 | Cmaj7 | (A, A')
#   B walk under one note E5: Cmaj7 | Am7 | Fmaj7 | Dm7 | Em7 | A7 | Dm7 | G7(9)
#   C: Cmaj7(lyd) | Cmaj7 | G#maj7(bVI) | G#maj7 | Em7 | A7(b9) | A#7(bVII7) | Dm7 G7 -> loops mid-phrase into bar1
def bossa():
    c0 = (
        bars('E4 . . . G4 . . . A4 . . . . . D5 . . . . . C5 . . . . . B4 . . . . . C5 . . . A4 . . . . . . . . . F4 . . . . . G4 . . . A4 . . . . . . .') +
        bars('G4 . . . . . E4 . . . . . . . B4 . . . . . C5 . . . A4 . . . G4 . F4 . . . . . A4 . . . D5 . . . . . . . C5 . . . . . E4 . . . . . . . D5 .') +
        bars('. . C5 . . . B4 . . . G4 . . . . . E4 . . . . . G4 . A4 . . . . . C5 . . . . . A4 . . . F4 . . . . . E4 . . . . . D5 . . . . . B4 . . . . .') +
        bars('G4 . . . . . A4 . B4 . . . . . . . . . C5 . . . A4 . . . G4 . . . F4 . . . . . A4 . . . D5 . . . . . E5 . . . . . . . . . . . . . . . E5 .') +
        bars('E5 . . . . . E5 . . . . . E5 . . . E5 . . . . . E5 . . . . . E5 . . . E5 . . . . . E5 . . . . . E5 . . . E5 . . . . . E5 . . . . . E5 . . .') +
        bars('E5 . . . . . E5 . . . . . E5 . . . E5 . . . . . E5 . . . . . E5 . . . E5 . . . . . E5 . . . . . E5 . . . E5 . . . . . D5 . . . . . . . C5 .') +
        bars('. . . . B4 . . . G4 . . . . . A4 . . . . . G4 . . . E4 . . . . . D5 . . . . . F4 . . . A4 . . . . . . . G4 . . . . . D4 . . . . . . . B4 .') +
        bars('. . . . G4 . . . E4 . . . . . . . . . C5 . . . A4 . . . G4 . . . F4 . . . . . A4 . . . D5 . . . . . . . C5 . . . . . E4 . . . . . . . D5 x') +
        bars('. . . . E5 . . . D5 . . . . . C5 . . . . . C5 . . . A4 . . . . . G4 . . . . . C5 . . . D5 . . . . . . . . . . . C5 . . . . . A4 . . . . .') +
        bars('G4 . . . . . B4 . . . . . . . C5 . . . . . A4 . . . G4 . . . . . F4 . . . . . A4 . . . G4 . . . . . . . . . . . A4 . . . D5 . . . . . E4 .')
    )
    c1 = (
        bars('. . E3 . . . B3 . . . E3 . . . . . . . B3 . . . E3 . . . . . . . B3 . . . F3 . . . C4 . . . F3 . . . . . . . B3 . . . F3 . . . B3 . . . . .') +
        bars('. . G3 . . . . . . . E3 . G3 . . . . . G3 . . . C4 . . . G3 . . . . . . . F3 . . . C4 . . . F3 . . . . . . . E3 . . . . . . . B3 . E3 . . .') +
        bars('. . . . . . . . . . . . E3 . B3 . . . B3 . . . . . . . E3 . B3 . . . . . F3 . . . C4 . . . F3 . . . . . . . B3 . . . F3 . . . . . . . B3 .') +
        bars('. . G3 . . . . . . . E3 . . . G3 . . . . . G3 . . . . . . . C4 . . . . . F3 . . . C4 . . . F3 . . . . . . . E3 . . . B3 . . . E3 . . . . .') +
        bars('. . E3 . . . . . . . B3 . . . E3 . . . G3 . . . . . . . C4 . . . G3 . . . E3 . . . . . . . A3 . . . E3 . . . F3 . . . . . . . C4 . . . F3 .') +
        bars('. . G3 . . . . . . . B3 . . . G3 . . . G3 . . . . . . . C4 . . . G3 . . . F3 . . . . . . . C4 . . . F3 . . . B3 . . . . . . . F3 . B3 . . .') +
        bars('. . E3 . . . B3 . . . E3 . . . . . . . B3 . . . E3 . . . B3 . . . . . . . F3 . . . C4 . . . F3 . . . . . . . B3 . . . . . . . F3 . B3 . . .') +
        bars('. . G3 . . . E3 . . . G3 . . . . . . . . . G3 . . . . . . . C4 . . . . . F3 . . . C4 . . . F3 . . . . . . . E3 . . . . . . . B3 . E3 . . .') +
        bars('. . E3 . . . B3 . . . E3 . . . . . . . B3 . . . E3 . . . B3 . . . . . . . G3 . . . C4 . . . G3 . . . . . . . C4 . . . G3 . . . . . . . C4 .') +
        bars('. . G3 . . . . . . . B3 . G3 . . . . . G3 . . . C4 . . . G3 . . . . . . . G#3 . . . C4 . . . G#3 . . . . . . . F3 . . . B3 . . . F3 . . . . .')
    )
    c2 = (
        bars('C3 . . . . . . . G2 . . . . . . . C3 . . . . . . . G2 . . . . . C#3 . D3 . . . . . . . A2 . . . . . F#2 . G2 . . . . . . . D3 . . . . . . .') +
        bars('E3 . . . . . . . B2 . . . . . . . A2 . . . . . . . E3 . . . . . C#3 . D3 . . . . . . . G2 . . . . . B2 . C3 . . . . . . . G2 . . . . . . .') +
        bars('C3 . . . . . . . G2 . . . . . . . C3 . . . . . . . G2 . . . . . C#3 . D3 . . . . . . . A2 . . . . . F#2 . G2 . . . . . . . D3 . . . . . . .') +
        bars('E3 . . . . . . . B2 . . . . . . . A2 . . . . . . . E3 . . . . . C#3 . D3 . . . . . . . G2 . . . . . B2 . C3 . . . . . . . G2 . . . . . . .') +
        bars('C3 . . . . . . . G2 . . . . . . . A2 . . . . . . . E3 . . . . . . . F2 . . . . . . . C3 . . . . . . . D3 . . . . . . . A2 . . . . . . .') +
        bars('E3 . . . . . . . B2 . . . . . . . A2 . . . . . . . E3 . . . . . C#3 . D3 . . . . . . . A2 . . . . . . . G2 . . . . . . . D3 . . . . . B2 .') +
        bars('C3 . . . . . . . G2 . . . . . . . C3 . . . . . . . G2 . . . . . C#3 . D3 . . . . . . . A2 . . . . . F#2 . G2 . . . . . . . D3 . . . . . . .') +
        bars('E3 . . . . . . . B2 . . . . . . . A2 . . . . . . . E3 . . . . . C#3 . D3 . . . . . . . G2 . . . . . B2 . C3 . . . . . . . G2 . . . . . A2 .') +
        bars('C3 . . . . . . . G2 . . . . . A2 . C3 . . . . . . . G2 . . . . . A2 . G#2 . . . . . . . D#3 . . . . . . . G#2 . . . . . . . D#3 . . . . . F2 .') +
        bars('E3 . . . . . . . B2 . . . . . . . A2 . . . . . . . E3 . . . . . . . A#2 . . . . . . . F3 . . . . . C#3 . D3 . . . . . . . G2 . . . . . B2 .')
    )
    c3 = (
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD] +
        [1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,1,HOLD,2,HOLD,0,HOLD,2,HOLD,1,HOLD,2,HOLD,1,HOLD,2,HOLD,2,HOLD,2,HOLD]
    )
    return dict(ch=[c0, c1, c2, c3], loop=0, speed=7,
                env1=envelope(11, 0, 0, 2), env2=envelope(11, 2, 0, 1), wavevol=0x2000)

SONGS['BOSSA'] = bossa()

ORDER = ['PRELUDE', 'EXPLORE', 'BATTLE', 'BOSS', 'VICTORY', 'CRASH', 'AZURE', 'GAIA', 'TADPOLE', 'SELUNE', 'BOSSA']

# Jukebox display names, one per ORDER entry. mkassets emits song_names[]
# from this and asserts it stays in step with ORDER -- and that the jukebox
# screen still has a slot for every track.
TITLES = {
    'PRELUDE': 'Prelude',
    'EXPLORE': 'Fleshy Halls',
    'BATTLE':  'Draw Steel!',
    'BOSS':    'Commander Zhalk',
    'VICTORY': 'Victory',
    'CRASH':   'The Long Fall',
    'AZURE':   'Kind of Azure',
    'GAIA':    'Gaia',
    'TADPOLE': 'Tadpole Twist',
    'SELUNE':  'Under Selune',
    'BOSSA':   "Baldur's Bossa",
}
