#!/usr/bin/env python3
"""Capture a labelled screenshot at every story beat for docs/ and review.
Emits a runner script on stdout: python3 test/capture_docs.py > test/docs.script
"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
import scenario as S

S.out = []
S.setup(3, [0, 0, 0, 0, 0])          # wizard, spare Us, save all, connect

# title + crawl
S.w("wait 130"); S.shot("doc_title")
S.w("wait 120"); S.shot("doc_crawl")
S.w("wait 320"); S.shot("doc_class")
S.ready(); S.shot("doc_nursery")

# nursery -> surgery
S.walk("RIGHT", 4); S.walk("DOWN", 3); S.ready(); S.shot("doc_surgery_intro")
S.tap("DOWN"); S.tap("A"); S.ready(); S.shot("doc_us")

# -> deck
S.walk("LEFT", 2); S.walk("DOWN", 7); S.walk("RIGHT", 2); S.walk("DOWN", 1)
S.ready(); S.shot("doc_deck")
S.walk("DOWN", 1)
S.w("wait 520"); S.shot("doc_battle")     # mid imp fight
S.ready(); S.shot("doc_deck_after")

# -> pods
S.walk("DOWN", 8); S.ready(); S.shot("doc_pods_intro")
S.walk("RIGHT", 1); S.walk("DOWN", 4); S.walk("RIGHT", 3); S.ready()
S.walk("LEFT", 7); S.tap("UP"); S.tap("A"); S.ready(); S.shot("doc_shadowheart")

# -> helm
S.walk("RIGHT", 3); S.walk("DOWN", 5); S.ready(); S.shot("doc_helm_room")
S.walk("UP", 5)
S.w("wait 560"); S.shot("doc_helm")       # boss finale
S.done(); S.shot("doc_tally")

print("\n".join(S.out))
