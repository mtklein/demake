The ROM builds clean with the new BOSS theme compiled in. The music-lint passes with zero errors and zero warnings for BOSS, and the full build (assets → C → ELF → GBA, with `fixrom`) succeeds. Everything is verified end-to-end.

Now let me emit the required output. The prompt demands the final message be ONLY a Python block, parseable, with the exact `TOTAL_ROWS`/`FORM` header and the four `CH_*` channels. I'll present the composition as it now lives in `songs.py`, with one bar per line and odd-length bars labeled.

```python
# TOTAL_ROWS = 352
# FORM (speed 5, 4 rows/beat; ch0=Zhalk sword 75% duty, ch1=mind-flayer 50% hollow,
#       ch2=drop-tuned bass wave sounds 8vb -- pitches written are what you HEAR,
#       ch3=noise 0kick 1snare 2hat 3crash 4ride; E tonic, E phrygian-dominant E F G# A B C D):
#   @0   CHUG RIFF A   -- bars: 16, 16, 14 (7/8), 10 (5/8).  Odd pair sums to 24 so row 56 realigns.
#                         Each odd bar ends on a unison STOP-HIT into an 'x' VOID.
#   @56  CHUG RIFF A'  -- bars: 16, 16, 14 (7/8), 10 (5/8).  Gallop varied, second void.
#   @112 MANIC VERSE   -- 4x16.  ch0 rips phrygian-dominant tremolo (re-attack every row); ch1 TACET.
#   @176 CLEAN BRIDGE  -- 4x16.  THE WHIPLASH: sword (ch0) falls SILENT; only the flayer's croon
#                         (ch1) over a held bass. Beautiful and wrong. Drums a soft ride only.
#   @240 HALF-TIME BREAKDOWN -- 3x16.  Octave slam on beats 1&3, held G# (maj3) grind; huge, slow.
#   @288 CHAOTIC CLIMAX -- 3x16.  Register/key LIFT E->G (minor 3rd): chug up to G2, G-phryg-dom
#                         tremolo, snare rolls into crashes; last bar wrenches home to E.
#   @336 TURNAROUND    -- 1x16.  8-row snare roll -> crash, unison E slam, void; seam -> loop row 0.
# All four channels EXACTLY 352 rows. loop = row 0. music-lint: OK (strong-diss 0/0, cross 0/0).

from songs import seq, rep, n, HOLD, REST
X = REST

CH_LEAD = (  # ch0 -- ZHALK (sq1, 75% duty, biting): chug gallop, tremolo fury
    # --- CHUG RIFF A (0-55) ---
    seq('E3','E3','E3','G#3','E3','E3','E3','F3','E3','E3','G#3','E3','E3','F3','E3','E3')      # bar 0 (CHUG A, 4/4)
  + seq('E3','E3','E3','E3','G#3','E3','A3','E3','C4','E3','D4','E3','B3','G#3','F3','E3')      # bar 1 (CHUG A, 4/4)
  + seq('E3','E3','E3','G#3','E3','E3','G#4','x','x','x','E3','E3','G#3','F3')                 # bar 2 (CHUG A, 7/8 = 14, stop-hit->void)
  + seq('E3','E3','G#3','F3','E3','x','x','x','x','x')                                         # bar 3 (CHUG A, 5/8 = 10, VOID)
    # --- CHUG RIFF A' (56-111) ---
  + seq('E3','E3','G#3','E3','E3','F3','E3','E3','E3','G#3','E3','E3','F3','E3','G#3','E3')      # bar 4 (CHUG A', 4/4)
  + seq('E3','E3','E3','G#3','A3','B3','C4','D4','E4','E3','D4','C4','B3','A3','G#3','F3')       # bar 5 (CHUG A', 4/4, rip to octave)
  + seq('E3','E3','E3','F3','E3','E3','G#4','x','x','x','E3','G#3','B3','G#3')                  # bar 6 (CHUG A', 7/8 = 14, stop-hit->void)
  + seq('E3','E3','G#3','F3','E4','x','x','x','x','x')                                         # bar 7 (CHUG A', 5/8 = 10, E4 scream->VOID)
    # --- MANIC VERSE (112-175) ---
  + seq('E4','F4','G#4','A4','B4','C5','D5','C5','B4','A4','G#4','A4','B4','C5','B4','A4')       # bar 8 (VERSE, 4/4, rip up phryg-dom)
  + seq('G#4','G#4','A4','A4','B4','B4','A4','A4','G#4','G#4','F4','F4','G#4','A4','B4','C5')    # bar 9 (VERSE, 4/4, tremolo pivot)
  + seq('D5','E5','D5','C5','B4','A4','G#4','F4','E4','F4','G#4','A4','B4','C5','D5','E5')       # bar 10 (VERSE, 4/4, scream to E5, crash down)
  + seq('E5','E5','D5','D5','C5','C5','B4','B4','A4','G#4','F4','E4','D4','C4','B3','G#3')       # bar 11 (VERSE, 4/4, high tremolo -> dive)
    # --- CLEAN BRIDGE (176-239): sword SILENT (flayer alone) ---
  + seq(*(['x'] * 64))                                                                          # bars 12-15 (BRIDGE, 4x 4/4, ch0 tacet)
    # --- HALF-TIME BREAKDOWN (240-287) ---
  + seq('E3',HOLD,HOLD,HOLD,'x','x','x','x','E3',HOLD,HOLD,HOLD,'G#3',HOLD,'F3','E3')            # bar 16 (BREAKDOWN, 4/4, half-time slam)
  + seq('E3',HOLD,HOLD,HOLD,'x','x','x','x','G#3',HOLD,HOLD,HOLD,HOLD,HOLD,'F3','E3')            # bar 17 (BREAKDOWN, 4/4, held G# grind)
  + seq('E3',HOLD,HOLD,HOLD,'x','x','E3','E3','G#3',HOLD,'A3','B3','C4','B3','A3','G#3')          # bar 18 (BREAKDOWN, 4/4, fill out)
    # --- CHAOTIC CLIMAX (288-335): LIFT E->G ---
  + seq('G4','G4','A#4','A#4','C5','C5','D5','D5','A#4','A#4','C5','D5','D#5','D5','C5','A#4')    # bar 19 (CLIMAX, 4/4, G phryg-dom tremolo)
  + seq('D#5','D5','C5','A#4','G4','A#4','C5','D5','D#5','D5','C5','A#4','G4','G4','A#4','C5')    # bar 20 (CLIMAX, 4/4, G-summit scream)
  + seq('D5','C5','B4','A4','G#4','F4','E4','G#4','B4','E5','E5','E5','E5','D5','C5','B4')        # bar 21 (CLIMAX, 4/4, wrench home to E, E5 scream)
    # --- TURNAROUND (336-351) ---
  + seq('E4','x','x','x','E4','x','x','x','G#4','x','B4','x','E5',HOLD,'x','x')                  # bar 22 (TURN, 4/4, E slam + void -> loop)
)

CH_HARM = (  # ch1 -- MIND FLAYER (sq2, 50% hollow): TACET in the chaos; the clean croon in the bridge
    rep([REST], 112)                                                                            # CHUG A + A' : tacet
  + rep([REST], 64)                                                                             # MANIC VERSE : tacet
    # --- CLEAN BRIDGE (176-239): the flayer's croon -- slow, eerie, lyrical, alone ---
  + seq('B4',HOLD,HOLD,HOLD,'C5',HOLD,'B4',HOLD,'G#4',HOLD,HOLD,HOLD,'A4',HOLD,HOLD,HOLD)        # bar 12 (BRIDGE, 4/4)
  + seq('G4',HOLD,'F4',HOLD,'E4',HOLD,HOLD,HOLD,'F4',HOLD,'G4',HOLD,'B4',HOLD,HOLD,HOLD)         # bar 13 (BRIDGE, 4/4)
  + seq('C5',HOLD,HOLD,HOLD,'B4',HOLD,'A4',HOLD,'G#4',HOLD,HOLD,HOLD,'E4',HOLD,HOLD,HOLD)        # bar 14 (BRIDGE, 4/4)
  + seq('F4',HOLD,'G4',HOLD,'F4',HOLD,'E4',HOLD,'E4',HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD)         # bar 15 (BRIDGE, 4/4)
  + rep([REST], 48)                                                                             # HALF-TIME BREAKDOWN : tacet
  + rep([REST], 48)                                                                             # CHAOTIC CLIMAX : tacet
  + rep([REST], 16)                                                                             # TURNAROUND : tacet
)

CH_BASS = (  # ch2 -- drop-tuned bass chug (wave, sounds 8vb; pitches are what you HEAR, E2/G2 low)
    # --- CHUG RIFF A (0-55): E2 gallop, octave-unison under ch0 ---
    seq('E2','E2','E2','G#2','E2','E2','E2','F2','E2','E2','G#2','E2','E2','F2','E2','E2')       # bar 0 (4/4)
  + seq('E2','E2','E2','E2','G#2','E2','A2','E2','C3','E2','D3','E2','B2','G#2','F2','E2')       # bar 1 (4/4)
  + seq('E2','E2','E2','G#2','E2','E2','G#3','x','x','x','E2','E2','G#2','F2')                  # bar 2 (7/8 = 14)
  + seq('E2','E2','G#2','F2','E2','x','x','x','x','x')                                          # bar 3 (5/8 = 10, VOID)
    # --- CHUG RIFF A' (56-111) ---
  + seq('E2','E2','G#2','E2','E2','F2','E2','E2','E2','G#2','E2','E2','F2','E2','G#2','E2')       # bar 4 (4/4)
  + seq('E2','E2','E2','G#2','A2','B2','C3','D3','E3','E2','D3','C3','B2','A2','G#2','F2')        # bar 5 (4/4)
  + seq('E2','E2','E2','F2','E2','E2','G#3','x','x','x','E2','G#2','B2','G#2')                  # bar 6 (7/8 = 14)
  + seq('E2','E2','G#2','F2','E3','x','x','x','x','x')                                          # bar 7 (5/8 = 10, VOID)
    # --- MANIC VERSE (112-175): relentless E2 sixteenth pedal ---
  + rep([n('E2')], 16)                                                                          # bar 8 (4/4)
  + rep([n('E2')], 16)                                                                          # bar 9 (4/4)
  + rep([n('E2')], 16)                                                                          # bar 10 (4/4)
  + seq('E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2')          # bar 11 (4/4)
    # --- CLEAN BRIDGE (176-239): held / slow bass under the croon ---
  + seq('E2',HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD)         # bar 12 (E)
  + seq('C2',HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD)         # bar 13 (C)
  + seq('A2',HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD)         # bar 14 (A)
  + seq('B2',HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,'B2',HOLD,HOLD,HOLD,HOLD,HOLD,HOLD,HOLD)         # bar 15 (B, the V)
    # --- HALF-TIME BREAKDOWN (240-287): huge E2 on 1 & 3, G# grind ---
  + seq('E2',HOLD,HOLD,HOLD,'x','x','x','x','E2',HOLD,HOLD,HOLD,'G#2',HOLD,'F2','E2')            # bar 16 (4/4)
  + seq('E2',HOLD,HOLD,HOLD,'x','x','x','x','G#2',HOLD,HOLD,HOLD,HOLD,HOLD,'F2','E2')            # bar 17 (4/4)
  + seq('E2',HOLD,HOLD,HOLD,'x','x','E2','E2','G#2',HOLD,'A2','B2','C3','B2','A2','G#2')          # bar 18 (4/4)
    # --- CHAOTIC CLIMAX (288-335): chug LIFTED to G2, last bar home to E2 ---
  + rep([n('G2')], 16)                                                                          # bar 19 (4/4)
  + rep([n('G2')], 16)                                                                          # bar 20 (4/4)
  + seq('E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2','E2')          # bar 21 (4/4)
    # --- TURNAROUND (336-351) ---
  + seq('E2','x','x','x','E2','x','x','x','G#2','x','B2','x','E2',HOLD,'x','x')                  # bar 22 (4/4, E slam + void)
)

CH_DRUM = (  # ch3 -- noise: 0 kick, 1 snare, 2 hat, 3 crash, 4 ride
    # --- CHUG RIFF A (0-55) ---
    [3,0,2,0, 1,0,0,2, 0,0,2,0, 1,0,0,2]                                                        # bar 0 (4/4)
  + [0,0,2,0, 1,0,0,2, 0,2,0,2, 1,0,0,0]                                                        # bar 1 (4/4)
  + [3,0,2,0, 1,0,3,X,X,X, 0,0,1,0]                                                             # bar 2 (7/8 = 14, stop-hit)
  + [0,0,1,0, 3,X,X,X,X,X]                                                                      # bar 3 (5/8 = 10, VOID)
    # --- CHUG RIFF A' (56-111) ---
  + [3,0,2,0, 1,0,0,2, 0,0,2,0, 1,0,2,0]                                                        # bar 4 (4/4)
  + [0,0,2,0, 1,0,0,0, 0,0,0,0, 1,0,0,0]                                                        # bar 5 (4/4)
  + [3,0,2,0, 1,0,3,X,X,X, 0,0,1,1]                                                             # bar 6 (7/8 = 14)
  + [0,0,1,0, 3,X,X,X,X,X]                                                                      # bar 7 (5/8 = 10, VOID)
    # --- MANIC VERSE (112-175): blast beat -> fill/crash ---
  + [0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1]                                                        # bar 8 (4/4)
  + [0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1]                                                        # bar 9 (4/4)
  + [0,1,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,1]                                                        # bar 10 (4/4)
  + [0,1,0,1, 0,1,0,1, 0,1,0,1, 1,1,1,3]                                                        # bar 11 (4/4, fill -> crash)
    # --- CLEAN BRIDGE (176-239): a soft ride ticking, else silent ---
  + [4,X,X,X,X,X,X,X, 4,X,X,X,X,X,X,X]                                                          # bar 12 (4/4)
  + [4,X,X,X,X,X,X,X, 4,X,X,X,X,X,X,X]                                                          # bar 13 (4/4)
  + [4,X,X,X,X,X,X,X, 4,X,X,X,X,X,X,X]                                                          # bar 14 (4/4)
  + [4,X,X,X,X,X,X,X, 4,X,X,X,X,X,X,X]                                                          # bar 15 (4/4)
    # --- HALF-TIME BREAKDOWN (240-287): crash+kick on 1, snare on 3 ---
  + [3,X,X,X,X,X,X,X, 1,X,X,X,X,X,X,X]                                                          # bar 16 (4/4)
  + [3,X,X,X,X,X,X,X, 1,X,X,X,X,X,X,X]                                                          # bar 17 (4/4)
  + [3,X,X,X,X,X,0,0, 1,X,0,0, 1,0,1,0]                                                         # bar 18 (4/4, fill out)
    # --- CHAOTIC CLIMAX (288-335): snare rolls into crashes ---
  + [0,1,0,1, 1,1,1,1, 0,1,0,1, 1,1,1,3]                                                        # bar 19 (4/4, roll -> crash)
  + [0,1,0,1, 0,1,0,1, 0,1,0,1, 1,1,1,3]                                                        # bar 20 (4/4)
  + [0,1,0,1, 1,1,1,1, 0,1,1,1, 1,1,1,3]                                                        # bar 21 (4/4, roll -> crash)
    # --- TURNAROUND (336-351): 8-row snare roll -> crash, unison slam, void ---
  + [1,1,1,1, 1,1,1,3, 0,X,0,X, 3,X,X,X]                                                        # bar 22 (4/4)
)
```