#!/bin/sh
# Opens the built EBOOT in PPSSPP. Uses PPSSPPSDL from PATH, or the flatpak.
set -e
cd "$(dirname "$0")"
[ -f EBOOT.PBP ] || ./build.sh
if command -v PPSSPPSDL >/dev/null 2>&1; then
	exec PPSSPPSDL "$PWD/EBOOT.PBP"
fi
exec flatpak run org.ppsspp.PPSSPP "$PWD/EBOOT.PBP"
