# Ideas: shader-style effects on a fixed-function GE

The Graphics Engine has no programmable shaders. What it has is a set of
fixed operations that can be chained across passes: any VRAM address can be
read back as a texture, the framebuffer and the depth buffer included; the
palette (CLUT) turns an 8- or 16-bit value into an arbitrary colour, which
makes it a 256-entry lookup function; the blend unit does add, subtract,
multiply, min, max and absolute difference; alpha test and stencil throw
pixels away, which is the only conditional there is; and the vertex side is
fully programmable anyway, because the VFPU can transform vertices before
the GE ever sees them.

Scenes 8 (render to texture), 11 (depth buffer through a palette) and 13
(palette cycling) already show the building blocks. The list below is what
they could be assembled into, sorted by how many full-screen passes each one
costs. A full-screen pass at 480×272 is roughly 0.3 to 0.8 ms depending on
how much texture reading it does, so a 30 Hz scene can afford ten to fifteen
of them and a 60 Hz scene about half that.

## Cheap: one to three passes

- **Per-channel colour grading.** Read the 16-bit framebuffer back as a
  `GU_PSM_T16` texture, use `sceGuClutMode` shift and mask to isolate one
  channel, send it through a palette, draw additively. Three passes, one per
  channel. Sepia, night vision, tone curves, posterize and correctly weighted
  greyscale all fall out of the palette contents. Largest effect per line of
  code of anything here.
- **Motion blur and trails.** Blend the previous frame over the new one at
  70 to 90 % alpha. One pass; needs a second colour buffer to keep the
  history.
- **Pixelate, scanlines, CRT.** Render small and scale up with
  `GU_NEAREST`, then multiply an overlay texture on top. One or two passes.
- **Vignette, flash, colour filter.** One textured or flat quad with the
  right blend mode.

## Medium: four to eight passes

- **Bloom.** Downsample the frame to a quarter with bilinear filtering,
  threshold through a palette or alpha test, blur with two to four additive
  passes offset by a pixel, add the result back. The signature effect of
  the PSP era.
- **Edge detection.** Draw the frame over itself shifted by one pixel with
  the absolute-difference blend (`GU_ABS`), once horizontally and once
  vertically, then threshold through a palette. Gives outlines for a comic
  look and a Sobel-ish filter for free.
- **Heat haze, water refraction, shock waves.** Render the scene to a
  texture, then draw a grid whose UVs the VFPU has perturbed, sampling that
  texture. The distortion is arbitrary per grid vertex rather than per
  pixel.
- **Emboss bump mapping.** Draw a height texture twice, slightly offset,
  with subtractive blending. The 1999 trick, and it works here unchanged.
- **Cel shading with a real light.** Scene 4 already does the ramp through
  envmap coordinates; a second pass with the inflated hull in black is the
  outline. What is missing is a light that moves per frame, which is a
  matrix update, not a pass.

## Expensive but possible

- **Depth of field.** Read the depth buffer as an index texture the way
  scene 11 does, but map depth to alpha instead of fog colour, and use that
  alpha to blend in a blurred copy of the frame from the bloom chain. Eight
  to ten passes.
- **Stencil shadow volumes.** The 8-bit stencil in `GU_PSM_8888` mode is
  enough. Extrude silhouettes on the VFPU, count front and back faces into
  the stencil, darken where the count is non-zero.
- **Shadow mapping proper.** Fails on the missing depth compare when
  sampling. It can in principle be rebuilt from subtractive blending and
  alpha test, but the cost puts it out of reach for a 30 Hz scene. Scene 10's
  projected shadow is the practical answer.

## What does not work

- Normal mapping with a per-pixel dot product. There is no Dot3 stage.
- Anything that turns a pixel value into a texture coordinate, except the
  256-entry palette lookup.
- Loops, branches and floating point in the pixel path. After about five
  passes at 8 bits per channel the banding shows.

## The real limit is VRAM

Double buffer plus depth already takes over 800 KB of the 2 MB, which
leaves about a megabyte for ping-pong targets and textures. Anything beyond
that renders into main RAM and pays in bandwidth. A scene that wants both
bloom and depth of field will have to share intermediate buffers.
