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
#include <pspctrl.h>
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

/* Buttons that belong to the scene; touching any of them, or the stick, holds the autoplay. */
#define SCENE_BUTTONS (PSP_CTRL_UP | PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_RIGHT | \
	PSP_CTRL_SQUARE | PSP_CTRL_CIRCLE | PSP_CTRL_TRIANGLE | PSP_CTRL_SELECT)

/* The pad as a scene sees it, one frame at a time. */
typedef struct DemoInput {
	float x, y;             /* analog stick, -1..1 with the dead zone cut out, +y is down */
	unsigned int held;      /* PSP_CTRL_* bits down this frame */
	unsigned int pressed;   /* bits that went down this frame */
	unsigned int repeat;    /* pressed, then again every few frames while held */
} DemoInput;

typedef struct Scene {
	const char* name;      /* headline drawn in the overlay */
	const char* detail;    /* one line on the technique */
	const char* controls;  /* the scene's own buttons, shown once the player takes over */
	void (*init)(void);    /* build geometry once, before the GU starts */
	void (*reset)(void);   /* settings back to their defaults, at start and on SELECT */
	/* frame is the scene's own clock: it stops while the scene is frozen */
	void (*draw)(int frame, const DemoInput* in);
} Scene;

extern const Scene* const demo_scenes[];
extern const int demo_scene_count;

/* A scene writes its current settings here while drawing; the overlay shows them. */
extern char demo_status[64];

/* Stick turns the object, up/down moves the camera: what most scenes share. */
typedef struct DemoTurn {
	float yaw, pitch;  /* radians, added up from the stick */
	float zoom;        /* camera distance factor, 1 is where the sample put it */
} DemoTurn;

void demo_zoom_update(float* zoom, const DemoInput* in);  /* up/down alone */
void demo_turn_reset(DemoTurn* turn);
void demo_turn_update(DemoTurn* turn, const DemoInput* in);
void demo_turn_apply(const DemoTurn* turn);  /* pitch, then yaw, onto the current gum matrix */

/* VRAM offset of the buffer currently drawn to; main updates it after every swap. */
extern void* demo_draw_buffer;

void demo_reset_state(void);
void demo_target_begin(int size);  /* draw into the top-left size x size of the texture at VRAM_RT */
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
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

#endif
