#!/usr/bin/env python3
"""Patch GBA cartridge header: logo, title, fixed byte, complement check."""
import sys

LOGO = bytes.fromhex(
    "24FFAE51699AA2213D84820A84E409AD11248B98C0817F21A352BE199309CE20"
    "10464A4AF82731EC58C7E83382E3CEBF85F4DF94CE4B09C194568AC01372A7FC"
    "9F844D73A3CA9A615897A327FC039876231DC7610304AE56BF38840040A70EFD"
    "FF52FE036F9530F197FBC08560D68025A963BE03014E38E2F9A234FFBB3E0344"
    "780090CB88113A9465C07C6387F03CAFD625E48B380AAC7221D4F807")
assert len(LOGO) == 156

def main(path, title):
    with open(path, "rb") as f:
        rom = bytearray(f.read())
    while len(rom) % 4:
        rom.append(0)

    rom[0x04:0xA0] = LOGO
    t = title.upper().encode("ascii")[:12]
    rom[0xA0:0xAC] = t + bytes(12 - len(t))
    rom[0xAC:0xB0] = b"ANLE"          # game code
    rom[0xB0:0xB2] = b"01"            # maker code
    rom[0xB2] = 0x96                  # fixed value
    for i in range(0xB3, 0xBD):
        rom[i] = 0
    chk = 0
    for i in range(0xA0, 0xBD):
        chk += rom[i]
    rom[0xBD] = (-(0x19 + chk)) & 0xFF
    rom[0xBE] = rom[0xBF] = 0

    with open(path, "wb") as f:
        f.write(rom)
    print(f"fixed {path}: {len(rom)} bytes, title={t.decode()}")

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else "NAUTILOID")
