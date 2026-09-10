/*
 * PSP Graphics Demo - one binary that walks through the drawing techniques
 * of the pspsdk GU samples, scene by scene.
 *
 * Scenes adapt code from src/samples/gu in the pspsdk (BSD license,
 * copyright the pspdev contributors, see LICENSE).
 */
#ifndef DEMO_H
#define DEMO_H

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <pspgu.h>
#include <pspgum.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#define BUF_WIDTH  512
#define SCR_WIDTH  480
#define SCR_HEIGHT 272

/* VRAM layout, offsets relative to sceGeEdramGetAddr() */
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * 4)   /* 8888 */
#define ZBUF_SIZE  (BUF_WIDTH * SCR_HEIGHT * 2)   /* 16 bit */
#define VRAM_FB0   0
#define VRAM_FB1   FRAME_SIZE
#define VRAM_ZBUF  (2 * FRAME_SIZE)
#define VRAM_RT    (2 * FRAME_SIZE + ZBUF_SIZE)   /* 128x128 8888 offscreen target */
#define RT_SIZE    128

typedef struct Scene {
	const char* name;    /* headline drawn in the overlay */
	const char* detail;  /* one line on the technique */
	void (*init)(void);  /* build geometry once, before the GU starts */
	void (*draw)(int frame);
} Scene;

extern const Scene* const demo_scenes[];
extern const int demo_scene_count;

/* VRAM offset of the buffer currently drawn to; main updates it after every swap. */
extern void* demo_draw_buffer;

void demo_reset_state(void);
void demo_target_begin(void);  /* draw into the 128x128 texture at VRAM_RT */
void demo_target_end(void);    /* back to the frame buffer */

void demo_text_begin(void);
void demo_draw_string(const char* text, int x, int y, unsigned int color, int fw);
int  demo_string_width(const char* text, int fw);
void demo_draw_rect(int x, int y, int w, int h, unsigned int color);

/* raw textures, linked through bin2o */
extern unsigned char logo_start[];      /* 64x64  GU_PSM_4444 */
extern unsigned char env0_start[];      /* 64x64  GU_PSM_4444 */
extern unsigned char lightmap_start[];  /* 64x64  GU_PSM_8888 */
extern unsigned char ball_start[];      /* 32x32  GU_PSM_5551 */
extern unsigned char font_start[];      /* 256x128 GU_PSM_8888 */

static inline float deg(float d) { return d * (GU_PI / 180.0f); }

#endif
