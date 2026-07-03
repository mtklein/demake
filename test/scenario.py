#!/usr/bin/env python3
"""Emit runner scripts for deterministic playthroughs: scenario.py NAME > x.script

Sync model: the ROM raises G_FIELD_IDLE=1 whenever the field loop is ready for
input and lowers it during dialog / battle / room transitions. The runner's
`until` command blocks on that flag, so the scenario never guesses frame counts
for variable-length narration -- it walks, then waits for the field to be ready
again. Dialogue auto-advances (G_DEMO) and choices pop from a poked buffer.
"""
import sys

out = []
def w(s): out.append(s)
def wait(n): w(f"wait {n}")
def tap(k, n=1, gap=8):
    for _ in range(n):
        w(f"hold {k} 3"); w(f"wait {gap}")
def walk(k, steps):
    w(f"hold {k} {steps * 8}"); w("wait 12")
def shot(name): w(f"shot test/shots/{name}.ppm")
def poke(addr, val): w(f"poke {addr:08x} {val:02x}")

DEMO, BATTLE, CLASS, IDLE, DONE = 0x0203FF00, 0x0203FF02, 0x0203FF03, 0x0203FF05, 0x0203FF06
CHOICE = 0x0203FF10

def ready(maxf=9000): w(f"until {IDLE:08x} 01 {maxf}")
def done(maxf=40000): w(f"until {DONE:08x} 01 {maxf}")

def setup(cls, choices, battle_mode=0):
    poke(DEMO, 1); poke(BATTLE, battle_mode); poke(CLASS, cls)
    poke(IDLE, 0); poke(DONE, 0)
    for i, c in enumerate(choices):
        poke(CHOICE + i, c)
    # magic cookie: gba_init wipes the flag block without it (hardware safety)
    for i, b in enumerate((0x01, 0xEE, 0xFF, 0xC0)):
        poke(0x0203FF38 + i, b)

def intro():
    wait(160); shot("s_title")
    wait(420); shot("s_class")     # crawl auto-plays, class auto-selects
    ready()                        # nursery intro (wake) finishes -> field ready
    shot("s_nursery")

def face_interact(dir):
    tap(dir); tap("A"); ready()

# --- rooms (each ends field-ready in the NEXT room) ---

def nursery():
    walk("RIGHT", 4); walk("DOWN", 3)     # door -> surgery
    ready(); shot("s_surgery_in")

def surgery():
    tap("DOWN"); tap("A"); ready()        # apparatus (below spawn) -> Us extraction
    shot("s_surgery")
    walk("LEFT", 2); walk("DOWN", 7); walk("RIGHT", 2); walk("DOWN", 1)  # door -> deck
    ready(); shot("s_deck_in")

def deck():
    walk("DOWN", 1); ready()              # Lae'zel scene + imp battle auto-resolves
    shot("s_deckdone")
    walk("DOWN", 8); ready()              # door -> pods
    shot("s_pods_in")

def pods():
    walk("RIGHT", 1); walk("DOWN", 4); walk("RIGHT", 3)   # onto rune corpse
    ready()
    walk("LEFT", 7)                       # below the console
    tap("UP"); tap("A"); ready()          # slot rune -> free Shadowheart
    shot("s_pods")
    walk("RIGHT", 3); walk("DOWN", 5); ready()   # door -> helm
    shot("s_helm_in")

def helm():
    walk("UP", 5)                         # trigger the finale
    shot("s_helm")
    done()                                # battle + crash + beach + tally
    shot("s_tally")

SCN = {}
def scn(f): SCN[f.__name__] = f; return f

@scn
def bard_full():
    setup(0, [0, 0, 0, 0, 0])             # extract+welcome Us, fight, slot rune, join SH
    intro(); nursery(); surgery(); deck(); pods(); helm()

@scn
def wizard_zhalk():
    setup(3, [0, 0, 0, 0, 0], battle_mode=2)  # kill Zhalk for the Everburn Blade
    intro(); nursery(); surgery(); deck(); pods(); helm()

@scn
def rogue_mutilate():
    setup(1, [0, 1, 0, 0, 2])             # mutilate Us, leave Shadowheart behind
    intro(); nursery(); surgery(); deck(); pods(); helm()

@scn
def ranger_full():
    setup(2, [0, 0, 0, 0, 0])             # ranger, spare Us, save everyone, connect
    intro(); nursery(); surgery(); deck(); pods(); helm()

if __name__ == "__main__":
    SCN[sys.argv[1]]()
    print("\n".join(out))
