# Original chiptunes in the FF4 idiom. Note data compiled to C by mkassets.
# Encoding per row: a note index (0..59, C2..B6), HOLD=60 (sustain), REST=61 (cut).
# Drum channel rows: 0 kick, 1 snare, 2 hat, 3 open-hat, HOLD/REST as above.

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
def prelude():
    wave = []
    arps = [('C3','E3','G3','C4','E4','G4','C5','E4'),      # C
            ('C3','E3','G3','C4','E4','G4','A4','E4'),      # C6
            ('A2','C3','E3','A3','C4','E4','A4','C4'),      # Am
            ('A2','C3','E3','A3','C4','E4','A4','C4'),
            ('F2','A2','C3','F3','A3','C4','F4','A3'),      # F
            ('F2','A2','C3','F3','A3','C4','F4','A3'),
            ('G2','B2','D3','G3','B3','D4','G4','D4'),      # G
            ('G2','B2','D3','G3','B3','D4','G4','B4'),      #   ...climbing
            ('G#2','C3','D#3','G#3','C4','D#4','G#4','D#4'),# Ab  (bVI lift)
            ('G#2','C3','D#3','G#3','C4','D#4','C5','G#4'),
            ('A#2','D3','F3','A#3','D4','F4','A#4','F4'),   # Bb  (bVII)
            ('A#2','D3','F3','A#3','D4','F4','D5','A#4'),
            ('F2','A2','C3','F3','A3','C4','F4','A4'),      # F   (home stretch)
            ('D3','F3','A3','D4','F4','A4','F4','D4'),      # Dm
            ('G2','B2','D3','G3','B3','D4','G4','B4'),      # G
            ('G2','B2','D3','F3','G3','B3','D4','F4')]      # G7 -> loop (V7-I seam)
    for a in arps:
        for note in a: wave += [n(note), HOLD]
    RB = 'x . . . . . . . . . . . . . . .'                  # silent bar
    mel = bars(RB + '|' + RB + '|' + RB + '|' + RB +        # A: arps alone
        '''| A5 . . .  G5 . F5 .  A5 . . .  . . x .
           | x . . .   . . . .    . . . .   . . . .
           | B5 . . .  A5 . G5 .  B5 . . .  . . x .
           | x . . .   . . . .    . . . .   . . . .
           | C6 . . .  . . . .    . . . .   . . . .
           | A#5 . . . G#5 . . .  D#5 . . . . . x .
           | D6 . . .  C6 . A#5 . F5 . . .  . . . .
           | x . . .   . . . .    . . . .   . . . .
           | C6 . . .  A5 . G5 .  F5 . . .  . . x .
           | F5 . G5 . A5 . . .   D5 . . .  . . x .
           | B5 . . .  D6 . . .   G5 . . .  . . . .
           | G5 . F5 . D5 . B4 .  D5 . . .  . . . .''')
    harm = bars(RB + '|' + RB + '|' + RB + '|' + RB + '|' + RB +
        '''| A4 . . .  G4 . F4 .  A4 . . .  . . x .
           | x . . .   . . . .    . . . .   . . . .
           | B4 . . .  A4 . G4 .  B4 . . .  . . x .
           | D#5 . . . . . . .    . . . .   . . . .
           | D#5 . . . C5 . . .   G#4 . . . . . x .
           | A#4 . . . . . . .    . . . .   . . . .
           | F5 . D5 . A#4 . F4 . A#4 . . . . . x .
           | A4 . . .  . . . .    . . . .   . . . .
           | F4 . . .  . . . .    F4 . . .  . . x .
           | G4 . . .  . . . .    . . . .   . . . .
           | D4 . . .  . . . .    B3 . . .  . . . .''')
    return dict(ch=[mel, harm, wave, [REST] * 256], loop=0, speed=9,
                env1=envelope(11, 2, 0, 2), env2=envelope(8, 2, 0, 1),
                wavevol=0x2000)

SONGS['PRELUDE'] = prelude()

