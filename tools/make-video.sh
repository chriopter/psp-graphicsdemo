#!/bin/sh
# Records the whole demo once through in PPSSPPHeadless and renders
# docs/demo.mp4 (30 fps, every scene) and docs/demo.gif (a shorter loop for
# the README). Needs Docker (for psp-nm), PPSSPPHeadless, node 22+, ffmpeg.
#
#   tools/make-video.sh            full run, about 13 scenes x 8 s
#   FRAMES=960 tools/make-video.sh  shorter test run
set -e
cd "$(dirname "$0")/.."
[ -f graphicsdemo.elf ] || ./build.sh
STUB=$(docker run --rm -v "$PWD:/src" -w /src pspdev/pspdev:latest psp-nm graphicsdemo.elf | awk '$3=="sceDisplaySetFrameBuf"{print $1}')
[ -n "$STUB" ] || { echo "no sceDisplaySetFrameBuf stub in graphicsdemo.elf" >&2; exit 1; }
SCENES=13
SCENE_FRAMES=480
FRAMES=${FRAMES:-$((SCENES * SCENE_FRAMES))}
rm -rf frames && mkdir -p frames docs
STUB=$STUB node tools/record.mjs "$PWD/graphicsdemo.elf" frames "$FRAMES" 2 0
# mp4: 30 fps, PSP resolution doubled so players do not blur it
ffmpeg -y -loglevel error -framerate 30 -i frames/frame_%05d.png \
  -vf "scale=960:544:flags=neighbor,format=yuv420p" -c:v libx264 -crf 20 -movflags +faststart docs/demo.mp4
# gif: 2.5 s from each scene at 12 fps, native size, so the README stays light
FILTER=""
i=0
while [ $i -lt $SCENES ]; do
  start=$(( (i * SCENE_FRAMES + 120) / 2 ))
  FILTER="$FILTER[0:v]trim=start_frame=$start:end_frame=$((start + 75)),setpts=PTS-STARTPTS[s$i];"
  i=$((i + 1))
done
CONCAT=""
i=0; while [ $i -lt $SCENES ]; do CONCAT="$CONCAT[s$i]"; i=$((i + 1)); done
ffmpeg -y -loglevel error -framerate 30 -i frames/frame_%05d.png -filter_complex \
  "${FILTER}${CONCAT}concat=n=$SCENES:v=1:a=0,fps=12,split[a][b];[a]palettegen=max_colors=128:stats_mode=diff[p];[b][p]paletteuse=dither=bayer:bayer_scale=3" \
  docs/demo.gif
ls -la docs/demo.mp4 docs/demo.gif
