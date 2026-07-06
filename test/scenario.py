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
_face = ['DOWN']          # scenario-side facing tracker (spawns face DOWN)
def _pre_turn(k):
    """the field turns in place for 4 frames when direction changes"""
    if _face[0] != k:
        w(f"hold {k} 4")
        _face[0] = k

def tap(k, n=1, gap=8):
    if k in ('UP', 'DOWN', 'LEFT', 'RIGHT'):
        _face[0] = k                     # a field tap turns in place
    for _ in range(n):
        w(f"hold {k} 3"); w(f"wait {gap}")
def walk(k, steps):
    _pre_turn(k)
    w(f"hold {k} {steps * 8}"); w("wait 12")   # settle before next command

def shot(name): w(f"shot test/shots/{name}.ppm")
def poke(addr, val): w(f"poke {addr:08x} {val:02x}")

DEMO, BATTLE, CLASS, IDLE, DONE = 0x0203FF00, 0x0203FF02, 0x0203FF03, 0x0203FF05, 0x0203FF06
CHOICE = 0x0203FF10

def ready(maxf=9000):
    w(f"until {IDLE:08x} 01 {maxf}")
    _face[0] = 'DOWN'   # room spawns face down
def done(maxf=90000): w(f"until {DONE:08x} 01 {maxf}")

def setup(cls, choices, battle_mode=0):
    poke(DEMO, 1); poke(BATTLE, battle_mode); poke(CLASS, cls)
    poke(0x0203FF0E, 7)   # origin = custom Tav (fixed origins override in their own scenarios)
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

# --- living-encounter verifications -------------------------------------

def _to_deck_cleared():
    intro(); nursery(); surgery()
    walk("DOWN", 1); ready(24000)         # deck brawl
    walk("DOWN", 8); ready()              # door -> pods

def sneak_strike_body():
    """Bard flanks the deck straggler through its blind rear, strikes first,
    and the kill persists across a deck<->pods round trip."""
    _to_deck_cleared()
    walk("UP", 1); ready()                # return door -> deck: straggler is up
    walk("UP", 4); walk("RIGHT", 4); walk("DOWN", 1); walk("RIGHT", 2)
    wait(30); shot("g_sneak")             # (16,6): in its blind spot
    walk("DOWN", 1)                       # bump to face it
    tap("A")                              # strike first: imp is surprised
    wait(130); shot("g_strike")
    ready(24000)
    walk("DOWN", 3); walk("LEFT", 6); walk("DOWN", 1); ready()
    walk("UP", 1); ready()                # round trip: doors both ways
    wait(30); shot("g_stays_dead")        # no respawn

def cone_ambush_body():
    """Wizard blunders into the pods prowler's forward cone; it chases and
    opens the fight with the party surprised."""
    _to_deck_cleared()
    walk("RIGHT", 1); walk("DOWN", 2)     # (9,3)
    walk("RIGHT", 5)                      # (14,3): inside the front cone
    wait(12); shot("g_alert")
    wait(90); shot("g_chase")
    ready(24000)                          # it reaches us: ambush
    wait(30); shot("g_ambushed")

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

def _mk_smoke(cls):
    def f():
        """boot as this class, win the deck brawl"""
        setup(cls, [0, 1, 0, 0, 2])
        intro(); nursery(); surgery()
        walk("DOWN", 1)
        w(f"until {IDLE:08x} 01 24000")
    f.__name__ = f"smoke_{cls}"
    return f

for _c in range(12):
    SCN[f"smoke_{_c}"] = _mk_smoke(_c)

@scn
def origin_check():
    """Shadowheart origin: cleric, Masks subclass (id 16) from level 1, her name"""
    poke(DEMO, 1); poke(BATTLE, 0); poke(0x0203FF0E, 4)   # origin = Shadowheart
    for i, byte in enumerate((0x01, 0xEE, 0xFF, 0xC0)): poke(0x0203FF38 + i, byte)
    poke(0x0203FF05, 0); poke(0x0203FF06, 0)
    wait(700)                          # title + crawl + origin auto-select
    w(f"until {IDLE:08x} 01 12000")    # reach the nursery
    shot("origin_shadow")

