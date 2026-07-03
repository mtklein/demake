"""Minimal PNG writer (RGB8), stdlib only."""
import struct, zlib

def _chunk(tag, data):
    c = tag + data
    return struct.pack(">I", len(data)) + c + struct.pack(">I", zlib.crc32(c))

def write_png(path, w, h, rgb):
    """rgb: bytes/bytearray of w*h*3 RGB triples, row-major."""
    assert len(rgb) == w * h * 3, (len(rgb), w, h)
    raw = b"".join(b"\x00" + bytes(rgb[y*w*3:(y+1)*w*3]) for y in range(h))
    png = (b"\x89PNG\r\n\x1a\n"
           + _chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
           + _chunk(b"IDAT", zlib.compress(raw, 6))
           + _chunk(b"IEND", b""))
    with open(path, "wb") as f:
        f.write(png)

def write_png_scaled(path, w, h, rgb, scale):
    """Nearest-neighbor upscale for readability."""
    out = bytearray()
    for y in range(h):
        row = bytearray()
        for x in range(w):
            px = rgb[(y*w+x)*3:(y*w+x)*3+3]
            row += px * scale
        out += row * scale
    write_png(path, w*scale, h*scale, out)
