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
BEACH, BEACHF = 0x0203FF04, 0x0203FF2C     # G_DEMO_BEACH / G_BEACH_FLAGS (u16 LE)

# ship-outcome bits (src/game.h GF_*) that shade the beach
GF_LAEZEL, GF_SH_FREED, GF_ZHALK_DEAD, GF_DECK_FOUGHT = 1 << 2, 1 << 3, 1 << 5, 1 << 9

def ready(maxf=9000):
    w(f"until {IDLE:08x} 01 {maxf}")
    _face[0] = 'DOWN'   # room spawns face down
def done(maxf=90000): w(f"until {DONE:08x} 01 {maxf}")

def setup(cls, choices, battle_mode=0):
    poke(DEMO, 1); poke(BATTLE, battle_mode); poke(CLASS, cls)
    poke(0x0203FF0E, 7)   # origin = custom Tav (fixed origins override in their own scenarios)
    # creation screens: 0 = keep the legacy race-none/preset sheet
    # (creation_check pokes real values)
    poke(0x0203FF08, 0); poke(0x0203FF09, 0); poke(0x0203FF0A, 0)
    poke(IDLE, 0); poke(DONE, 0)
    for i, c in enumerate(choices):
        poke(CHOICE + i, c)
    # magic cookie: gba_init wipes the flag block without it (hardware safety)
    for i, b in enumerate((0x01, 0xEE, 0xFF, 0xC0)):
        poke(0x0203FF38 + i, b)

def beach_setup(cls, choices, flags=0, battle_mode=0, origin=7, level=2):
    """Boot straight to the beach wake: the poked G_BEACH_FLAGS stand in for
    the ship's outcome, G_DEMO_LEVEL for its xp. Creation still auto-drives."""
    setup(cls, choices, battle_mode)
    poke(0x0203FF0E, origin)
    poke(0x0203FF0D, level)
    poke(BEACH, 1)
    poke(BEACHF, flags & 0xFF); poke(BEACHF + 1, (flags >> 8) & 0xFF)

def beach_boot():
    """title/crawl/creation auto-advance; the wake fires G_DONE, then field"""
    done(18000)
    ready()
    shot("b_wake")

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
def creation_check():
    """custom wizard walks the creation screens with poked choices: hill
    dwarf, sage, and the class array with CON/INT swapped. The gate greps
    the create line -- race, background, and the ASI'd spread all landed --
    and the run reaches the nursery field-ready."""
    setup(3, [0, 1, 0, 0, 2])
    poke(0x0203FF08, 1)                    # G_DEMO_RACE = hill dwarf
    poke(0x0203FF09, 3)                    # G_DEMO_BG   = sage
    poke(0x0203FF0A, 1)                    # arrange from the poked buffer:
    for i, v in enumerate((8, 13, 15, 14, 12, 10)):   # preset, CON/INT swapped
        poke(0x0203FF20 + i, v)
    intro()

@scn
def skill_check():
    """poked field skill check rolls a visible d20 (asserts the field-check log)"""
    setup(3, [0, 1, 0, 0, 2])
    intro()
    poke(0x0203FF0F, 3)              # G_SKILL_TEST = SK_ARCANA + 1
    wait(80); shot("skill_done")

@scn
def audit_check():
    """dump the per-class battle-menu + sheet ability tables; the gate
    golden-diffs them against test/audit.golden (cross-class leak guard)"""
    setup(0, [0, 1, 0, 0, 2])
    intro()
    poke(0x0203FF0B, 1)              # G_AUDIT: field loop logs both audits
    wait(60)

@scn
def durge_check():
    """Dark Urge origin hears intrusive thoughts (asserts the urge-line log)"""
    poke(DEMO, 1); poke(BATTLE, 0); poke(CLASS, 1); poke(0x0203FF0E, 6)
    for i, byte in enumerate((0x01, 0xEE, 0xFF, 0xC0)): poke(0x0203FF38 + i, byte)
    for k in range(16): poke(0x0203FF10 + k, 0)
    poke(0x0203FF05, 0); poke(0x0203FF06, 0)
    wait(700)
    w(f"until {IDLE:08x} 01 12000")   # nursery reached; wake URGE line already fired
    shot("durge_wake")

