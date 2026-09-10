#!/bin/sh
# Records the demo in PPSSPPHeadless and renders docs/demo.mp4 (30 fps, the
# whole run) and docs/demo.gif (a shorter loop for the README).
#
# PPSSPP's headless build cannot save screenshots and its debugger is racy,
# so the demo records itself: with ms0:/PSP/GRAPHICSDEMO.REC present it
# appends every second frame raw to ms0:/PSP/GRAPHICSDEMO.RAW and exits when
# done (see record_open in src/main.c). Frames are exact emulated vblanks,
# so the timing does not depend on host speed.
#
#   tools/make-video.sh                  full run, 13 scenes x 8 s
#   FRAMES=960 tools/make-video.sh       shorter test run
#
# Needs PPSSPPHeadless, ffmpeg, and the EBOOT built (./build.sh).
# PPSSPP_MEMSTICK points at the headless memory stick root, ~/.ppsspp by default.
set -e
cd "$(dirname "$0")/.."
[ -f graphicsdemo.elf ] || ./build.sh
MS=${PPSSPP_MEMSTICK:-$HOME/.ppsspp}
SCENES=13
SCENE_FRAMES=480
FRAMES=${FRAMES:-$((SCENES * SCENE_FRAMES))}
STEP=${STEP:-2}
OFFSET=${OFFSET:-0}
mkdir -p "$MS/PSP" docs
rm -f "$MS/PSP/GRAPHICSDEMO.RAW"
printf '%d %d %d' "$FRAMES" "$STEP" "$OFFSET" > "$MS/PSP/GRAPHICSDEMO.REC"
PPSSPPHeadless "$PWD/graphicsdemo.elf" --graphics=software --timeout=3600 >/dev/null 2>&1 || true
rm -f "$MS/PSP/GRAPHICSDEMO.REC"
RAW="$MS/PSP/GRAPHICSDEMO.RAW"
[ -s "$RAW" ] || { echo "no frames recorded" >&2; exit 1; }
echo "$(( $(stat -c %s "$RAW") / (512 * 272 * 4) )) frames in $RAW"
[ "$1" = "--frames-only" ] && exit 0
IN="-f rawvideo -pix_fmt rgba -video_size 512x272 -framerate 30 -i $RAW"
# mp4: native PSP resolution, 30 fps
ffmpeg -y -loglevel error $IN -vf "crop=480:272:0:0,format=yuv420p" \
  -c:v libx264 -crf 27 -preset slow -movflags +faststart docs/demo.mp4
# gif: 2.5 s from each scene at 12 fps, native size, so the README stays light
FILTER=""; CONCAT=""; i=0
while [ $i -lt $SCENES ]; do
  start=$(( (i * SCENE_FRAMES + 120) / STEP ))
  FILTER="$FILTER[0:v]trim=start_frame=$start:end_frame=$((start + 75)),setpts=PTS-STARTPTS[s$i];"
  CONCAT="$CONCAT[s$i]"
  i=$((i + 1))
done
ffmpeg -y -loglevel error $IN -filter_complex \
  "${FILTER}${CONCAT}concat=n=$SCENES:v=1:a=0,crop=480:272:0:0,format=rgb24,fps=12,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=3" \
  docs/demo.gif
ls -la docs/demo.mp4 docs/demo.gif
