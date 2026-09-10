#!/bin/sh
# Zips the built EBOOT as PSP/GAME/GraphicsDemo/, the layout a Memory Stick
# and PPSSPP expect. Writes dist/psp-graphicsdemo.zip and its sha256.
set -e
cd "$(dirname "$0")/.."
[ -f EBOOT.PBP ] || { echo "build first: ./build.sh" >&2; exit 1; }
rm -rf dist && mkdir -p dist/PSP/GAME/GraphicsDemo
cp EBOOT.PBP dist/PSP/GAME/GraphicsDemo/
cp LICENSE dist/PSP/GAME/GraphicsDemo/LICENSE.txt
(cd dist && zip -q -r -X psp-graphicsdemo.zip PSP)
rm -rf dist/PSP
sha256sum dist/psp-graphicsdemo.zip | tee dist/psp-graphicsdemo.zip.sha256
