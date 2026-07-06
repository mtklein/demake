The full composition is verified and complete. All requirements are satisfied: 640 rows / 40 bars, four channels of exactly equal length, all tokens legal and in range, zero voice/guide collisions, the mandatory Jobim one-note passage (bars 17–23), the bVI drift (G#maj7, bars 35–36), the bVII7 (A#7, bar 39), ii-V chains, an inaudible looping seam, and straight-eighth anticipations throughout.

Here is the final composition.

```python
# TOTAL_ROWS = 640
# FORM: 0-127 A | 128-255 A' | 256-383 B (Jobim one-note, harmony walks) | 384-511 A'' | 512-639 C/turnaround+seam
#   Harmony: | Cmaj7 | Cmaj7 | Dm7 | G7(9) | Em7 | A7(b9)->Dm7 | Dm7 G7 | Cmaj7 | (A, A')
#   B walk under one note E5: Cmaj7 | Am7 | Fmaj7 | Dm7 | Em7 | A7 | Dm7 | G7(9)
#   C: Cmaj7(lyd) | Cmaj7 | G#maj7(bVI) | G#maj7 | Em7 | A7(b9) | A#7(bVII7) | Dm7 G7 -> loops mid-phrase into bar1
CH_VOICE = seq(
    # A (1-8)
    "E4 -  -  -  G4 -  -  -  A4 -  -  -  -  -  D5 - ",   # bar 1  Cmaj7  (D5=9th anticip -> bar2)
    "-  -  -  -  C5 -  -  -  -  -  B4 -  -  -  -  - ",   # bar 2  Cmaj7  (resolve down)
    "C5 -  -  -  A4 -  -  -  -  -  -  -  -  -  F4 - ",   # bar 3  Dm7    (F4=3rd anticip -> bar4)
    "-  -  -  -  G4 -  -  -  A4 -  -  -  -  -  -  - ",   # bar 4  G7(9)  (A=9th)
    "G4 -  -  -  -  -  E4 -  -  -  -  -  -  -  B4 - ",   # bar 5  Em7    (B4 anticip -> bar6)
    "-  -  -  -  C5 -  -  -  A4 -  -  -  G4 -  F4 - ",   # bar 6  A7(b9)->Dm7 (chromatic fall, F4 anticip)
    "-  -  -  -  A4 -  -  -  D5 -  -  -  -  -  -  - ",   # bar 7  Dm7 | G7
    "C5 -  -  -  -  -  E4 -  -  -  -  -  -  -  D5 - ",   # bar 8  Cmaj7  (D5 anticip -> A')
    # A' (9-16)
    "-  -  C5 -  -  -  B4 -  -  -  G4 -  -  -  -  - ",   # bar 9  Cmaj7  (gentle fall)
    "E4 -  -  -  -  -  G4 -  A4 -  -  -  -  -  C5 - ",   # bar 10 Cmaj7  (rise, C5 anticip)
    "-  -  -  -  A4 -  -  -  F4 -  -  -  -  -  E4 - ",   # bar 11 Dm7    (E4 anticip)
    "-  -  -  -  D5 -  -  -  -  -  B4 -  -  -  -  - ",   # bar 12 G7(9)  (D=5, B=3)
    "G4 -  -  -  -  -  A4 -  B4 -  -  -  -  -  -  - ",   # bar 13 Em7    (climb)
    "-  -  C5 -  -  -  A4 -  -  -  G4 -  -  -  F4 - ",   # bar 14 A7(b9) (falling, F4 anticip)
    "-  -  -  -  A4 -  -  -  D5 -  -  -  -  -  E5 - ",   # bar 15 Dm7 | G7 (leap up, E5 anticip -> B)
    "-  -  -  -  -  -  -  -  -  -  -  -  -  -  E5 - ",   # bar 16 Cmaj7  (E5 held, anticip into one-note)
    # B (17-24) THE JOBIM MOVE: one note E5 repeated while harmony walks
    "E5 -  -  -  -  -  E5 -  -  -  -  -  E5 -  -  - ",   # bar 17 Cmaj7   E=3
    "E5 -  -  -  -  -  E5 -  -  -  -  -  E5 -  -  - ",   # bar 18 Am7     E=5
    "E5 -  -  -  -  -  E5 -  -  -  -  -  E5 -  -  - ",   # bar 19 Fmaj7   E=7
    "E5 -  -  -  -  -  E5 -  -  -  -  -  E5 -  -  - ",   # bar 20 Dm7     E=9
    "E5 -  -  -  -  -  E5 -  -  -  -  -  E5 -  -  - ",   # bar 21 Em7     E=1
    "E5 -  -  -  -  -  E5 -  -  -  -  -  E5 -  -  - ",   # bar 22 A7      E=5
    "E5 -  -  -  -  -  E5 -  -  -  -  -  E5 -  -  - ",   # bar 23 Dm7     E=9
    "E5 -  -  -  -  -  D5 -  -  -  -  -  -  -  C5 - ",   # bar 24 G7(9)   E->D->C, anticip out
    # A'' (25-32) warmer, lower close
    "-  -  -  -  B4 -  -  -  G4 -  -  -  -  -  A4 - ",   # bar 25 Cmaj7  (A4 anticip)
    "-  -  -  -  G4 -  -  -  E4 -  -  -  -  -  D5 - ",   # bar 26 Cmaj7  (settle low, D5 anticip)
    "-  -  -  -  F4 -  -  -  A4 -  -  -  -  -  -  - ",   # bar 27 Dm7    (F=3, A=5)
    "G4 -  -  -  -  -  D4 -  -  -  -  -  -  -  B4 - ",   # bar 28 G7(9)  (B4 anticip)
    "-  -  -  -  G4 -  -  -  E4 -  -  -  -  -  -  - ",   # bar 29 Em7
    "-  -  C5 -  -  -  A4 -  -  -  G4 -  -  -  F4 - ",   # bar 30 A7(b9) (falling, F4 anticip)
    "-  -  -  -  A4 -  -  -  D5 -  -  -  -  -  -  - ",   # bar 31 Dm7 | G7
    "C5 -  -  -  -  -  E4 -  -  -  -  -  -  -  D5 x ",   # bar 32 Cmaj7  (D5 anticip -> C)
    # C / turnaround & seam (33-40): lydian(in harmony) + bVI + bVII7
    "-  -  -  -  E5 -  -  -  D5 -  -  -  -  -  C5 - ",   # bar 33 Cmaj7 lydian (melody floats 9->8)
    "-  -  -  -  C5 -  -  -  A4 -  -  -  -  -  G4 - ",   # bar 34 Cmaj7  (G4 anticip)
    "-  -  -  -  C5 -  -  -  D5 -  -  -  -  -  -  - ",   # bar 35 G#maj7 bVI drift
    "-  -  -  -  C5 -  -  -  -  -  A4 -  -  -  -  - ",   # bar 36 G#maj7 (settle)
    "G4 -  -  -  -  -  B4 -  -  -  -  -  -  -  C5 - ",   # bar 37 Em7    (rise, C5 anticip)
    "-  -  -  -  A4 -  -  -  G4 -  -  -  -  -  F4 - ",   # bar 38 A7(b9) (fall, F4 anticip)
    "-  -  -  -  A4 -  -  -  G4 -  -  -  -  -  -  - ",   # bar 39 A#7 bVII7 (backdoor color)
    "-  -  -  -  A4 -  -  -  D5 -  -  -  -  -  E4 - ",   # bar 40 Dm7 | G7 -> E4 anticip attacks into bar1 (seam)
)
CH_GUIDE = seq(
    # A (1-8): guide tones (3rd/7th) stabbed in the melodic gaps — interlocks with voice, never stacks
    "-  -  E3 -  -  -  B3 -  -  -  E3 -  -  -  -  - ",   # bar 1  Cmaj7  7=B / 3=E
    "-  -  B3 -  -  -  E3 -  -  -  -  -  -  -  B3 - ",   # bar 2  Cmaj7
    "-  -  F3 -  -  -  C4 -  -  -  F3 -  -  -  -  - ",   # bar 3  Dm7    3=F / 7=C
    "-  -  B3 -  -  -  F3 -  -  -  B3 -  -  -  -  - ",   # bar 4  G7     3=B / 7=F
    "-  -  G3 -  -  -  -  -  -  -  E3 -  G3 -  -  - ",   # bar 5  Em7    3=G
    "-  -  G3 -  -  -  C4 -  -  -  G3 -  -  -  -  - ",   # bar 6  A7(b9) b7=G
    "-  -  F3 -  -  -  C4 -  -  -  F3 -  -  -  -  - ",   # bar 7  Dm7|G7
    "-  -  E3 -  -  -  -  -  -  -  B3 -  E3 -  -  - ",   # bar 8  Cmaj7
    # A' (9-16)
    "-  -  -  -  -  -  -  -  -  -  -  -  E3 -  B3 - ",   # bar 9  Cmaj7
    "-  -  B3 -  -  -  -  -  -  -  E3 -  B3 -  -  - ",   # bar 10 Cmaj7
    "-  -  F3 -  -  -  C4 -  -  -  F3 -  -  -  -  - ",   # bar 11 Dm7
    "-  -  B3 -  -  -  F3 -  -  -  -  -  -  -  B3 - ",   # bar 12 G7
    "-  -  G3 -  -  -  -  -  -  -  E3 -  -  -  G3 - ",   # bar 13 Em7
    "-  -  -  -  G3 -  -  -  -  -  -  -  C4 -  -  - ",   # bar 14 A7(b9)
    "-  -  F3 -  -  -  C4 -  -  -  F3 -  -  -  -  - ",   # bar 15 Dm7|G7
    "-  -  E3 -  -  -  B3 -  -  -  E3 -  -  -  -  - ",   # bar 16 Cmaj7
    # B (17-24): comp spells the walking harmony under the one repeated note
    "-  -  E3 -  -  -  -  -  -  -  B3 -  -  -  E3 - ",   # bar 17 Cmaj7  3=E / 7=B
    "-  -  G3 -  -  -  -  -  -  -  C4 -  -  -  G3 - ",   # bar 18 Am7    7=G / 3=C
    "-  -  E3 -  -  -  -  -  -  -  A3 -  -  -  E3 - ",   # bar 19 Fmaj7  7=E / 3=A
    "-  -  F3 -  -  -  -  -  -  -  C4 -  -  -  F3 - ",   # bar 20 Dm7    3=F / 7=C
    "-  -  G3 -  -  -  -  -  -  -  B3 -  -  -  G3 - ",   # bar 21 Em7    3=G / 5=B
    "-  -  G3 -  -  -  -  -  -  -  C4 -  -  -  G3 - ",   # bar 22 A7     b7=G
    "-  -  F3 -  -  -  -  -  -  -  C4 -  -  -  F3 - ",   # bar 23 Dm7
    "-  -  B3 -  -  -  -  -  -  -  F3 -  B3 -  -  - ",   # bar 24 G7(9)  3=B / 7=F
    # A'' (25-32)
    "-  -  E3 -  -  -  B3 -  -  -  E3 -  -  -  -  - ",   # bar 25 Cmaj7
    "-  -  B3 -  -  -  E3 -  -  -  B3 -  -  -  -  - ",   # bar 26 Cmaj7
    "-  -  F3 -  -  -  C4 -  -  -  F3 -  -  -  -  - ",   # bar 27 Dm7
    "-  -  B3 -  -  -  -  -  -  -  F3 -  B3 -  -  - ",   # bar 28 G7
    "-  -  G3 -  -  -  E3 -  -  -  G3 -  -  -  -  - ",   # bar 29 Em7
    "-  -  -  -  G3 -  -  -  -  -  -  -  C4 -  -  - ",   # bar 30 A7(b9)
    "-  -  F3 -  -  -  C4 -  -  -  F3 -  -  -  -  - ",   # bar 31 Dm7|G7
    "-  -  E3 -  -  -  -  -  -  -  B3 -  E3 -  -  - ",   # bar 32 Cmaj7
    # C / turnaround & seam (33-40)
    "-  -  E3 -  -  -  B3 -  -  -  E3 -  -  -  -  - ",   # bar 33 Cmaj7 (lydian in harmony)
    "-  -  B3 -  -  -  E3 -  -  -  B3 -  -  -  -  - ",   # bar 34 Cmaj7
    "-  -  G3 -  -  -  C4 -  -  -  G3 -  -  -  -  - ",   # bar 35 G#maj7 7=G / 3=C
    "-  -  C4 -  -  -  G3 -  -  -  -  -  -  -  C4 - ",   # bar 36 G#maj7
    "-  -  G3 -  -  -  -  -  -  -  B3 -  G3 -  -  - ",   # bar 37 Em7
    "-  -  G3 -  -  -  C4 -  -  -  G3 -  -  -  -  - ",   # bar 38 A7(b9)
    "-  -  G#3 -  -  -  C4 -  -  -  G#3 -  -  -  -  - ", # bar 39 A#7 (Bb7) b7=Ab=G#
    "-  -  F3 -  -  -  B3 -  -  -  F3 -  -  -  -  - ",   # bar 40 Dm7|G7
)
CH_BASS = seq(
    # A (1-8): root(row0)/fifth(row8), chromatic approach(row14) into next root  (sounds 8vb)
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  -  - ",   # bar 1  C
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  C#3 - ",  # bar 2  C   -> C#3 up to D
    "D3 -  -  -  -  -  -  -  A2 -  -  -  -  -  F#2 - ",  # bar 3  Dm7 -> F#2 up to G
    "G2 -  -  -  -  -  -  -  D3 -  -  -  -  -  -  - ",   # bar 4  G7
    "E3 -  -  -  -  -  -  -  B2 -  -  -  -  -  -  - ",   # bar 5  Em7
    "A2 -  -  -  -  -  -  -  E3 -  -  -  -  -  C#3 - ",  # bar 6  A7  -> C#3 up to D
    "D3 -  -  -  -  -  -  -  G2 -  -  -  -  -  B2 - ",   # bar 7  Dm7|G7 (mid=G root of G7) -> B2 up to C
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  -  - ",   # bar 8  C
    # A' (9-16)
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  -  - ",   # bar 9  C
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  C#3 - ",  # bar 10 C -> D
    "D3 -  -  -  -  -  -  -  A2 -  -  -  -  -  F#2 - ",  # bar 11 Dm7 -> G
    "G2 -  -  -  -  -  -  -  D3 -  -  -  -  -  -  - ",   # bar 12 G7
    "E3 -  -  -  -  -  -  -  B2 -  -  -  -  -  -  - ",   # bar 13 Em7
    "A2 -  -  -  -  -  -  -  E3 -  -  -  -  -  C#3 - ",  # bar 14 A7 -> D
    "D3 -  -  -  -  -  -  -  G2 -  -  -  -  -  B2 - ",   # bar 15 Dm7|G7 (mid=G) -> C
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  -  - ",   # bar 16 C
    # B (17-24) WALK — bass carries the movement under the one held note
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  -  - ",   # bar 17 C   root/5
    "A2 -  -  -  -  -  -  -  E3 -  -  -  -  -  -  - ",   # bar 18 Am7 A/5
    "F2 -  -  -  -  -  -  -  C3 -  -  -  -  -  -  - ",   # bar 19 Fmaj7 F/5
    "D3 -  -  -  -  -  -  -  A2 -  -  -  -  -  -  - ",   # bar 20 Dm7 D/5
    "E3 -  -  -  -  -  -  -  B2 -  -  -  -  -  -  - ",   # bar 21 Em7 E/5
    "A2 -  -  -  -  -  -  -  E3 -  -  -  -  -  C#3 - ",  # bar 22 A7  -> C#3 up to D
    "D3 -  -  -  -  -  -  -  A2 -  -  -  -  -  -  - ",   # bar 23 Dm7 D/5
    "G2 -  -  -  -  -  -  -  D3 -  -  -  -  -  B2 - ",   # bar 24 G7  -> B2 up to C
    # A'' (25-32)
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  -  - ",   # bar 25 C
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  C#3 - ",  # bar 26 C -> D
    "D3 -  -  -  -  -  -  -  A2 -  -  -  -  -  F#2 - ",  # bar 27 Dm7 -> G
    "G2 -  -  -  -  -  -  -  D3 -  -  -  -  -  -  - ",   # bar 28 G7
    "E3 -  -  -  -  -  -  -  B2 -  -  -  -  -  -  - ",   # bar 29 Em7
    "A2 -  -  -  -  -  -  -  E3 -  -  -  -  -  C#3 - ",  # bar 30 A7 -> D
    "D3 -  -  -  -  -  -  -  G2 -  -  -  -  -  B2 - ",   # bar 31 Dm7|G7 (mid=G) -> C
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  A2 - ",   # bar 32 C -> A2 down to G# (bVI)
    # C / turn (33-40)
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  A2 - ",   # bar 33 C -> A2 down to G#
    "C3 -  -  -  -  -  -  -  G2 -  -  -  -  -  A2 - ",   # bar 34 C -> A2 down to G#
    "G#2 -  -  -  -  -  -  -  D#3 -  -  -  -  -  -  - ", # bar 35 G#maj7 (bVI) root/5
    "G#2 -  -  -  -  -  -  -  D#3 -  -  -  -  -  F2 - ", # bar 36 G#maj7 -> F2 down to E
    "E3 -  -  -  -  -  -  -  B2 -  -  -  -  -  -  - ",   # bar 37 Em7
    "A2 -  -  -  -  -  -  -  E3 -  -  -  -  -  -  - ",   # bar 38 A7
    "A#2 -  -  -  -  -  -  -  F3 -  -  -  -  -  C#3 - ", # bar 39 A#7 (bVII7) root/5 -> C#3 up to D
    "D3 -  -  -  -  -  -  -  G2 -  -  -  -  -  B2 - ",   # bar 40 Dm7|G7 (mid=G) -> B2 up to C == LOOP
)
CH_KIT = rep([
    "1 - 2 - 2 - 1 - 2 - 2 - 1 - 2 -",   # bar A: shaker eighths + clave rim-clicks (1) on beat1, and-of-2, beat4
    "0 - 2 - 1 - 2 - 1 - 2 - 2 - 2 -",   # bar B: soft kick(0) on beat1, clave clicks on beat2 & beat3, shaker fills
], 20)
```