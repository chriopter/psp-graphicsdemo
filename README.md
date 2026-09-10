# PSP Graphics Demo

A homebrew demo for the PlayStation Portable that walks through what the
PSP's Graphics Engine can do, one technique per scene. Every scene is a
condensed, running version of one of the [pspsdk GU samples](https://github.com/pspdev/pspsdk/tree/master/src/samples/gu),
so the source doubles as a tour of the `sceGu` API. Tested in **PPSSPP 1.20.4**;
nobody has run it on real hardware yet.

![All thirteen scenes, two and a half seconds each](docs/demo.gif)

The full run, thirty frames per second: [docs/demo.mp4](docs/demo.mp4).

## Scenes

| # | Scene | What the GE is doing | Sample |
|---|---|---|---|
| 1 | Textured cube | `sceGumDrawArray`, a 4-bit-alpha texture added onto vertex colours | cube |
| 2 | Hardware lights | four point lights, diffuse and specular, lit per vertex | lights |
| 3 | Environment map | `GU_ENVIRONMENT_MAP`, the 2×2 matrix rides in light slots 2 and 3 | envmap |
| 4 | Cel shading | a 1D lightmap sampled through envmap coordinates, inflated hull for the outline | celshading |
| 5 | Morph targets | `GU_VERTICES(2)`, sphere and cube blended by `sceGuMorphWeight` | morph |
| 6 | Matrix skinning | eight bone matrices, cubic B-spline weights per vertex | skinning |
| 7 | Spline surface | `sceGumDrawSpline` tessellates an 18×18 control net on the GE | splinesurface |
| 8 | Render to texture | a torus drawn into a 128×128 VRAM target, then mapped onto a cube | rendertarget |
| 9 | Stencil mirror | the mirror marks the stencil, the flipped cube draws only inside it | reflection |
| 10 | Projected shadow | a shadow map from the light's view, projected through `GU_TEXTURE_MATRIX` | shadowprojection |
| 11 | Depth buffer fog | the z-buffer read back as an 8-bit texture through a fog palette | zbufferfog |
| 12 | Sprite cloud | 16384 `GU_SPRITES` billboards, alpha test cuts out the ball | sprite |
| 13 | Palette cycling | an 8-bit XOR texture whose 256-entry CLUT is rewritten every frame | clut |

Scenes advance on their own every eight seconds and loop.

## Controls

| PSP | Action |
|---|---|
| L / R, or D-pad left / right | previous / next scene |
| Cross | hold the current scene, press again to resume |
| Start, or Home | exit |

## Downloads

Get `psp-graphicsdemo.zip` from the [latest release](https://github.com/chriopter/psp-graphicsdemo/releases/latest).
It contains `PSP/GAME/GraphicsDemo/EBOOT.PBP`; copy the `PSP` folder to a
Memory Stick, or open the EBOOT in PPSSPP. The zip is what
[PSPDX](https://github.com/chriopter/pspdx) installs from `app.pspdx`.

Pushing a `v*` tag builds the same zip in CI and attaches it to a release.

## Building

Requirements: Docker. The pspdev toolchain image does the rest.

```sh
./build.sh          # -> EBOOT.PBP
./run-ppsspp.sh     # opens it in PPSSPPSDL or the PPSSPP flatpak
tools/package.sh    # -> dist/psp-graphicsdemo.zip
```

Without Docker, `make` works against any pspdev install that has
`psp-config` on the PATH.

## How the recording is made

PPSSPP's headless build cannot save screenshots by itself, so
`tools/record.mjs` drives its WebSocket debugger: a breakpoint on the
`sceDisplaySetFrameBuf` stub fires once per presented frame, the registers
say which VRAM buffer went to the screen, and `memory.read` pulls it out.
`tools/make-video.sh` runs the whole demo that way and renders the MP4 and
GIF with ffmpeg. Needs PPSSPPHeadless, node 22+, ffmpeg and Docker.

## Layout

```
src/main.c         GU setup, scene loop, overlay, controls
src/scene_*.c      one technique each, init() builds geometry, draw(frame) renders
src/text.c         2D text from the pspsdk font sheet
src/geometry.c     torus and grid generators from the pspsdk samples
assets/*.raw       textures from the samples, linked with bin2o
tools/             recorder, video, packaging
```

## License

BSD 3-Clause, see [LICENSE](LICENSE). The scene code and textures are
adapted from the pspsdk samples, which carry the same license.
