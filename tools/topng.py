#!/usr/bin/env python3
"""Convert binary PPM (P6) to PNG (2x upscaled): topng.py file.ppm [out.png]"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from pnglib import write_png_scaled

def main(src, dst=None):
    with open(src, "rb") as f:
        data = f.read()
    assert data[:2] == b"P6", "not a P6 ppm"
    parts = data.split(b"\n", 3)
    w, h = map(int, parts[1].split())
    rgb = parts[3][: w * h * 3]
    write_png_scaled(dst or src.rsplit(".", 1)[0] + ".png", w, h, rgb, 2)

if __name__ == "__main__":
    main(*sys.argv[1:])
