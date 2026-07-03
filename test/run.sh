#!/bin/sh
# usage: test/run.sh SCRIPT [ROM]  — run scripted emulator session, convert shots to png
set -e
cd "$(dirname "$0")/.."
ROM="${2:-build/nautiloid.gba}"
make -s all runner
mkdir -p test/shots
./build/runner "$ROM" "$1"
for f in test/shots/*.ppm; do
    [ -e "$f" ] || continue
    python3 tools/topng.py "$f" && rm "$f"
done