# ------------------------------------------------------------ EXPLORE (dungeon)
# A-minor dread, 224 rows, form A A' B A''.
#   A  (0-63):    sparse melody over sustained dyads; Am Am F Am Dm C E Am.
#   A' (64-127):  same tune, ch1 becomes a counter-melody (6ths + contrary).
#   B  (128-159): lift to C major (fragile hope), iv-V retransition @152.
#   A'' (160-223): reprise; seam bar swaps in an E7 pickup (G#-B -> A) @216.
def explore():
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
    melB  = bars('''E4 . G4 . C5 . .  .  | B4 . G4 . D5 . .   .
                  | A4 . G4 . F4 . E4 .  | F4 . D4 . B3 . G#3 .''')
    harmB = bars('''C4 . E4 . G4 . .  .  | G4 . D4 . B4 . .   .
                  | F4 . D4 . C4 . .  .  | D4 . A3 . E3 . .   .''')
    bassB = bars('''C2 . . .  G2 . .  .  | G2 . . .  B2 . .   .
                  | F2 . . .  C3 . .  .  | D2 . . .  E2 . B2  .''')
    melS  = bars('A3 . . . x  x G#3 B3')          # seam: E7 pickup -> loop A3
    harmS = bars('C3 . . . E3 . .   .')
    bassS = bars('A2 . . . E2 . E2  .')
    mel  = melA + melA + melB + melA[:56] + melS
    harm = harmA + cntr + harmB + harmA[:56] + harmS
    bass = bassA + bassA + bassB + bassA[:56] + bassS
    R = REST
    beat  = [0,R,R,R, 2,R,R,R, 0,R,R,R, R,R,R,R]  # heartbeat kick, distant hat
    beatB = [0,R,R,R, 2,R,R,R]                    # B: hats double (small earn)
    drum = rep(beat, 8) + rep(beatB, 3) + [0,R,R,R, 2,R,1,1] \
         + rep(beat, 3) + beatB + [0,R,R,R, 0,R,1,1]
    return dict(ch=[mel, harm, bass, drum], loop=0, speed=10,
                env1=envelope(11, 0, 0, 2), env2=envelope(8, 0, 0, 1),
                wavevol=0x2000)

SONGS['EXPLORE'] = explore()

# ------------------------------------------------------------ BATTLE
# Fast driving D-minor, 128 rows.
#   A (0-63):    original drive (harm bar 6 raised an octave to stay in a 12th).
#   B (64-119):  VI-VII-i riff sprint Bb-C-Dm, A(V), then Gm-A and a scale run.
#   T (120-127): drum-fill turnaround on A7, chromatic bass C#2 -> D2 at loop.
def battle():
    lead = seq('D4','A4','D4','A4','F4','A4','D4','A4',
               'C4','G4','C4','G4','E4','G4','C4','G4',
               'D4','F4','A4','F4','D4','A4','F4','D4',
               'E4','G4','B4','G4','E4','B4','G4','E4',
               'D4','A4','D5','A4','F4','A4','D5','A4',
               'C5','B4','A4','G4','F4','E4','D4','C4',
               'A3','D4','F4','A4','D5','A4','F4','D4',
               'E4','E4','C#4','C#4','D4','D4',HOLD,'A4') + bars(
        '''A#4 F4  D4  F4   A#4 F4  D5 A#4 | C5 G4  E4  G4   C5  G4  E5 C5
         | D5  A4  F4  A4   D5  A4  F5 D5  | E5 C#5 A4  C#5  E5  C#5 A4 E4
         | G4  D4  A#3 D4   G4  D4  A#4 G4 | A4 E4  C#4 E4   A4  C#5 E5 C#5
         | D5  C5  A#4 A4   G4  F4  E4  C#4| A4 .   x   x    A3  C#4 E4 G4''')
    harm = seq('F3','A3','F3','A3','D3','F3','A3','F3',
               'E3','G3','E3','G3','C3','E3','G3','E3',
               'D3','F3','A3','F3','D3','A3','F3','D3',
               'C3','E3','G3','E3','C3','G3','E3','C3',
               'F3','A3','D4','A3','F3','A3','D4','A3',
               'E4','D4','C4','B3','A3','G3','F3','E3',
               'D3','F3','A3','D4','F4','D4','A3','F3',
               'A3','A3','G#3','G#3','A3','A3','A3','A3') + bars(
        '''D4  D4  A#3 D4   D4  D4  F4 D4  | E4 E4  C4  E4   E4  E4  G4 E4
         | F4  F4  D4  F4   F4  F4  A4 F4  | A4 A4  E4  A4   A4  A4  E4 C#4
         | A#3 A#3 G3  A#3  A#3 A#3 D4 A#3 | C#4 C#4 A3 C#4  C#4 E4  A4 E4
         | F4  E4  D4  C#4  A#3 A3  G3 E3  | C#4 .  x   x    x   A3  C#4 E4''')
    bass = seq('D2','D3','D2','D3','D2','D3','A2','A3',
               'C2','C3','C2','C3','C2','C3','G2','G3',
               'D2','D3','D2','D3','A2','A3','A2','A3',
               'E2','E3','E2','E3','A2','A3','E2','E3',
               'D2','D3','D2','D3','D2','D3','A2','A3',
               'F2','F3','C2','C3','A2','A3','E2','E3',
               'D2','A2','D3','A2','D2','A2','D3','A2',
               'A2','A2','E2','E2','A2','A2','A2','A2') + bars(
        '''A#2 . F2 . A#2 . B2  . | C3 . G2 . C3 . C#3 .
         | D3  . A2 . D3  . E3  . | A2 . E2 . A2 . F#2 .
         | G2  . D2 . G2  . G#2 . | A2 . E2 . A2 . C#3 .
         | D3  . A2 . F2  . E2  . | A2 . E2 . A2 . C#2 .''')
    beat = [0, 2, 1, 2, 0, 0, 1, 2]
    drum = rep(beat, 8) + rep(beat, 6) \
         + [0,1,0,1, 0,1,1,1] + [1,1,2,1, 1,1,1,3]      # build + fill turnaround
    return dict(ch=[lead, harm, bass, drum], loop=0, speed=6,
                env1=envelope(12, 0, 0, 2), env2=envelope(9, 0, 0, 1),
                wavevol=0x2000)

