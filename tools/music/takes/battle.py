Raising `B2 A2` → `B3 A3` keeps them a consonant minor-3rd below the lead and within the sweet range. That's the final polish. The composition is fully validated. Here is the finished re-cut.

```python
# TOTAL_ROWS = 256
# FORM: 0 A (Dm arpeggio drive) | 64 A' (escalate: 8va lift + descending scale run, countermelody) | 128 B BRIDGE (bVI-bVII Bb->C climb into F major, long sung phrases over a driven pedal) | 192 A'' (Dm drive reignites) | 208 KEY-LIFT (riff up a step Dm->Em) | 240 T TURNAROUND (drum-fill on A7 -> Dm, chromatic C#2->D2 seam back to row 0)
CH_LEAD = seq(  # ch0 sq1 50% -- the propulsive lead
    'D4','A4','D4','A4','F4','A4','D4','A4','C4','G4','C4','G4','E4','G4','C4','G4',  # bar 1  (A: Dm | C)
    'D4','F4','A4','F4','D4','A4','F4','D4','E4','G4','B4','G4','E4','B4','G4','E4',  # bar 2  (A: Dm | Em)
    'D4','A4','D4','A4','F4','A4','D4','A4','C4','G4','C4','G4','E4','G4','C4','G4',  # bar 3  (A: Dm | C)
    'D4','F4','A4','F4','D4','A4','F4','D4','E4','G4','B4','G4','E4','B4','G4','E4',  # bar 4  (A: Dm | Em)
    'D4','A4','D5','A4','F4','A4','D5','A4','C5','G4','C5','G4','E5','G4','C5','G4',  # bar 5  (A': Dm8va | C8va)
    'D5','A4','F4','A4','D5','A4','F4','D5','C5','B4','A4','G4','F4','E4','D4','C4',  # bar 6  (A': Dm | C-scale run)
    'A3','D4','F4','A4','D5','A4','F4','D4','E5','B4','G4','E4','E5','B4','G4','E4',  # bar 7  (A': Dm sweep | Em8va)
    'D5','A4','F4','D4','A4','F4','D4','A3','E4','E4','C#4','C#4','D4','A4',HOLD,'A4',# bar 8  (A': Dm cascade | E->A(V) pickup)
    'F4',HOLD,HOLD,HOLD,'A4',HOLD,HOLD,HOLD,'A#4',HOLD,HOLD,HOLD,'A4',HOLD,'F4',HOLD,# bar 9  (B: Bb bVI, sung)
    'G4',HOLD,HOLD,HOLD,'A#4',HOLD,HOLD,HOLD,'C5',HOLD,HOLD,HOLD,'A#4',HOLD,'G4',HOLD,# bar 10 (B: C bVII, climbs)
    'A4',HOLD,HOLD,HOLD,'C5',HOLD,HOLD,HOLD,'F5',HOLD,HOLD,HOLD,'E5',HOLD,'D5',HOLD, # bar 11 (B: F rel-major crest)
    'C5',HOLD,HOLD,HOLD,'A4',HOLD,HOLD,HOLD,'G4',HOLD,'F4',HOLD,'E4',HOLD,'D4',HOLD, # bar 12 (B: C7 settle home)
    'D4','A4','D4','A4','F4','A4','D4','A4','C4','G4','C4','G4','E4','G4','C4','G4',  # bar 13 (A'': Dm | C, reignite)
    'E4','B4','E4','B4','G4','B4','E4','B4','D4','A4','D4','A4','F#4','A4','D4','A4', # bar 14 (KEY-LIFT: Em | D  @208)
    'E4','G4','B4','G4','E4','B4','G4','E4','F#4','A4','D5','A4','F#4','D5','A4','F#4',# bar 15 (Em | B = V of Em)
    'A4',HOLD,'C#5','E5','A4',HOLD,HOLD,HOLD,'A3','C#4','E4','G4','A4',HOLD,'x','x')  # bar 16 (T: A7 -> loop)
CH_HARM = seq(  # ch1 sq2 25% -- harmony/countermelody, at or below the lead
    'F3','A3','F3','A3','D3','F3','A3','F3','E3','G3','E3','G3','C3','E3','G3','E3',  # bar 1
    'D3','F3','A3','F3','D3','A3','F3','D3','C3','E3','G3','E3','C3','G3','E3','C3',  # bar 2
    'F3','A3','F3','A3','D3','F3','A3','F3','E3','G3','E3','G3','C3','E3','G3','E3',  # bar 3
    'D3','F3','A3','F3','D3','A3','F3','D3','C3','E3','G3','E3','C3','G3','E3','C3',  # bar 4
    'F3','A3','D4','A3','F3','A3','D4','A3','E3','G3','C4','G3','E3','G3','C4','G3',  # bar 5  (A')
    'A3','F3','D3','F3','A3','F3','D3','A3','A3','G3','F3','E3','D3','C3','B3','A3',  # bar 6  (A' countermelody)
    'D3','F3','A3','D4','F4','D4','A3','F3','G3','E3','B3','G3','G3','E3','B3','G3',  # bar 7
    'D4','A3','F3','D3','F3','D3','A3','F3','A3','A3','G#3','G#3','A3','A3','A3','A3',# bar 8  (A into bridge)
    'D4',HOLD,HOLD,HOLD,'F4',HOLD,HOLD,HOLD,'F4',HOLD,HOLD,HOLD,'F4',HOLD,'D4',HOLD, # bar 9  (B: Bb = D-F)
    'E4',HOLD,HOLD,HOLD,'G4',HOLD,HOLD,HOLD,'G4',HOLD,HOLD,HOLD,'G4',HOLD,'E4',HOLD, # bar 10 (B: C = E-G)
    'F4',HOLD,HOLD,HOLD,'A4',HOLD,HOLD,HOLD,'C5',HOLD,HOLD,HOLD,'C5',HOLD,'A4',HOLD, # bar 11 (B: F = F-A-C)
    'E4',HOLD,HOLD,HOLD,'F4',HOLD,HOLD,HOLD,'E4',HOLD,'D4',HOLD,'C4',HOLD,'A#3',HOLD,# bar 12 (B: C7 -> Dm approach)
    'F3','A3','F3','A3','D3','F3','A3','F3','E3','G3','E3','G3','C3','E3','G3','E3',  # bar 13
    'G3','B3','G3','B3','E3','G3','B3','G3','F3','A3','F3','A3','D3','F3','A3','F3',  # bar 14 (Em | D harm  @208)
    'G3','B3','E3','B3','G3','B3','E3','G3','A3','A3','F#3','F#3','A3','F#3','A3','A3',# bar 15 (Em | B(F#))
    'C#4',HOLD,'A3','C#4','C#4',HOLD,HOLD,HOLD,'x','A3','C#4','E4','C#4',HOLD,'x','x')# bar 16 (T: A7 tones)
CH_BASS = seq(  # ch2 wave (sounds 8vb) -- root-octave drive; bridge = pedal under moving harmony
    'D2','D3','D2','D3','D2','D3','A2','A3','C2','C3','C2','C3','C2','C3','G2','G3',  # bar 1  (Dm | C)
    'D2','D3','D2','D3','A2','A3','A2','A3','E2','E3','E2','E3','A2','A3','E2','E3',  # bar 2  (Dm | Em)
    'D2','D3','D2','D3','D2','D3','A2','A3','C2','C3','C2','C3','C2','C3','G2','G3',  # bar 3  (Dm | C)
    'D2','D3','D2','D3','A2','A3','A2','A3','E2','E3','E2','E3','A2','A3','E2','E3',  # bar 4  (Dm | Em)
    'D2','D3','D2','D3','D2','D3','A2','A3','C2','C3','C2','C3','C2','C3','G2','G3',  # bar 5  (A')
    'D2','D3','D2','D3','D2','D3','A2','A3','C2','C3','G2','G3','C2','C3','C2','C3',  # bar 6
    'D2','A2','D3','A2','D2','A2','D3','A2','E2','B2','E3','B2','E2','B2','E3','B2',  # bar 7  (walking)
    'D2','D3','A2','A3','D2','D3','A2','A3','A2','A2','E2','E2','A2','A2','A2','A2',  # bar 8  (A(V) into bridge)
    'A#2','A#3','A#2','A#3','F2','F3','A#2','A#3','C3','C2','C3','C2','G2','G3','C3','C2',  # bar 9  (Bb | C pulse)
    'C3','C2','C3','C2','G2','G3','C3','C2','A#2','A#3','A#2','A#3','F2','F3','A#2','A#3',  # bar 10 (C | Bb)
    'F2','F3','F2','F3','C3','C2','F2','F3','A2','A3','A2','A3','E2','E3','A2','A3',        # bar 11 (F | A pull)
    'G2','G3','C3','C2','G2','G3','C2','C3','A2','A3','E2','A2','A2','A3','E2','A2',        # bar 12 (C7 | A -> D)
    'D2','D3','D2','D3','D2','D3','A2','A3','C2','C3','C2','C3','C2','C3','G2','G3',  # bar 13 (Dm | C home)
    'E2','E3','E2','E3','E2','E3','B2','B3','D2','D3','D2','D3','D2','D3','A2','A3',  # bar 14 (Em | D lift  @208)
    'E2','E3','E2','E3','E2','E3','B2','B3','F#2','F#3','B2','B3','F#2','F#3','B2','B3',    # bar 15 (Em | B = V/Em)
    'A2','A3','E2','A2','A2','A3','E2','G2','A2','E2','A2','E2','A2','A2','C#2','D2') # bar 16 (T: A7 -> chromatic C#2->D2 seam)
CH_DRUM = seq(  # ch3 noise -- 0 kick 1 snare 2 hat 3 crash 4 ride; never stops
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 1  (drive)
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 2
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 3
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 4
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 5  (A')
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 6
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 7
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 8
    0,4,1,4,0,0,1,4,0,4,1,4,0,0,1,4,  # bar 9  (B: ride shimmer on the lift)
    0,4,1,4,0,0,1,4,0,4,1,4,0,0,1,4,  # bar 10
    0,4,1,4,0,0,1,4,0,4,1,4,0,0,1,4,  # bar 11
    0,4,1,4,0,0,1,4,0,0,1,1,0,2,1,2,  # bar 12 (mini-fill back to A'')
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 13 (hard drive returns)
    0,2,1,2,0,0,1,2,0,2,1,2,0,0,1,2,  # bar 14 (Em lift)
    0,2,1,2,0,0,1,2,0,1,0,1,0,1,1,1,  # bar 15 (build)
    1,1,0,1,1,1,1,3,1,1,2,1,1,1,1,3)  # bar 16 (T: drum-fill + crash seam -> loop row 0)
```