@scn
def prepare_check():
    """cleric opens the prepare screen and toggles a spell without crashing
    (guards the long-spell-name buffer overflow)"""
    setup(5, [0, 1, 0, 0, 2])
    intro()
    tap("START"); wait(20)
    tap("DOWN", 2, gap=12); tap("A"); wait(20)   # -> Prepare
    tap("A"); wait(30)                            # pick member
    tap("DOWN", 3, gap=12)
    tap("A"); wait(20)                            # toggle a spell
    tap("B"); tap("B"); tap("B"); wait(20)        # back out
    shot("prepare_done")

@scn
def levelup_check():
    """wizard poked to level 2 without a subclass picks one on deck victory"""
    setup(3, [0, 1, 0, 0, 2])
    for k in range(16): poke(0x0203FF10 + k, 0)
    intro()
    poke(0x03000061, 2); poke(0x03000062, 255)   # level 2, no subclass
    nursery(); surgery()
    walk("DOWN", 1)
    wait(1500); shot("levelup")
    w(f"until {IDLE:08x} 01 24000")

@scn
def wildshape_check():
    """druid poked to level 2 wild-shapes into the boar (Aeon moment)"""
    setup(7, [0, 1, 0, 0, 2])            # CLS_DRUID
    poke(0x0203FF07, 1)                  # manual battles
    intro()
    poke(0x03000061, 2)                  # G.pm[0].level = 2 (wild shape unlocks)
    nursery(); surgery()
    walk("DOWN", 1)                      # deck brawl
    wait(650)                            # cutscene + initiative + first menu
    for _ in range(6):                   # a few turns: DOWN once -> WildShape row
        tap("DOWN"); tap("A", 3, gap=24)
        wait(50); shot("wildshape")      # (re)captures until the boar is on screen
        wait(40)
    poke(0x0203FF07, 0)
    tap("A", 10, gap=30)
    wait(900)

@scn
def panic_check():
    """poke the crash-screen test flag; the report must reach the log"""
    setup(0, [0, 1, 0, 0, 2])
    wait(260)
    poke(0x0203FF0C, 1)
    wait(120)
    shot("panic_screen")

@scn
def tether_check():
    """Manual deck fight: form a melee engagement and see the tether draw.
    The gate asserts the 'engage' and 'tether' log lines."""
    setup(0, [0, 1, 0, 0, 2])
    poke(0x0203FF07, 1)                  # G_MANUAL_BAT: menus are ours
    intro(); nursery(); surgery()
    walk("DOWN", 1)                      # trigger the deck brawl
    wait(600)                            # cutscene + initiative
    tap("A", 60, gap=26)                 # drive Attack->target->confirm, rounds of it
    poke(0x0203FF07, 0)                  # hand the rest back to the AI
    tap("B", 4, gap=20); tap("A", 12, gap=30)
    wait(1200)                           # assertion is the tether log, not the win

@scn
def sneak_strike():
    setup(0, [0, 1, 0, 0, 2])
    sneak_strike_body()

@scn
def cone_ambush():
    setup(3, [0, 1, 0, 0, 2])
    cone_ambush_body()

@scn
def helm_sleepz():
    setup(3, [0, 1, 0, 0, 2], 2)
    poke(0x0203FF0D, 3)              # kill-all stress test at beach-arc strength
    intro(); nursery(); surgery(); deck(); pods()
    walk("UP", 5)
    wait(500); shot("g_zz1")
    wait(400); shot("g_zz2")
    done()
    shot("g_zz_end")

if __name__ == "__main__":
    SCN[sys.argv[1]]()
    print("\n".join(out))

