#!/usr/bin/env python3
"""Emit a runner script that captures frames for the README demo GIF.
Usage: python3 test/capture_gif.py > test/gif.script
Frames land as test/shots/gif_NNN.ppm in chronological order.
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
import scenario as S

S.out = []
S.setup(0, [0, 0, 0, 0, 0])          # bard, spare Us, save everyone, connect

_n = 0
def snap():
    global _n
    S.w(f"shot test/shots/gif_{_n:03d}.ppm")
    _n += 1

def burst(count, every):
    for _ in range(count):
        S.wait(every)
        snap()

S.w("wait 100")
burst(2, 30)          # title, nautiloid bobbing
burst(8, 22)          # prologue crawl pages, dragon flyby
burst(3, 30)          # class select / name
S.ready()
burst(3, 8)           # nursery wake
S.walk("RIGHT", 4)
snap()
S.walk("DOWN", 3)     # -> surgery
S.ready()
S.tap("DOWN"); S.tap("A")
burst(4, 30)          # Us extraction scene
S.ready()
S.walk("LEFT", 2); S.walk("DOWN", 7); S.walk("RIGHT", 2); S.walk("DOWN", 1)
S.ready()
S.walk("DOWN", 1)     # Lae'zel + imp fight
burst(22, 12)         # meeting, battle actions, popups
S.ready()
snap()
S.walk("DOWN", 8)     # -> pods
S.ready()
S.walk("RIGHT", 1); S.walk("DOWN", 4); S.walk("RIGHT", 3)
S.ready()
S.walk("LEFT", 7)
S.tap("UP"); S.tap("A")
burst(6, 24)          # rune console, Shadowheart pod bursts
S.ready()
S.walk("RIGHT", 3); S.walk("DOWN", 5)
S.ready()
S.walk("UP", 5)       # the helm finale
burst(24, 13)         # boss battle, nerves, crash
S.done()
burst(3, 20)          # beach + tally

print("\n".join(S.out))
