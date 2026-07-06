All songs lint clean (TADPOLE's high numbers are the intentional surf-echo effect, explicitly waived via `ECHO_SONGS`). The enriched PRELUDE is done, integrated, and compiled into the ROM assets.

Below is the enriched track in the requested output format.

```python
# TOTAL_ROWS = 288
# FORM: 0=A(arps C-C6-Am-Am) | 64=A'(F-F-G-G, melody+countermelody DUET enters)
#       128=B(bVI-bVII lift Ab-Bb, soaring melody + weaving counter)
#       192=C(home stretch F-Dm-G-G7) | 256=TAG(C arrival, sus C5->B4 leading-tone cadence -> loop row 0)
# Grid: speed 9, 16 rows/bar, 2 rows/note. ch0 sq1(50%) melody; ch1 sq2(25%) countermelody
#       (always below ch0); ch2 wave arpeggio bed (sounds 8vb); ch3 noise SILENT.
# Verified: strong-beat both-attack dissonance 0/30, voice-crossing 0/164,
#           all weak-beat suspensions resolve by step. Loops row 287 -> row 0.

CH_MELODY = seq(                                 # ch0 — arc preserved note-for-note
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 1  (arps alone)
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 2
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 3
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 4
    'A5','-','-','-','G5','-','F5','-','A5','-','-','-','-','-','x','-', # bar 5  (F: melody enters)
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 6
    'B5','-','-','-','A5','-','G5','-','B5','-','-','-','-','-','x','-', # bar 7  (G)
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 8
    'C6','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',  # bar 9  (Ab: crest)
    'A#5','-','-','-','G#5','-','-','-','D#5','-','-','-','-','-','x','-', # bar 10 (Ab)
    'D6','-','-','-','C6','-','A#5','-','F5','-','-','-','-','-','-','-', # bar 11 (Bb lift, peak D6)
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 12
    'C6','-','-','-','A5','-','G5','-','F5','-','-','-','-','-','x','-', # bar 13 (F: home stretch)
    'F5','-','G5','-','A5','-','-','-','D5','-','-','-','-','-','x','-', # bar 14 (Dm)
    'B5','-','-','-','D6','-','-','-','G5','-','-','-','-','-','-','-', # bar 15 (G)
    'G5','-','F5','-','D5','-','B4','-','D5','-','-','-','-','-','-','-', # bar 16 (G7)
    'C6','-','-','-','-','-','-','-','G5','-','-','-','E5','-','-','-', # bar 17 TAG (C arrival -> held E5)
    'E5','-','-','-','-','-','-','-','D5','-','-','-','-','-','-','-') # bar 18 TAG (E5 suspends -> D5)

CH_COUNTER = seq(                                # ch1 — the true countermelody (always below ch0)
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 1  (pristine open)
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 2
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 3
    'x','-','-','-','-','-','-','-','-','-','-','-','-','-','-','-',   # bar 4
    'F4','-','-','-','G4','-','A4','-','C5','-','-','-','-','-','x','-', # bar 5  contrary motion vs A5-G5-F5
    'A4','-','C5','-','A4','-','F4','-','G4','-','-','-','-','-','x','-', # bar 6  SINGS the melody's rest
    'D4','-','-','-','E4','-','F#4','-','G4','-','-','-','-','-','x','-', # bar 7  contrary vs B5-A5-G5
    'G4','-','F#4','-','E4','-','D4','-','B3','-','-','-','-','-','x','-', # bar 8  answers in the rest
    'D#4','-','F4','-','G4','-','G#4','-','C5','-','-','-','-','-','-','-', # bar 9  swells under held C6
    'D#5','-','-','-','C5','-','-','-','G4','-','-','-','-','-','x','-', # bar 10 chord tones (P5, m6) below
    'F4','-','-','-','E4','-','-','-','A#3','-','-','-','-','-','x','-', # bar 11 warm low anchor under D6 peak
    'F5','-','D5','-','A#4','-','F4','-','A#4','-','-','-','-','-','x','-', # bar 12 spotlight fill in the rest
    'A4','-','-','-','C5','-','-','-','D5','-','C5','-','-','-','x','-', # bar 13 contrary vs C6-A5-G5-F5
    'D4','-','F4','-','A4','-','-','-','F4','-','-','-','-','-','x','-', # bar 14 Dm arpeggio in 3rds below
    'D4','-','G4','-','B4','-','-','-','B4','-','-','-','-','-','-','-', # bar 15 G-triad 6ths; holds B4 for seam
    'B3','-','A3','-','G3','-','F3','-','G3','-','-','-','-','-','-','-', # bar 16 3rds/6ths + F3 (7th of G7)
    'E4','-','F4','-','G4','-','-','-','G4','-','-','-','C5','-','-','-', # bar 17 TAG lands C5 (3rd under E5)
    'C5','-','-','-','B4','-','-','-','G3','-','-','-','-','-','-','-') # bar 18 TAG sus C5->B4 leading-tone home

CH_ARP = seq(                                    # ch2 — arpeggio bed (unchanged identity; sounds 8vb)
    'C3','-','E3','-','G3','-','C4','-','E4','-','G4','-','C5','-','E4','-',   # bar 1  C
    'C3','-','E3','-','G3','-','C4','-','E4','-','G4','-','A4','-','E4','-',   # bar 2  C6
    'A2','-','C3','-','E3','-','A3','-','C4','-','E4','-','A4','-','C4','-',   # bar 3  Am
    'A2','-','C3','-','E3','-','A3','-','C4','-','E4','-','A4','-','C4','-',   # bar 4  Am
    'F2','-','A2','-','C3','-','F3','-','A3','-','C4','-','F4','-','A3','-',   # bar 5  F
    'F2','-','A2','-','C3','-','F3','-','A3','-','C4','-','F4','-','A3','-',   # bar 6  F
    'G2','-','B2','-','D3','-','G3','-','B3','-','D4','-','G4','-','D4','-',   # bar 7  G
    'G2','-','B2','-','D3','-','G3','-','B3','-','D4','-','G4','-','B4','-',   # bar 8  G
    'G#2','-','C3','-','D#3','-','G#3','-','C4','-','D#4','-','G#4','-','D#4','-', # bar 9  Ab maj7 (bVI)
    'G#2','-','C3','-','D#3','-','G#3','-','C4','-','D#4','-','C5','-','G#4','-',  # bar 10 Ab
    'A#2','-','D3','-','F3','-','A#3','-','D4','-','F4','-','A#4','-','F4','-', # bar 11 Bb (bVII)
    'A#2','-','D3','-','F3','-','A#3','-','D4','-','F4','-','D5','-','A#4','-', # bar 12 Bb
    'F2','-','A2','-','C3','-','F3','-','A3','-','C4','-','F4','-','A4','-',   # bar 13 F (home)
    'D3','-','F3','-','A3','-','D4','-','F4','-','A4','-','F4','-','D4','-',   # bar 14 Dm
    'G2','-','B2','-','D3','-','G3','-','B3','-','D4','-','G4','-','B4','-',   # bar 15 G
    'G2','-','B2','-','D3','-','F3','-','G3','-','B3','-','D4','-','F4','-',   # bar 16 G7 (V7 seam)
    'C3','-','E3','-','G3','-','C4','-','E4','-','G4','-','C5','-','E4','-',   # bar 17 C  (TAG arrival)
    'G2','-','B2','-','D3','-','F3','-','G3','-','B3','-','D4','-','F4','-')   # bar 18 G7 (TAG -> loop)

CH_DRUM = rep(['x'], 288)                         # ch3 — silent (drumless title)
```

**What I did**

- Turned ch1 from a sparse octave-echo into a genuine **countermelody duet** (ch0/ch1): contrary motion under every melodic phrase (bars 5, 7, 13), independent singing **fills** where the melody rests (bars 6, 8, 12), and 3rds/6ths on shared attacks. It always stays below the melody as a warm inner voice.
- Added a **2-bar tag** (bars 17–18, rows 256–287) for an **earned V7→I cadence**: the melody arrives on C6, falls to a held E5 that suspends; the countermelody lands a C5 (a 3rd under the tonic), then resolves **C5 → B4 (leading tone) → home** on bar 1's downbeat, so the loop is a gentle breath, not a cut.
- **Preserved the identity**: same C-major chord journey (C·C6·Am·Am·F·F·G·G·Ab·Ab·Bb·Bb·F·Dm·G·G7 + C·G7 tag), the same rising arpeggio bed note-for-note, the melody's arc byte-identical for bars 1–16, speed 9, 16 rows/bar, 2 rows/note, drumless.

**Verification**: `TOTAL_ROWS = 288`, all four channels exactly 288. Project linter (`tools/music/check_music.py`) passes: PRELUDE strong-beat dissonance 0/30, voice-crossing 0/164, all weak-beat suspensions resolve by step, arp bed within the wave clamp (≤ B5). The lone "square spacing > a 12th" note is an informational warn (soaring melody over inner-voice counter), not a failure. `tools/mkassets.py` compiles it cleanly into `build/gen/assets.c` (drum channel all-REST, Song struct records 288 rows / loop 0 / speed 9).

**Files changed**: `/Users/mtklein/demake/tools/music/songs.py` (the `prelude()` function) and `/Users/mtklein/demake/tools/music/check_music.py` (FORMS map updated to the new 5-section, 288-row structure).