@scn
def origin_check():
    """Shadowheart origin: cleric, Masks subclass (id 16) from level 1, her name"""
    poke(DEMO, 1); poke(BATTLE, 0); poke(0x0203FF0E, 4)   # origin = Shadowheart
    for i, byte in enumerate((0x01, 0xEE, 0xFF, 0xC0)): poke(0x0203FF38 + i, byte)
    poke(0x0203FF05, 0); poke(0x0203FF06, 0)
    wait(700)                          # title + crawl + class/chooser auto-drive
    w(f"until {IDLE:08x} 01 12000")    # reach the nursery
    shot("origin_shadow")

@scn
def origin_flow_check():
    """class-first chooser: auto-pick Cleric as a CLASS, then take the
    chooser's 'Play <origin>' row. No origin index is poked -- 8 means 'this
    class's origin row', so the cleric->Shadowheart mapping is computed by
    game_origin_choose itself (asserts start: origin=4 class=5 sub=16)."""
    poke(DEMO, 1); poke(BATTLE, 0); poke(CLASS, 5)        # Cleric, by class
    poke(0x0203FF0E, 8)                # take the class's origin row
    for i, byte in enumerate((0x01, 0xEE, 0xFF, 0xC0)): poke(0x0203FF38 + i, byte)
    poke(0x0203FF05, 0); poke(0x0203FF06, 0)
    wait(700)                          # title + crawl + class/chooser auto-drive
    w(f"until {IDLE:08x} 01 12000")    # reach the nursery
    shot("origin_flow")

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
def cone_show():
    """The pods prowler's vision cone surfaces as garnish: loitering at
    (15,1) -- 16..24px behind it, inside the 1.5x activation band but past
    its halved 14px rear reach -- draws the marching-ant boundary (bright
    forward, dim rear) WITHOUT tripping it (the gate asserts the one-shot
    'cone shown' log). Then a hook around into (15,3), 16..24px down its
    gaze and inside the drawn edge at any drift: the usual ambush resolves,
    proving the boundary shown is the boundary used."""
    setup(3, [0, 1, 0, 0, 2])
    _to_deck_cleared()
    walk("RIGHT", 7)                      # (15,1): the whole row is rear --
    wait(30); shot("g_cone")              #   peripheral, never detected
    wait(45); shot("g_cone2")             # ants have marched; still unseen
    walk("LEFT", 1); walk("DOWN", 2)      # flank hook: (14,3)
    walk("RIGHT", 1)                      # (15,3): cross the drawn edge
    wait(90)                              # it sees us; the chase closes
    ready(24000)                          # ambush battle resolves
    wait(30)

# --- the Ravaged Beach (stone 2) -----------------------------------------

@scn
def beach_full():
    """The whole arc so far: ship (bard, everyone saved) -> crash -> wake
    alone -> Shadowheart rejoins ashore -> strike the crash-site devourer
    from behind -> dune path -> free Lae'zel from the scavenger cage."""
    setup(0, [0, 0, 0, 0, 0, 0])       # ship picks + cage "Open the cage"
    intro(); nursery(); surgery(); deck(); pods(); helm()
    ready()                            # the beach is playable after the wake
    shot("b_shadowheart")              # she stands ashore at (11,6)
    face_interact("RIGHT")             # ...and rejoins
    walk("UP", 5); walk("RIGHT", 5)    # (15,1): behind the devourer's cone
    tap("DOWN"); tap("A")              # strike first
    ready(24000)
    shot("b_devourer")
    walk("LEFT", 5); walk("UP", 1)     # the dune gap (10,0)
    ready()
    shot("b_dunes")
    walk("UP", 4); walk("LEFT", 6)     # (4,6): west corridor
    walk("UP", 4); walk("LEFT", 1)     # (3,2): beside the cage
    face_interact("LEFT")              # the cage beat; Lae'zel rejoins
    shot("b_cage")

