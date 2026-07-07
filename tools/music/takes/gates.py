Commission delivered: the grove-gates finale battle theme. The brief was "wild,
like B.O.B" -- so this is the GRAMMAR of that idiom at GBA-PSG scale (breakneck
double-time grid, chopped jungle break, rolling funk bass ostinato, rap-cadence
lead, gospel answer walls, one true shred break) with every note original.
Nothing quoted: not the hook, not the bass figure, not the changes. The two
laws held at maximum: it locks (0 strong-beat dissonance, 0 crossing, 0 warns
against the real linter) and the noise channel drives from UNDER the squares
(no ride at all -- the ride-fatigue lesson; opens are accents, rolls are
punctuation, the chop bars are the hype man answering the MC).

```python
# TOTAL_ROWS = 800
# FORM: 0=DROP-IN(chopped break alone; bass raps the hook rhythm in b2)
#       32=VERSE1(rap-cadence hook, choir answer-melismas in the gaps)
#       160=WALL1(gospel dyads pump Bb-C-Dm in parallel 4ths)
#       224=VERSE2(hook stuttered 16th-triple, bass roll doubles)
#       352=WALL2(climb widens Gm-C-Dm; bar 4 = octave-unison riser)
#       416=GATE(full-band tresillo stutter-lock, noise chokes with it)
#       448=SHRED(row-rate melodic-minor rip, Dm/A sweep arps to E6)
#       512=BASS BREAK(the wave grabs the hook solo; square chips answer)
#       544=HALF-TIME DROP(anthem slams Dm-Bb-C-A, choir organ-pumps)
#       608=BUILD(pulse climb + kick-anchored accelerating snare roll)
#       640=OUT(hook over breathing choir pads, 8va peak, opens splash)
#       768=TURN(unison slam into an 8-row VOID, fall, roll-up -> loop 0)
# Grid: speed 4, 16 rows/bar, 4 rows/beat -- a 225 BPM sixteenth grid that
# reads as a 112.5 half-time frame under the double-time break (the fastest
# engine rate in the album; BOSS/TADPOLE sit at speed 5). Key: D minor with
# dorian B-natural sass; shred = D melodic minor + Dm/A sweeps; walls climb
# bVI-bVII-i. Loop -> row 0: the roll-up slam-cuts to the dry chop, DJ style,
# and the seam bass hangs on C#3 that only resolves when the D returns in b2.
# ch0 sq1 25% duty (the MC: staccato x-gated rap cells; walls' top voice)
# ch1 sq2 50% duty, decay step 2 (the choir: answer figures, parallel walls,
#     breathing pads; always at-or-below ch0, all strong co-attacks consonant)
# ch2 wave (write-what-you-hear): x-choked funk ostinato, never above A#3;
#     grabs the hook verbatim for the bass break
# ch3 noise 0=kick 1=snare 2=hat 3=open: two-step core (K on 1, S on 4/12,
#     displaced K on row 9), '.' lets tails ring, 'x' chokes only in the chop
#     bars, the gate, and the void. No ride anywhere.

CH_MC = (                            # ch0 sq1 25% -- THE MC
    # DROP-IN (bars 0-1): the break speaks first
    seq('x   x  x  x   x  x  x  x   x  x  x  x   x  x  x  x')     # bar 0  drums alone
  + seq('x   x  x  x   x  x  x  x   x  x  x  x   x  x  x  x')     # bar 1  bass raps the flow
    # VERSE 1 (bars 2-9): hook H circles D minor pentatonic, choir answers
  + seq('D4  D4 x  D4  x  F4 D4 x   G4 x  F4 D4  x  C4 D4 x')     # bar 2  H
  + seq('F4  x  F4 G4  x  A4 A4 x   x  x  x  x   x  x  x  x')     # bar 3  call up; choir answers
  + seq('D4  D4 x  D4  x  F4 D4 x   G4 x  F4 D4  x  C4 D4 x')     # bar 4  H
  + seq('C5  x  C5 A4  x  G4 A4 x   x  x  x  x   x  x  x  x')     # bar 5  call down; choir answers
  + seq('A4  A4 x  A4  x  C5 A4 x   D5 x  C5 A4  x  G4 A4 x')     # bar 6  S: hook up a 5th
  + seq('G4  x  G4 F4  x  D4 C4 x   x  x  x  x   x  x  x  x')     # bar 7  tumble; choir answers
  + seq('D4  D4 x  D4  x  F4 D4 x   G4 x  F4 D4  x  C4 D4 x')     # bar 8  H
  + seq('x   x  x  x   x  x  x  x   x  x  x  x   x  x  x  x')     # bar 9  MC breathes; break chops
    # WALL 1 (bars 10-13): gospel dyads, ch1 a P4 below throughout
  + seq('A#4 .  .  .   A#4 . C5 .   A#4 . .  .   A#4 . C5 .')     # bar 10 Bb wall
  + seq('C5  .  .  .   C5 .  D5 .   C5 .  .  .   C5 .  D5 .')     # bar 11 C wall
  + seq('D5  .  .  .   D5 .  F5 .   D5 .  .  .   D5 .  F5 .')     # bar 12 Dm wall
  + seq('D5  .  .  .   .  .  .  .   A4 .  C5 .   D5 .  .  .')     # bar 13 turn + pickup
    # VERSE 2 (bars 14-21): hook stuttered, register pushes
  + seq('D4  D4 D4 x   D4 x  F4 x   G4 G4 x  F4  D4 x  C4 x')     # bar 14 H2 stutter
  + seq('F4  x  F4 G4  x  A4 A4 x   x  x  x  x   x  x  x  x')     # bar 15 call up
  + seq('D4  D4 D4 x   D4 x  F4 x   G4 G4 x  F4  D4 x  C4 x')     # bar 16 H2
  + seq('D5  x  C5 A4  x  C5 A4 x   x  x  x  x   x  x  x  x')     # bar 17 peak call
  + seq('A4  A4 A4 x   A4 x  C5 x   D5 D5 x  C5  A4 x  G4 x')     # bar 18 S2 stutter
  + seq('A#4 x  A#4 A4 x  G4 A4 x   x  x  x  x   x  x  x  x')     # bar 19 Bb-A dorian sass
  + seq('D4  D4 D4 x   D4 x  F4 x   G4 G4 x  F4  D4 x  C4 x')     # bar 20 H2
  + seq('x   x  x  x   x  x  x  x   x  x  x  x   x  x  x  x')     # bar 21 breath; break chops
    # WALL 2 (bars 22-25): wider voicings, then the octave riser
  + seq('A#4 .  .  .   A#4 . D5 .   A#4 . .  .   A#4 . D5 .')     # bar 22 Gm wall
  + seq('C5  .  .  .   C5 .  E5 .   C5 .  .  .   C5 .  E5 .')     # bar 23 C wall (dorian E)
  + seq('D5  .  .  .   D5 .  F5 .   D5 .  .  .   D5 .  F5 .')     # bar 24 Dm wall
  + seq('D5  D5 x  D5  x  F5 x  G5  x  A5 x  x   A5 x  x  x')     # bar 25 riser (8ves w/ ch1)
    # GATE (bars 26-27): everything chops the same tresillo, then walks down
  + seq('D5  x  x  D5  x  x  D5 x   D5 x  x  D5  x  x  D5 x')     # bar 26 stutter-lock
  + seq('C5  x  x  C5  x  x  C5 x   A#4 x x  A4  x  x  G4 x')     # bar 27 gate walks down
    # SHRED (bars 28-31): every row attacks; D melodic minor, then sweeps
  + seq('A4  B4 C#5 D5 E5 F5 E5 D5  C#5 D5 E5 F5 G5 A5 G5 F5')    # bar 28 rip up
  + seq('E5  F5 G5 A5  A#5 A5 G5 F5 E5 F5 E5 D5  C#5 D5 E5 C#5')  # bar 29 summit turn
  + seq('D5  F5 A5 D6  A5 F5 D5 F5  A5 D6 A5 F5  D5 F5 A5 D6')    # bar 30 Dm sweep
  + seq('C#6 A5 E5 A5  C#6 E6 C#6 A5 E5 G5 F5 E5 D5 C#5 D5 x')    # bar 31 A7 sweep, snap home
    # BASS BREAK (bars 32-33): the wave has the mic
  + seq('x   x  x  x   x  x  x  x   x  x  x  x   x  x  x  x')     # bar 32 all yours, bass
  + seq('x   x  x  x   x  x  x  x   A4 x  x  C5  x  D5 x  x')     # bar 33 chips ride back in
    # HALF-TIME DROP (bars 34-37): the detonation, huge and slow
  + seq('D5  .  .  .   .  .  .  .   .  .  .  .   A#4 . .  .')     # bar 34 Dm slam
  + seq('C5  .  .  .   .  .  .  .   A#4 . .  .   A4 .  .  .')     # bar 35 Bb lean
  + seq('G4  .  .  .   .  .  .  .   .  .  .  .   E4 .  .  .')     # bar 36 C settles
  + seq('E4  .  .  .   A4 .  .  .   B4 .  .  .   C#5 . .  .')     # bar 37 V climbs out
    # BUILD (bars 38-39): pulse ladder + the roll
  + seq('D4  x  D4 x   D4 x  D4 x   F4 x  F4 x   F4 x  F4 x')     # bar 38 8th pulses
  + seq('A4  x  A4 x   A4 A4 A4 A4  D5 D5 D5 D5  D5 D5 D5 D5')    # bar 39 16ths take over
    # OUT (bars 40-47): hook over choir pads, then the 8va lap
  + seq('D4  D4 x  D4  x  F4 D4 x   G4 x  F4 D4  x  C4 D4 x')     # bar 40 H over pads
  + seq('F4  x  F4 G4  x  A4 A4 x   x  x  A4 x   C5 x  D5 x')     # bar 41 self-answer
  + seq('D4  D4 x  D4  x  F4 D4 x   G4 x  F4 D4  x  C4 D4 x')     # bar 42 H
  + seq('C5  x  C5 A4  x  G4 A4 x   x  x  G4 x   F4 x  D4 x')     # bar 43 fold back
  + seq('A4  A4 x  A4  x  C5 A4 x   D5 x  C5 A4  x  G4 A4 x')     # bar 44 S
  + seq('G4  x  G4 F4  x  D4 C4 x   x  x  D4 x   F4 x  G4 x')     # bar 45 climb turn
  + seq('D5  D5 x  D5  x  F5 D5 x   G5 x  F5 D5  x  C5 D5 x')     # bar 46 H an octave up
  + seq('F5  x  F5 G5  x  A5 A5 x   G5 x  F5 x   D5 x  C5 x')     # bar 47 peak lap
    # TURN (bars 48-49): slam, void, fall; the roll-up owns the seam
  + seq('D5  x  x  x   x  x  x  x   A4 x  G4 x   F4 x  D4 x')     # bar 48 slam/VOID/fall
  + seq('x   x  x  x   x  x  x  x   x  x  x  x   x  x  x  x')     # bar 49 roll-up -> loop
)

CH_CHOIR = (                         # ch1 sq2 50% -- THE CHOIR
    # DROP-IN
    seq('x  x  x  x   x  x  x  x    x  x  x  x    x  x  x  x')    # bar 0
  + seq('x  x  x  x   x  x  x  x    x  x  x  x    x  x  x  x')    # bar 1
    # VERSE 1: chips in the hook's rests, melismas in the answer bars
  + seq('x  x  x  x   x  x  x  A3   x  x  x  x    x  x  x  x')    # bar 2  "uh!"
  + seq('x  x  x  x   x  x  x  x    F4 x  G4 A4   x  G4 F4 x')    # bar 3  answer rise
  + seq('x  x  x  x   x  x  x  A3   x  x  x  x    x  x  x  A3')   # bar 4  "uh! ...uh!"
  + seq('x  x  x  x   x  x  x  x    D4 x  C4 A3   x  C4 D4 x')    # bar 5  answer fall
  + seq('x  x  x  x   x  x  x  E4   .  .  .  .    G4 .  .  x')    # bar 6  held 9th color
  + seq('x  x  x  x   x  x  x  x    F4 x  G4 A4   x  C5 A4 x')    # bar 7  answer lifts
  + seq('x  x  x  x   x  x  x  A3   x  x  x  x    x  x  x  A3')   # bar 8
  + seq('D4 x  C4 x   A3 x  G3 x    A3 x  x  x    x  x  x  x')    # bar 9  choir tumbles out
    # WALL 1: parallel a P4 under the MC
  + seq('F4 .  .  .   F4 .  G4 .    F4 .  .  .    F4 .  G4 .')    # bar 10
  + seq('G4 .  .  .   G4 .  A4 .    G4 .  .  .    G4 .  A4 .')    # bar 11
  + seq('A4 .  .  .   A4 .  A4 .    A4 .  .  .    A4 .  A4 .')    # bar 12
  + seq('F4 .  .  .   .  .  .  .    F4 .  G4 .    A4 .  .  .')    # bar 13
    # VERSE 2: chips + answers, one more voice awake
  + seq('x  x  x  x   x  x  x  A3   x  x  x  x    x  x  A3 x')    # bar 14
  + seq('x  x  x  x   x  x  x  x    F4 x  G4 A4   x  G4 F4 x')    # bar 15
  + seq('x  x  x  x   x  x  x  A3   x  x  x  x    x  x  A3 x')    # bar 16
  + seq('x  x  x  x   x  x  x  x    F4 x  E4 D4   x  E4 F4 x')    # bar 17
  + seq('x  x  x  x   x  x  x  E4   .  .  .  .    .  G4 .  x')    # bar 18
  + seq('x  x  x  x   x  x  x  x    D4 x  F4 G4   x  F4 D4 x')    # bar 19
  + seq('x  x  x  x   x  x  x  A3   x  x  x  x    x  x  A3 x')    # bar 20
  + seq('F4 x  D4 x   C4 x  A3 x    G3 x  x  x    A3 x  x  x')    # bar 21 bigger tumble
    # WALL 2: 3rds under, then the riser in octaves
  + seq('G4 .  .  .   G4 .  A#4 .   G4 .  .  .    G4 .  A#4 .')   # bar 22
  + seq('G4 .  .  .   G4 .  C5 .    G4 .  .  .    G4 .  C5 .')    # bar 23
  + seq('A4 .  .  .   A4 .  C5 .    A4 .  .  .    A4 .  C5 .')    # bar 24
  + seq('D4 D4 x  D4  x  F4 x  G4   x  A4 x  x    A4 x  x  x')    # bar 25 riser 8vb
    # GATE: locked an octave below
  + seq('D4 x  x  D4  x  x  D4 x    D4 x  x  D4   x  x  D4 x')    # bar 26
  + seq('C4 x  x  C4  x  x  C4 x    A#3 x x  A3   x  x  G3 x')    # bar 27
    # SHRED: one anchor tone per bar, fading under the fury
  + seq('D4 .  .  .   .  .  .  .    .  .  .  .    .  .  .  .')    # bar 28
  + seq('E4 .  .  .   .  .  .  .    .  .  .  .    .  .  .  .')    # bar 29
  + seq('A4 .  .  .   .  .  .  .    .  .  .  .    .  .  .  .')    # bar 30
  + seq('A4 .  .  .   .  .  .  .    .  .  .  .    .  .  x  x')    # bar 31
    # BASS BREAK: silent, then a parallel rise-in with the chips
  + seq('x  x  x  x   x  x  x  x    x  x  x  x    x  x  x  x')    # bar 32
  + seq('x  x  x  x   x  x  x  x    F4 x  x  G4   x  A4 x  x')    # bar 33
    # HALF-TIME DROP: organ-pump quarters, all consonant under the anthem
  + seq('F4 .  .  .   F4 .  .  .    F4 .  .  .    F4 .  .  .')    # bar 34
  + seq('F4 .  .  .   F4 .  .  .    F4 .  .  .    F4 .  .  .')    # bar 35
  + seq('C4 .  .  .   C4 .  .  .    C4 .  .  .    C4 .  .  .')    # bar 36
  + seq('C#4 . .  .   E4 .  .  .    E4 .  .  .    E4 .  .  .')    # bar 37
    # BUILD: off-beat interlock, then the octave pump
  + seq('x  D4 x  D4  x  D4 x  D4   x  D4 x  D4   x  D4 x  D4')   # bar 38
  + seq('A3 .  .  .   A3 .  .  .    A3 .  .  .    A3 A3 A3 A3')   # bar 39
    # OUT: breathing pads (re-attack row 9, always in the MC's rest)
  + seq('A3 .  .  .   .  .  .  .    .  A3 .  .    .  .  .  .')    # bar 40
  + seq('A3 .  .  .   .  .  .  .    .  A3 .  .    .  .  .  .')    # bar 41
  + seq('A3 .  .  .   .  .  .  .    .  A3 .  .    .  .  .  .')    # bar 42
  + seq('G3 .  .  .   .  .  .  .    .  G3 .  .    .  .  .  .')    # bar 43
  + seq('D4 .  .  .   .  .  .  .    .  D4 .  .    .  .  .  .')    # bar 44
  + seq('G3 .  .  .   .  .  .  .    .  G3 .  .    .  .  .  .')    # bar 45
  + seq('D4 .  .  .   .  .  .  .    .  D4 .  .    .  .  .  .')    # bar 46
  + seq('F4 .  .  .   .  .  .  .    .  F4 .  .    .  .  .  .')    # bar 47
    # TURN: one octave under the slam, then out
  + seq('D4 x  x  x   x  x  x  x    x  x  x  x    x  x  x  x')    # bar 48
  + seq('x  x  x  x   x  x  x  x    x  x  x  x    x  x  x  x')    # bar 49
)

CH_SUB = (                           # ch2 wave -- THE SUB (funk ostinato)
    # DROP-IN: silent bar, then the bass raps the hook's exact rhythm
    seq('x   x  x  x    x  x  x  x    x   x  x  x    x   x   x  x')   # bar 0
  + seq('D2  D2 x  D2   x  D2 D2 x    A#2 x  C3 C3   x   C#3 D3 x')   # bar 1  flow planted
    # VERSE 1: BA/BB cell -- octave pops, bVII-bVI walk, chromatic snap
  + seq('D2  x  x  D3   x  x  D2 x    D3  x  D2 x    A2  x   C3 x')   # bar 2  BA
  + seq('D2  x  x  D3   x  x  D2 x    C3  x  A#2 x   C3  x   C#3 x')  # bar 3  BB
  + seq('D2  x  x  D3   x  x  D2 x    D3  x  D2 x    A2  x   C3 x')   # bar 4  BA
  + seq('D2  x  x  D3   x  x  D2 x    C3  x  A#2 x   C3  x   C#3 x')  # bar 5  BB
  + seq('D2  x  x  D3   x  x  D2 x    D3  x  D2 x    A2  x   C3 x')   # bar 6  BA
  + seq('D2  x  x  D3   x  x  D2 x    C3  x  A#2 x   C3  x   C#3 x')  # bar 7  BB
  + seq('D2  x  x  D3   x  x  D2 x    D3  x  D2 x    A2  x   C3 x')   # bar 8  BA
  + seq('D2  x  D2 x    D3 x  D2 x    C3  x  C3 x    A2  x   A2 x')   # bar 9  walks to Bb
    # WALL 1: pumping 8ths, octave pops on the roots
  + seq('A#2 x  A#2 x   A#2 x A#3 x   A#2 x  A#2 x   A#2 x  A#3 x')   # bar 10
  + seq('C3  x  C3 x    C3 x  C2 x    C3  x  C3 x    C3  x  C2 x')    # bar 11
  + seq('D3  x  D3 x    D3 x  D2 x    D3  x  D3 x    D3  x  D2 x')    # bar 12
  + seq('D3  x  D2 x    D3 x  D2 x    F2  x  G2 x    A2  x  C3 x')    # bar 13
    # VERSE 2: the roll doubles (B2A/B2B)
  + seq('D2  D3 x  D2   D3 x  D2 D3   x   D2 x  D3   D2  x  A2 C3')   # bar 14 B2A
  + seq('D2  D3 x  D2   D3 x  D2 x    C3  x  C3 A#2  x   A#2 C3 C#3') # bar 15 B2B
  + seq('D2  D3 x  D2   D3 x  D2 D3   x   D2 x  D3   D2  x  A2 C3')   # bar 16 B2A
  + seq('D2  D3 x  D2   D3 x  D2 x    C3  x  C3 A#2  x   A#2 C3 C#3') # bar 17 B2B
  + seq('D2  D3 x  D2   D3 x  D2 D3   x   D2 x  D3   D2  x  A2 C3')   # bar 18 B2A
  + seq('D2  D3 x  D2   D3 x  D2 x    C3  x  C3 A#2  x   A#2 C3 C#3') # bar 19 B2B
  + seq('D2  D3 x  D2   D3 x  D2 D3   x   D2 x  D3   D2  x  A2 C3')   # bar 20 B2A
  + seq('D2  x  D2 x    D3 x  C3 x    A#2 x  A2 x    G#2 x  G#2 x')   # bar 21 chromatic to G
    # WALL 2
  + seq('G2  x  G2 x    G2 x  G3 x    G2  x  G2 x    G2  x  G3 x')    # bar 22
  + seq('C3  x  C3 x    C3 x  C2 x    C3  x  C3 x    C3  x  C2 x')    # bar 23
  + seq('D3  x  D3 x    D3 x  D2 x    D3  x  D3 x    D3  x  D2 x')    # bar 24
  + seq('D2  D2 D3 D3   D2 D2 D3 D3   D2  D3 D2 D3   D3  D3 D3 D3')   # bar 25 riser roll
    # GATE: choked with the band
  + seq('D2  x  x  D2   x  x  D2 x    D2  x  x  D2   x   x  D2 x')    # bar 26
  + seq('C2  x  x  C2   x  x  C2 x    A#2 x  x  A2   x   x  G2 x')    # bar 27
    # SHRED: pumping pedal anchors the fury
  + seq('D2  x  D2 x    D2 x  D2 x    D2  x  D2 x    D2  x  D2 x')    # bar 28
  + seq('C2  x  C2 x    C2 x  C2 x    C2  x  C2 x    C2  x  C2 x')    # bar 29
  + seq('D2  x  D2 x    D2 x  D2 x    F2  x  F2 x    F2  x  F2 x')    # bar 30
  + seq('A2  x  A2 x    A2 x  A2 x    A2  x  G2 x    F2  x  E2 x')    # bar 31 walkdown
    # BASS BREAK: the hook, verbatim, in the bass's own octave
  + seq('D3  D3 x  D3   x  F3 D3 x    G3  x  F3 D3   x   C3 D3 x')    # bar 32 the hook itself
  + seq('F3  x  F3 G3   x  A3 A3 x    A2  x  A#2 x   C3  x  C#3 x')   # bar 33 answer + walk up
    # HALF-TIME DROP: slams, laid-back pocket pickups after the snare
  + seq('D2  .  .  .    .  .  .  .    x   x  D2 x    D2  D3 x  x')    # bar 34
  + seq('A#2 .  .  .    .  .  .  .    x   x  A#2 x   A#2 C3 x  x')    # bar 35
  + seq('C3  .  .  .    .  .  .  .    x   x  C3 x    C2  C3 x  x')    # bar 36
  + seq('A2  .  .  .    .  .  .  .    A2  x  A2 A2   x   x  C#3 x')   # bar 37
    # BUILD
  + seq('D2  x  D2 x    D2 x  D2 x    D2  D2 x  D2   D2  x  D2 D2')   # bar 38
  + seq('D2  D2 D2 x    D2 D2 D2 x    D2  D2 D3 D3   D3  D3 D3 D3')   # bar 39
    # OUT: best of both grooves under the hook
  + seq('D2  x  x  D3   x  x  D2 x    D3  x  D2 x    A2  x  C3 x')    # bar 40 BA
  + seq('D2  x  x  D3   x  x  D2 x    C3  x  A#2 x   C3  x  C#3 x')   # bar 41 BB
  + seq('D2  D3 x  D2   D3 x  D2 D3   x   D2 x  D3   D2  x  A2 C3')   # bar 42 B2A
  + seq('D2  x  x  D3   x  x  D2 x    C3  x  A#2 x   C3  x  C#3 x')   # bar 43 BB
  + seq('D2  x  x  D3   x  x  D2 x    D3  x  D2 x    A2  x  C3 x')    # bar 44 BA
  + seq('D2  D3 x  D2   D3 x  D2 x    C3  x  C3 A#2  x   A#2 C3 C#3') # bar 45 B2B
  + seq('D2  D3 x  D2   D3 x  D2 D3   x   D2 x  D3   D2  x  A2 C3')   # bar 46 B2A
  + seq('D2  D3 x  D2   D3 x  D2 x    C3  x  A#2 A#2 x   C3 C#3 x')   # bar 47 launch
    # TURN: slam, void, i-bVII-bVI-V fall, then the final climb hangs on C#
  + seq('D2  x  x  x    x  x  x  x    D2  x  C3 x    A#2 x  A2 x')    # bar 48
  + seq('G2  x  G2 x    A2 x  A2 x    A#2 x  A#2 x   C3  x  C#3 x')   # bar 49 -> loop
)

CH_BREAK = (                         # ch3 noise -- THE BREAK (no ride anywhere)
    # DROP-IN: the chopped statement (also the loop landing), then two-step
    seq('0 x 0 x  1 x x 0  x 0 2 x  1 x 1 2')    # bar 0  chop: sliced cuts
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . 3 .')    # bar 1  two-step, open flags the drop
    # VERSE 1: two-step core / ghost variant
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 2  TS
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 3  TSg (double kick, ghost snare)
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 4
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 5
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 6
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 7
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 8
  + seq('0 0 x 1  x x 0 x  1 1 x 0  x 1 1 1')    # bar 9  the hype man chops
    # WALL 1: opens land with the choir pushes
  + seq('0 . 2 .  1 . 3 .  . 0 2 .  1 . 3 .')    # bar 10
  + seq('0 . 2 .  1 . 3 .  . 0 2 .  1 . 3 .')    # bar 11
  + seq('0 . 2 .  1 . 3 .  . 0 2 .  1 . 3 .')    # bar 12
  + seq('0 . 2 .  1 . 3 .  1 . 1 .  1 1 1 1')    # bar 13 roll into verse 2
    # VERSE 2: variant leads
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 14
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 15
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 16
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 17
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 18
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 19
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 20
  + seq('0 0 x 1  x x 0 0  x 1 1 x  0 x 1 1')    # bar 21 second chop feature
    # WALL 2
  + seq('0 . 2 .  1 . 3 .  . 0 2 .  1 . 3 .')    # bar 22
  + seq('0 . 2 .  1 . 3 .  . 0 2 .  1 . 3 .')    # bar 23
  + seq('0 . 2 .  1 . 3 .  . 0 2 .  1 . 3 .')    # bar 24
  + seq('0 . 2 .  1 . 2 .  1 . 1 .  1 1 1 1')    # bar 25 riser roll
    # GATE: kick locks the tresillo, chokes between; cracks appear late
  + seq('0 x x 0  x x 0 x  0 x x 0  x x 0 x')    # bar 26
  + seq('0 x x 0  x x 0 x  0 x x 0  1 x 0 1')    # bar 27
    # SHRED: skeleton only -- the lead owns the surface
  + seq('0 . . .  1 . . .  0 0 . .  1 . . .')    # bar 28
  + seq('0 . . .  1 . . .  0 0 . .  1 . . .')    # bar 29
  + seq('0 . . .  1 . . .  0 0 . .  1 . . .')    # bar 30
  + seq('0 . . .  1 . . .  0 . 1 .  1 1 1 3')    # bar 31 roll + open crash out
    # BASS BREAK: dry and tight for the star
  + seq('0 . 2 .  1 . . 2  . 0 2 .  1 . . 2')    # bar 32
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 1 1 1')    # bar 33 roll into the drop
    # HALF-TIME: vast
  + seq('0 . . .  . . 2 .  1 . . .  . . 2 .')    # bar 34
  + seq('0 . . .  . . 2 .  1 . . .  . . 2 .')    # bar 35
  + seq('0 . . .  . . 2 .  1 . . .  . . 2 .')    # bar 36
  + seq('0 . . .  . . 2 .  1 . . .  1 1 1 1')    # bar 37 roll into the build
    # BUILD: kick-anchored accelerating roll
  + seq('0 . 2 .  1 . 2 .  0 . 1 .  1 . 1 .')    # bar 38
  + seq('0 1 . 1  0 1 1 .  0 1 1 1  1 1 1 1')    # bar 39
    # OUT: opens splash on the off-8ths, ghosts under the answers
  + seq('0 . 3 .  1 . . 2  . 0 3 .  1 . . 2')    # bar 40 OT
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 41
  + seq('0 . 3 .  1 . . 2  . 0 3 .  1 . . 2')    # bar 42
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 43
  + seq('0 . 3 .  1 . . 2  . 0 3 .  1 . . 2')    # bar 44
  + seq('0 . 2 .  1 . . 0  . 0 2 .  1 . 1 .')    # bar 45
  + seq('0 . 3 .  1 . . 2  . 0 3 .  1 . . 2')    # bar 46
  + seq('0 . 3 .  1 . . 2  . 0 . 1  1 1 1 1')    # bar 47 roll into the slam
    # TURN: open crash, VOID, snares under the fall, the roll-up seam
  + seq('3 x x x  x x x x  1 . 1 .  1 . 1 .')    # bar 48
  + seq('1 . 1 .  1 1 1 .  1 1 1 1  1 1 1 1')    # bar 49 -> cut to the dry chop
)
```