SONGS['BATTLE'] = battle()

# ------------------------------------------------------------ BOSS (Zhalk)
# E-minor menace, 128 rows.
#   A (0-63):    original riff (bars 2-4 revoiced: consonant, within a 12th).
#   B (64-95):   semitone lift to Fm then Ab; ch0 (sword) and ch1 (tentacle)
#                trade two-bar phrases; kick-run "tom" fill into the climb.
#   C (96-111):  chromatic bass grind Bb->F, squares trade rising stabs.
#   D (112-127): B7 peak in octaves, chromatic crash G-F#-F-E; V seam -> loop.
def boss():
    lead = seq('E4','E4','B4','E4','G4','E4','B4','G4',
               'E4','E4','C5','E4','B4','G4','E4','B3',
               'E4','E4','B4','E4','A4','E4','C5','A4',
               'F#4','F#4','D5','F#4','C5','A4','F#4','D4',
               'E4','G4','B4','E5','D5','B4','G4','E4',
               'C5','B4','A4','G4','F#4','E4','D#4','E4',
               'B4','B4','A#4','B4','C5','B4','A4','G4',
               'F#4','G4','A4','B4','E4','E4','E4','E4') + bars(
        '''F4  F4 C5  F4 G#4 F4  C5 G#4 | C5  .   .   .   x   x   x  x
         | G#4 G#4 D#5 G#4 F5 D#5 C5 G#4| D#5 .   .   .   x   x   x  x
         | A#4 .  x   x  C5  .   x  x   | D5  .   x   x   D#5 .   x  x
         | D#5 E5 F#5 E5 D#5 B4  A4 F#4 | G4  F#4 F4  E4  .   .   x  x''')
    harm = seq('E3','B3','E3','B3','E3','B3','E3','B3',
               'E3','B3','G3','B3','E3','G3','E3','G3',
               'A3','E3','B3','E3','A3','E3','A3','E3',
               'D3','A3','A3','A3','A3','F#3','D3','F#3',
               'E3','B3','E3','B3','G3','B3','E3','B3',
               'A3','G3','F#3','E3','D3','C3','B2','B2',
               'B3','B3','A#3','B3','C4','B3','A3','G3',
               'F#3','G3','A3','B3','E3','E3','E3','E3') + bars(
        '''x   x  x   x  x   x   x  x    | F3  F3  G#3 F3  C4  G#3 F3 C3
         | x   x  x   x  x   x   x  x    | G#3 G#3 C4  G#3 D#4 C4 G#3 D#3
         | x   x  F4  .  x   x   G4 .    | x   x   A4  .   x   x   B4 .
         | D#4 E4 F#4 E4 D#4 B3  A3 F#3  | G3  F#3 F3  E3  .   .   x  x''')
    bass = []
    for root in ['E2','E2','A2','F#2','E2','C2','B2','E2']:
        bass += rep([n(root), n(root) + 12], 4)
    bass += rep([n('F2'), n('F3')], 6) + seq('F2','F3','G2','G2') \
          + rep([n('G#2'), n('G#3')], 6) + seq('G#2','G#3','A2','A2') \
          + bars('''A#2 A#2 B2 B2 C3 C3 C#3 C#3 | D3 D3 D#3 D#3 E3 E3 F3 F3
                  | B2 B3 B2 B3 B2 B3 A2 G2     | E2 .  G2 .   A2 .  B2 .''')
    pound = [0, 0, 1, 0, 0, 1, 0, 3]
    drum = rep(pound, 8) \
         + rep(pound, 3) + [0,0,1,0, 0,0,0,0] \
         + [0,1,0,1, 0,1,0,1] + [0,1,0,1, 0,1,1,1] \
         + pound + [0,0,1,1, 0,1,1,3]
    return dict(ch=[lead, harm, bass, drum], loop=0, speed=5,
                env1=envelope(13, 0, 0, 3), env2=envelope(10, 0, 0, 2),
                wavevol=0x2000)

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

ORDER = ['PRELUDE', 'EXPLORE', 'BATTLE', 'BOSS', 'VICTORY', 'CRASH']
