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

# Each song: dict(ch=[ch0,ch1,ch2,ch3], loop, speed, env1, env2, wavevol)
# env: SOUNDxCNT_H value. duty bits 6-7, env-step 8-10, dir bit11, init-vol 12-15.
# 0xF080 = vol15 decay-off(step0) 50% duty (sustained). 0xC780 = softer.
SONGS = {}

def envelope(vol=12, step=0, up=0, duty=2):
    return (vol << 12) | (up << 11) | (step << 8) | (duty << 6)

# ------------------------------------------------------------ PRELUDE (title)
# Rising harp arpeggios over a slow tonic drift — FF "Prelude" idiom, original.
def prelude():
    wave = []
    arps = [('C3','E3','G3','C4','E4','G4','C5','E4'),
            ('C3','E3','G3','C4','E4','G4','A4','E4'),
            ('A2','C3','E3','A3','C4','E4','A4','C4'),
            ('A2','C3','E3','A3','C4','E4','A4','C4'),
            ('F2','A2','C3','F3','A3','C4','F4','A3'),
            ('F2','A2','C3','F3','A3','C4','F4','A3'),
            ('G2','B2','D3','G3','B3','D4','G4','D4'),
            ('G2','B2','D3','G3','B3','D4','G4','D4')]
    for a in arps:
        for note in a: wave += [n(note), HOLD]
    # soft high melody entering second half
    mel = pad([], 128, REST)
    for i, note in enumerate(['E5','x','G5','x','C6','x','B5','x',
                              'A5','x','x','x','G5','x','x','x']):
        mel[64 + i * 2] = n(note) if note != 'x' else HOLD
    bass = []
    for root in ['C2','C2','A2','A2','F2','F2','G2','G2']:
        bass += hold_to(root, 16)
    return dict(ch=[mel, [REST]*128, wave, [REST]*128], loop=0, speed=9,
                env1=envelope(11, 2, 0, 2), env2=0, wavevol=0x2000)

SONGS['PRELUDE'] = prelude()

# ------------------------------------------------------------ EXPLORE (dungeon)
# Slow A-minor dread; sustained bass drone, sparse minor melody, heartbeat kick.
def explore():
    mel = seq('A3',HOLD,HOLD,HOLD,'C4',HOLD,'B3',HOLD,
              'A3',HOLD,HOLD,HOLD,REST,REST,'E4',HOLD,
              'F4',HOLD,HOLD,HOLD,'E4',HOLD,'D4',HOLD,
              'C4',HOLD,HOLD,HOLD,REST,REST,REST,REST,
              'A3',HOLD,HOLD,HOLD,'E4',HOLD,'F4',HOLD,
              'E4',HOLD,'D4',HOLD,'C4',HOLD,'B3',HOLD,
              'A3',HOLD,HOLD,HOLD,'G#3',HOLD,HOLD,HOLD,
              'A3',HOLD,HOLD,HOLD,REST,REST,REST,REST)
    harm = seq('E3',HOLD,HOLD,HOLD,'E3',HOLD,HOLD,HOLD,
               'E3',HOLD,HOLD,HOLD,REST,REST,REST,REST,
               'A3',HOLD,HOLD,HOLD,'A3',HOLD,HOLD,HOLD,
               'G3',HOLD,HOLD,HOLD,REST,REST,REST,REST,
               'C4',HOLD,HOLD,HOLD,'C4',HOLD,HOLD,HOLD,
               'B3',HOLD,HOLD,HOLD,'B3',HOLD,HOLD,HOLD,
               'E3',HOLD,HOLD,HOLD,'E3',HOLD,HOLD,HOLD,
               'A3',HOLD,HOLD,HOLD,REST,REST,REST,REST)
    bass = []
    for root in ['A2','A2','F2','G2']:
        bass += hold_to(root, 16)
    drum = pad([], 64, REST)
    for i in range(0, 64, 8): drum[i] = 0        # slow heartbeat kick
    for i in range(4, 64, 16): drum[i] = 2       # occasional hat
    return dict(ch=[mel, harm, bass, drum], loop=0, speed=10,
                env1=envelope(11, 0, 0, 2), env2=envelope(8, 0, 0, 1),
                wavevol=0x2000)

SONGS['EXPLORE'] = explore()

