#!/usr/bin/env python3
"""Generate build/gen/assets.c + assets.h from art/music sources in tools/."""
import os, sys

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "build", "gen")

def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    h = ["#ifndef ASSETS_H", "#define ASSETS_H", '#include "gba.h"']
    c = ['#include "assets.h"']
    # (asset sections appended here as the pipeline grows)
    h.append("#endif")
    with open(os.path.join(OUT_DIR, "assets.h"), "w") as f:
        f.write("\n".join(h) + "\n")
    with open(os.path.join(OUT_DIR, "assets.c"), "w") as f:
        f.write("\n".join(c) + "\n")
    print("assets generated")

if __name__ == "__main__":
    main()