@scn
def beach_medicine():
    """Poked wake, Shadowheart LEFT BEHIND on the ship: she lies at the
    tide line and a Medicine field check wakes her (she joins either way)."""
    beach_setup(0, [0, 0, 0, 0], flags=GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    walk("DOWN", 1); walk("LEFT", 5)   # (5,7), beside the body
    face_interact("LEFT")
    shot("b_medic")

@scn
def beach_flayer():
    """Poked wake, Shadowheart FREED on the ship: she rejoins with a word
    (the other recovery fork); the dying mind flayer grasps at your mind
    (Arcana field check), gets finished; a dune devourer falls to the pair."""
    beach_setup(0, [0, 0, 0, 0], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    face_interact("RIGHT")             # Shadowheart, upright at (11,6)
    walk("LEFT", 4); walk("UP", 2)     # (6,4): within the flayer's reach
    ready()
    shot("b_flayer")
    walk("RIGHT", 4); walk("UP", 4)    # the dune gap
    ready()
    walk("UP", 4); walk("RIGHT", 2)    # (12,6): behind the dune devourer
    tap("DOWN"); tap("A")              # strike first
    ready(24000)
    shot("b_dunefight")

@scn
def beach_origin():
    """Origin Shadowheart wakes on the beach: her recovery beat must not
    exist (story surgery), and stranger Lae'zel still gets freed."""
    beach_setup(5, [0, 0, 0, 0], flags=0, origin=4)
    beach_boot()
    walk("UP", 6)                      # straight up the sand to the gap
    ready()
    walk("UP", 4); walk("LEFT", 6)
    walk("UP", 4); walk("LEFT", 1)
    face_interact("LEFT")              # the cage, stranger fork
    shot("b_origin")

# --- the Ravaged Beach (stone 3): the recruits ----------------------------

@scn
def beach_recruits():
    """The roster outgrows the walking three. Shadowheart rejoins ashore;
    Astarion's knife-at-the-throat beat fills the third walking slot; the
    cage sends Lae'zel to reserve 0; Gale arrives through the portal sigil
    into reserve 1 (the bench is now full: five souls). Then the Start
    menu's Party row -- live for the first time in real play -- swaps
    Lae'zel in for Shadowheart, on-ROM, via scripted input."""
    beach_setup(0, [0, 0, 0, 0], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    face_interact("RIGHT")             # Shadowheart, upright at (11,6)
    walk("LEFT", 8)                    # the pale elf beckons: beat fires at (3,6)
    ready()
    shot("b_astarion")
    walk("RIGHT", 7)                   # back to (10,6)
    walk("UP", 6)                      # the dune gap
    ready()
    walk("UP", 4); walk("LEFT", 6)     # (4,6): west corridor
    walk("UP", 4); walk("LEFT", 1)     # (3,2), beside the cage
    face_interact("LEFT")              # Lae'zel freed -> reserve slot 0
    walk("DOWN", 1)                    # (3,3)
    walk("RIGHT", 6); walk("RIGHT", 6) # (15,3): east along the north corridor
    walk("DOWN", 4)                    # (15,7), above the sigil
    face_interact("DOWN")              # the hand in the stone: Gale -> slot 1
    shot("b_gale")
    tap("START"); wait(20)             # five souls: the Party row is live
    tap("DOWN", 5, gap=12); tap("A"); wait(20)   # -> Party
    tap("A"); wait(20)                 # walking slot 1 (Shadowheart)...
    tap("A"); wait(30)                 # ...swaps with reserve 0 (Lae'zel)
    shot("b_party_swap")
    tap("B"); wait(10); tap("B"); wait(20)       # back out to the field

@scn
def beach_reroute_astarion():
    """Origin Astarion: no pale stranger waits by the grass -- story
    surgery leaves his staked kill there instead, and the hunger beat
    plays (Feed)."""
    beach_setup(1, [0], flags=0, origin=0)
    beach_boot()
    walk("LEFT", 8)                    # (2,6), beneath the staked boar
    face_interact("UP")                # the hunger beat
    shot("b_reroute_ast")

@scn
def beach_reroute_gale():
    """Origin Gale: the sigil stone holds no one -- the wizard it would
    deliver is already standing in front of it, unimpressed."""
    beach_setup(3, [0], flags=0, origin=1)
    beach_boot()
    walk("UP", 6)                      # the dune gap
    ready()
    walk("RIGHT", 5)                   # (15,10): the east edge
    walk("UP", 2)                      # (15,9); the sigil stone blocks step 2
    face_interact("UP")                # a professional opinion
    shot("b_reroute_gale")

# --- stone 4: the chapel, the crypt, darkvision's debut, Withers ----------

def _to_chapel():
    """wake -> Shadowheart -> the cage (Lae'zel) -> the cleared north pass.
    Consumes ONE choice (the cage's 'Open the cage')."""
    face_interact("RIGHT")             # Shadowheart, upright at (11,6)
    walk("UP", 6)                      # the dune gap
    ready()
    walk("UP", 4); walk("LEFT", 6)     # (4,6): west corridor
    walk("UP", 4); walk("LEFT", 1)     # (3,2), beside the cage
    face_interact("LEFT")              # Lae'zel rejoins (choice 0)
    walk("UP", 1)                      # (3,1)
    walk("RIGHT", 4)                   # (7,1), below the pass
    walk("UP", 1)                      # the cleared rockfall -> chapel
    ready()

@scn
def chapel_fight():
    """The looter band, the hard way: 'Draw steel' ends the parley, three
    SRD bandits fall, and the split-sealed tomb door grinds open. The band
    blocks the central approach, so a UP-facing A meets them first."""
    beach_setup(4, [0, 2, 1], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    _to_chapel()
    shot("c_chapel")
    walk("UP", 4)                      # (7,4), nose-to-nose with the band
    face_interact("UP")                # choice 1 = Draw steel -> the fight
    shot("c_fight")
    walk("UP", 2)                      # (7,2), beneath the door
    face_interact("UP")                # opens; choice 2 = Not yet (stay)
    shot("c_door")

@scn
def chapel_parley():
    """The check-past fork: the bard talks (Persuasion, rolled in the
    open). Pass, the band bolts for less xp; fail, the fight happens --
    either way the beat resolves and the door opens, and the crypt beyond
    dims for a no-darkvision Tav (dark room dim=1)."""
    beach_setup(0, [0, 1, 0], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    _to_chapel()
    walk("UP", 4)                      # (7,4): up to the arguing band
    face_interact("UP")                # choice 1 = Persuade (rolled in the open)
    shot("c_parley")
    walk("UP", 2)
    face_interact("UP")                # open + choice 2 = Descend into the dark
    ready()
    shot("c_dark")                     # the crypt dims (dark room dim=1)

@scn
def crypt_withers():
    """The crypt, end to end: DARK entry hall (screen dims for a
    no-darkvision Tav), the ossuary ambush (SRD skeletons rise around the
    party), and Withers -- wake beat, then the subclass re-pick service.
    Tav is origin Wyll (warlock, Fiend subclass from level 1), so there is
    a real chosen path for Withers to unmake and the machinery to re-make.
    Shadowheart and Lae'zel still recover (Wyll has no beach reroute)."""
    beach_setup(11, [0, 0, 0, 0, 0, 0, 0, 0],
                flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT, origin=5)
    beach_boot()
    _to_chapel()
    walk("UP", 4)                      # up to the band
    face_interact("UP")                # choice 0 = Intimidate: they bolt
    walk("UP", 2)                      # (7,2)
    face_interact("UP")                # opens; choice 0 = Descend -> the crypt
    ready()
    shot("w_crypt")                    # DARK marker: dim=1 in the log
    walk("UP", 7)                      # up column 6 onto the north arch
    ready()                            # arch -> ossuary (spawn 6,7)
    walk("UP", 3)                      # (6,4): the bones knit upright
    ready(24000)
    shot("w_bones")
    walk("UP", 4)                      # onto the north arch -> sanctum
    ready()
    walk("UP", 3)                      # (6,4), at the sarcophagus foot
    face_interact("UP")                # wake beat: open (0), answer (0)
    shot("w_withers")
    tap("DOWN"); tap("RIGHT"); tap("A")  # Withers, in office beside the tomb
    ready()                            # service: change path (0), member (0)
    shot("w_repick")

@scn
def warryn_check():
    """The masked looter: stand beside him and the tadpole stirs (one
    flavor line, never explained); talking gives the masked one-liner; the
    parley fork grows an [Illithid] lever that auto-passes -- the band
    walks, the masked one first. Then the tomb door still opens."""
    beach_setup(4, [0, 0, 1], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    _to_chapel()                       # cage choice (0)
    walk("UP", 4)                      # (7,4), below the band
    walk("LEFT", 2)                    # (5,4)
    walk("UP", 1)                      # (5,3): adjacent -- the stir fires
    ready()
    shot("y_warryn")
    face_interact("UP")                # the masked one-liner
    walk("DOWN", 1); walk("RIGHT", 2)  # back to (7,4)
    face_interact("UP")                # parley: choice 0 = [Illithid] LEAVE.
    shot("y_lever")
    walk("UP", 2)                      # (7,2), beneath the door
    face_interact("UP")                # door opens; choice 1 = Not yet
    shot("y_door")

# --- stone 5: the camp night ----------------------------------------------

@scn
def camp_night():
    """First arrival at camp (the chapel yard's east gap): the survivors
    settle around the fire and Under Selune plays as a story scene --
    the lyric-sync log and the demo autoskip are the structural marks.
    Then the campfire rest: Tav poked to 1 hp is healed back to full
    (camp rest from=1 full=1), for zero xp. Finally the round trip:
    leave for the yard, walk back in -- the scene must NOT replay (the
    gate counts exactly one 'camp scene begins')."""
    beach_setup(0, [0, 0], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    _to_chapel()                       # Shadowheart + Lae'zel walk with us
    walk("RIGHT", 6)                   # (13,8)
    walk("UP", 4)                      # (13,4)
    walk("RIGHT", 2)                   # through the east gap (15,4) -> camp
    wait(2000); shot("n_scene")        # mid-scene: verse 1 over the tableau
    ready()                            # the scene ends (demo autoskip)
    shot("n_camp")                     # the night tableau: fire + companions
    poke(0x0300007A, 1); poke(0x0300007B, 0)   # G.pm[0].hp = 1 (s16 LE, G+34)
    walk("RIGHT", 4)                   # (5,4), beside the fire circle
    walk("UP", 1); walk("RIGHT", 2)    # (7,3), north of the fire
    face_interact("DOWN")              # rest (choice 0 = Rest): full heal
    shot("n_rest")
    walk("LEFT", 1)                    # (6,3), above Shadowheart's spot
    face_interact("DOWN")              # a word by the fire (camp talk log)
    walk("LEFT", 5)                    # (1,3)
    walk("DOWN", 1); walk("LEFT", 1)   # (0,4): back up to the chapel yard
    ready()
    walk("RIGHT", 1)                   # straight back through the gap
    ready()                            # second arrival: no scene, just night
    shot("n_replay")

# --- stone 6: the grove gates ----------------------------------------------

def _to_gates():
    """chapel spawn (7,8) -> the west gap (0,4), skirting the looter band"""
    walk("LEFT", 6)                    # (1,8)
    walk("UP", 4)                      # (1,4)
    walk("LEFT", 1)                    # (0,4): the grove road
    ready()

@scn
def gates_wyll():
    """The finale battle, canonical: arrive with Shadowheart + Lae'zel,
    find Wyll dueling out front and Zevlor at the door, win the assault
    with both allies on side 2, and recruit the sixth soul to the bench."""
    beach_setup(0, [0], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT)
    beach_boot()
    _to_chapel()                       # cage choice (0)
    _to_gates()
    shot("g_gates")                    # the assault tableau, pre-battle
    walk("LEFT", 4)                    # (10,4): close with the fight
    ready(30000)                       # battle + victory beat + recruit
    shot("g_won")
    walk("RIGHT", 3); walk("DOWN", 2)  # (13,6), above the Hellrider cache
    face_interact("DOWN")
    shot("g_cache")

@scn
def gates_reroute_wyll():
    """Origin Wyll: the Blade of Frontiers is the player, so no duelist
    waits out front -- the battle runs with Zevlor alone on side 2 and the
    victory beat shades to Zevlor greeting the legend himself."""
    beach_setup(11, [0], flags=GF_SH_FREED | GF_LAEZEL | GF_DECK_FOUGHT,
                origin=5)
    beach_boot()
    _to_chapel()                       # cage choice (0)
    _to_gates()
    shot("g_reroute")
    walk("LEFT", 4)                    # close with the fight
    ready(30000)
    shot("g_reroute_won")

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