# ------------------------------------------------------------ BATTLE
# Fast driving D-minor; urgent square lead, walking bass, backbeat drums.
def battle():
    lead = seq('D4','A4','D4','A4','F4','A4','D4','A4',
               'C4','G4','C4','G4','E4','G4','C4','G4',
               'D4','F4','A4','F4','D4','A4','F4','D4',
               'E4','G4','B4','G4','E4','B4','G4','E4',
               'D4','A4','D5','A4','F4','A4','D5','A4',
               'C5','B4','A4','G4','F4','E4','D4','C4',
               'A3','D4','F4','A4','D5','A4','F4','D4',
               'E4','E4','C#4','C#4','D4','D4','REST' if False else HOLD,'A4')
    harm = seq('F3','A3','F3','A3','D3','F3','A3','F3',
               'E3','G3','E3','G3','C3','E3','G3','E3',
               'D3','F3','A3','F3','D3','A3','F3','D3',
               'C3','E3','G3','E3','C3','G3','E3','C3',
               'F3','A3','D4','A3','F3','A3','D4','A3',
               'E3','D3','C3','B2','A2','G2','F2','E2',
               'D3','F3','A3','D4','F4','D4','A3','F3',
               'A3','A3','G#3','G#3','A3','A3','A3','A3')
    bass = seq('D2','D3','D2','D3','D2','D3','A2','A3',
               'C2','C3','C2','C3','C2','C3','G2','G3',
               'D2','D3','D2','D3','A2','A3','A2','A3',
               'E2','E3','E2','E3','A2','A3','E2','E3',
               'D2','D3','D2','D3','D2','D3','A2','A3',
               'F2','F3','C2','C3','A2','A3','E2','E3',
               'D2','A2','D3','A2','D2','A2','D3','A2',
               'A2','A2','E2','E2','A2','A2','A2','A2')
    drum = []
    beat = [0, 2, 1, 2, 0, 0, 1, 2]
    drum = rep(beat, 8)
    return dict(ch=[lead, harm, bass, drum], loop=0, speed=6,
                env1=envelope(12, 0, 0, 2), env2=envelope(9, 0, 0, 1),
                wavevol=0x2000)

SONGS['BATTLE'] = battle()

# ------------------------------------------------------------ BOSS (Zhalk)
# Heavier, faster, chromatic menace in E-minor with a pounding tom pattern.
def boss():
    lead = seq('E4','E4','B4','E4','G4','E4','B4','G4',
               'E4','E4','C5','E4','B4','G4','E4','B3',
               'E4','E4','B4','E4','A4','E4','C5','A4',
               'F#4','F#4','D5','F#4','C5','A4','F#4','D4',
               'E4','G4','B4','E5','D5','B4','G4','E4',
               'C5','B4','A4','G4','F#4','E4','D#4','E4',
               'B4','B4','A#4','B4','C5','B4','A4','G4',
               'F#4','G4','A4','B4','E4','E4','E4','E4')
    harm = seq('E3','B3','E3','B3','E3','B3','E3','B3',
               'E3','B3','E3','B3','E3','G3','E3','G3',
               'A3','E3','A3','E3','A3','E3','A3','E3',
               'D3','A3','D3','A3','D3','F#3','D3','F#3',
               'E3','B3','E3','B3','G3','B3','E3','B3',
               'A3','G3','F#3','E3','D3','C3','B2','B2',
               'B3','B3','A#3','B3','C4','B3','A3','G3',
               'F#3','G3','A3','B3','E3','E3','E3','E3')
    bass = []
    for root in ['E2','E2','A2','F#2','E2','C2','B1' if False else 'B2','E2']:
        bass += rep([n(root), n(root)+12], 4)
    drum = rep([0, 0, 1, 0, 0, 1, 0, 3], 8)
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
    harm = seq('E4','E4','E4','E4','E4',HOLD,HOLD,'E4',
               'E4',HOLD,'G4',HOLD,'C5',HOLD,HOLD,HOLD,
               'C5','G4','E4','G4','C5',HOLD,'B4',HOLD,
               'G4',HOLD,HOLD,HOLD,HOLD,HOLD,REST,REST)
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
    bass = seq('D3',HOLD,'C3',HOLD,'A#2',HOLD,'G#2',HOLD,
               'F#2',HOLD,'E2',HOLD,'D2',HOLD,'C2',HOLD,
               'A#1' if False else 'A#2',HOLD,'G#2',HOLD,'F#2',HOLD,'E2',HOLD,
               'D2',HOLD,HOLD,HOLD,'D2',HOLD,'D2',HOLD)
    drum = seq(0,REST,0,REST,0,REST,0,REST, 0,REST,0,REST,0,0,0,0,
               3,3,3,3,3,3,3,3, 0,0,0,0,0,0,0,0)
    return dict(ch=[lead, [REST]*32, bass, drum], loop=0xFFFF, speed=6,
                env1=envelope(13, 0, 0, 2), env2=0, wavevol=0x2000)

SONGS['CRASH'] = crash()

ORDER = ['PRELUDE', 'EXPLORE', 'BATTLE', 'BOSS', 'VICTORY', 'CRASH']
