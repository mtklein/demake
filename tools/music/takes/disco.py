Everything verified against the real linter. Note the STAB intro (bar 0) is silent and bar 1 teases with `stab2` — the transcript confirms the exact per-bar content. I have the precise, validated bars to transcribe into the strict output format.

```python
# TOTAL_ROWS = 256
# FORM: 0=INTRO(bass octave-mantra + 4-on-floor establish; kit opens bar 2)
#       32=A(robot hook x2 + vocoder off-beat stabs, full disco groove)
#       96=A'(hook varied/brighter, register up, major-splash turns)
#       160=BREAK/THE DROP(strip to bass+kick, "filter shut": closed hats only)
#       192=BUILD(riser: lead pulses climb, ride shimmer opens, snare roll explodes)
#       224=A''(hook returns full; bar 2 = turnaround fill -> loop row 0, seam under the 4-on-floor kick)
# Grid: speed 7 (~126 BPM), 16 rows/bar, 4/4. Key: A minor / A Dorian (modal house vamp).
# ch0 sq1 25% duty (thin/robotic hook) | ch1 sq2 50% (off-beat stabs, <= ch0)
# ch2 wave 8vb (write-what-you-hear octave bass) | ch3 noise: 0 kick 1 snare 2 closed-hat 3 open-hat 4 ride
# Four-on-the-floor: kick on EVERY beat (rows 0/4/8/12) in every bar except the
# snare-roll climax (build) and the pre-loop fill (both intentional). Off-beat
# open-hats on rows 2/6/10/14 = the disco "tss". Lead & stabs interlock (never
# stack on strong beats): strong-beat sq-vs-sq dissonance 0%, voice-crossing 0%.
# Envelope intent: env1=25% duty sustained lead, env2=50% duty stabs, wavevol 0x2000.

CH_LEAD = seq(                                              # ch0 sq1 25% -- ROBOT HOOK
    'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x',           # bar 0 (INTRO: silent)
    'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x',           # bar 1 (INTRO: silent)
    'A4','x','A4','C5','x','A4','E5','x','D5','x','C5','x','A4','x','G4','x',   # bar 2 (A: hook a)
    'A4','x','C5','x','E5','x','D5','C5','A4','x','G4','A4','x','E4','x','x',   # bar 3 (A: hook b)
    'A4','x','A4','C5','x','A4','E5','x','D5','x','C5','x','A4','x','G4','x',   # bar 4 (A: hook a)
    'A4','x','C5','x','E5','x','D5','C5','A4','x','G4','A4','x','E4','x','x',   # bar 5 (A: hook b)
    'A4','x','E5','x','A5','x','G5','E5','D5','x','C5','D5','x','E5','x','x',   # bar 6 (A': hook a', soar)
    'A4','x','C5','E5','x','D5','x','C5','A4','x','A4','G4','x','A4','x','x',   # bar 7 (A': hook b')
    'A4','x','E5','x','A5','x','G5','E5','D5','x','C5','D5','x','E5','x','x',   # bar 8 (A': hook a')
    'A4','x','C5','E5','x','D5','x','C5','A4','x','A4','G4','x','A4','x','x',   # bar 9 (A': hook b')
    'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x',           # bar 10 (BREAK: stripped)
    'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x',           # bar 11 (BREAK: stripped)
    'A4','x','A4','x','A4','x','A4','x','A4','x','A4','x','A4','A4','A4','A4',  # bar 12 (BUILD: riser lo)
    'C5','x','C5','x','E5','x','E5','x','A5','x','A5','A5','A5','A5','A5','A5', # bar 13 (BUILD: riser climbs)
    'A4','x','A4','C5','x','A4','E5','x','D5','x','C5','x','A4','x','G4','x',   # bar 14 (A'': hook a)
    'A4','x','C5','x','E5','x','D5','C5','A4','x','G4','x','E4','x','x','x')    # bar 15 (A'': turnaround)

CH_STAB = seq(                                              # ch1 sq2 50% -- VOCODER STABS (off-beat)
    'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x',           # bar 0 (INTRO: silent)
    'x','E4','x','C4','x','E4','x','x','x','C4','x','E4','x','C4','x','x',     # bar 1 (INTRO: stabs tease)
    'x','E4','x','x','x','E4','x','C4','x','E4','x','x','x','E4','x','D4',     # bar 2 (A: stab a)
    'x','E4','x','C4','x','E4','x','x','x','C4','x','E4','x','C4','x','x',     # bar 3 (A: stab b)
    'x','E4','x','x','x','E4','x','C4','x','E4','x','x','x','E4','x','D4',     # bar 4 (A: stab a)
    'x','E4','x','C4','x','E4','x','x','x','C4','x','E4','x','C4','x','x',     # bar 5 (A: stab b)
    'x','A4','x','x','x','G4','x','E4','x','A4','x','x','x','G4','x','E4',     # bar 6 (A': stab a', bright)
    'x','G4','x','E4','x','A4','x','x','x','E4','x','C4','x','E4','x','x',     # bar 7 (A': stab b')
    'x','A4','x','x','x','G4','x','E4','x','A4','x','x','x','G4','x','E4',     # bar 8 (A': stab a')
    'x','G4','x','E4','x','A4','x','x','x','E4','x','C4','x','E4','x','x',     # bar 9 (A': stab b')
    'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x',           # bar 10 (BREAK: silent)
    'x','x','x','x','x','x','x','x','x','x','x','x','x','x','x','x',           # bar 11 (BREAK: silent)
    'x','E4','x','E4','x','E4','x','E4','x','E4','x','E4','x','E4','x','E4',   # bar 12 (BUILD: pumping E)
    'x','E4','x','E4','x','E4','x','E4','x','E4','x','E4','x','E4','x','E4',   # bar 13 (BUILD: pumping E)
    'x','E4','x','x','x','E4','x','C4','x','E4','x','x','x','E4','x','D4',     # bar 14 (A'': stab a)
    'x','E4','x','C4','x','E4','x','x','x','C4','x','E4','x','E4','x','x')     # bar 15 (A'': turnaround)

CH_BASS = seq(                                              # ch2 wave 8vb -- SYNCOPATED OCTAVE BASS
    'A2','A3','x','A2','A3','x','A2','A3','x','A2','A3','x','A3','x','A2','A3', # bar 0 (INTRO: A mantra)
    'G2','G3','x','G2','G3','x','G2','G3','x','A2','A3','x','A3','x','A2','A3', # bar 1 (INTRO: bVII->i)
    'A2','A3','x','A2','A3','x','A2','A3','x','A2','A3','x','A3','x','A2','A3', # bar 2 (A: mantra)
    'G2','G3','x','G2','G3','x','G2','G3','x','A2','A3','x','A3','x','A2','A3', # bar 3 (A: bVII->i)
    'A2','A3','x','A2','A3','x','A2','A3','x','A2','A3','x','A3','x','A2','A3', # bar 4 (A: mantra)
    'G2','G3','x','G2','G3','x','G2','G3','x','A2','A3','x','A3','x','A2','A3', # bar 5 (A: bVII->i)
    'A2','A3','x','A2','A3','x','A2','A3','x','A2','A3','x','A3','x','A2','A3', # bar 6 (A': mantra)
    'G2','G3','x','G2','G3','x','G2','G3','x','A2','A3','x','A3','x','A2','A3', # bar 7 (A': bVII->i)
    'A2','A3','x','A2','A3','x','A2','A3','x','A2','A3','x','A3','x','A2','A3', # bar 8 (A': mantra)
    'G2','G3','x','G2','G3','x','G2','G3','x','A2','A3','x','A3','x','A2','A3', # bar 9 (A': bVII->i)
    'A2','x','A3','x','A2','x','A3','x','A2','x','A3','x','A2','x','A3','x',     # bar 10 (BREAK: pump 8ths)
    'A2','x','A3','x','A2','x','A3','x','A2','x','A3','x','A2','x','A3','x',     # bar 11 (BREAK: pump 8ths)
    'A2','A3','A2','A3','A2','A3','A2','A3','A2','A3','A2','A3','A2','A3','A2','A3', # bar 12 (BUILD: 16ths)
    'C3','C4','C3','C4','E3','E4','E3','E4','A3','A4','A3','A4','A3','A4','A3','A4', # bar 13 (BUILD: climb i->III->v->i)
    'A2','A3','x','A2','A3','x','A2','A3','x','A2','A3','x','A3','x','A2','A3', # bar 14 (A'': mantra)
    'G2','G3','x','G2','G3','x','G2','G3','x','A2','A3','x','A3','x','A2','A3') # bar 15 (A'': lands on A -> loop)

CH_DRUM = seq(                                              # ch3 noise: 0 K 1 S 2 hat 3 open 4 ride
    0,2,2,2, 0,2,2,2, 0,2,2,2, 0,2,2,2,                       # bar 0 (INTRO: 4-on-floor, filter shut)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 1 (INTRO: open hats engage)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 2 (A: full groove)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 3 (A)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 4 (A)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 5 (A)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 6 (A')
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 7 (A')
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 8 (A')
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 9 (A')
    0,2,2,2, 0,2,2,2, 0,2,2,2, 0,2,2,2,                       # bar 10 (BREAK: filter shut, kick+closed)
    0,2,2,2, 0,2,2,2, 0,2,2,2, 0,2,2,2,                       # bar 11 (BREAK)
    0,4,3,4, 0,4,3,4, 0,4,3,4, 0,4,3,4,                       # bar 12 (BUILD: ride shimmer opens)
    0,2,1,2, 0,1,2,1, 0,1,1,1, 1,1,1,1,                       # bar 13 (BUILD: snare roll EXPLODES)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 0,2,3,2,                       # bar 14 (A'': full groove)
    0,2,3,2, 0,2,3,2, 0,2,3,2, 1,1,1,1)                       # bar 15 (A'': turnaround fill -> loop)
```