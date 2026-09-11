/* Textured cube - after the pspsdk "cube" sample (Jesper Svennevid). */
#include "demo.h"

typedef struct { float u, v; unsigned int color; float x, y, z; } Vertex;

static Vertex __attribute__((aligned(16))) vertices[12*3] = {
	{0, 0, 0xff7f0000,-1,-1, 1}, {1, 0, 0xff7f0000,-1, 1, 1}, {1, 1, 0xff7f0000, 1, 1, 1},
	{0, 0, 0xff7f0000,-1,-1, 1}, {1, 1, 0xff7f0000, 1, 1, 1}, {0, 1, 0xff7f0000, 1,-1, 1},
	{0, 0, 0xff7f0000,-1,-1,-1}, {1, 0, 0xff7f0000, 1,-1,-1}, {1, 1, 0xff7f0000, 1, 1,-1},
	{0, 0, 0xff7f0000,-1,-1,-1}, {1, 1, 0xff7f0000, 1, 1,-1}, {0, 1, 0xff7f0000,-1, 1,-1},
	{0, 0, 0xff007f00, 1,-1,-1}, {1, 0, 0xff007f00, 1,-1, 1}, {1, 1, 0xff007f00, 1, 1, 1},
	{0, 0, 0xff007f00, 1,-1,-1}, {1, 1, 0xff007f00, 1, 1, 1}, {0, 1, 0xff007f00, 1, 1,-1},
	{0, 0, 0xff007f00,-1,-1,-1}, {1, 0, 0xff007f00,-1, 1,-1}, {1, 1, 0xff007f00,-1, 1, 1},
	{0, 0, 0xff007f00,-1,-1,-1}, {1, 1, 0xff007f00,-1, 1, 1}, {0, 1, 0xff007f00,-1,-1, 1},
	{0, 0, 0xff00007f,-1, 1,-1}, {1, 0, 0xff00007f, 1, 1,-1}, {1, 1, 0xff00007f, 1, 1, 1},
	{0, 0, 0xff00007f,-1, 1,-1}, {1, 1, 0xff00007f, 1, 1, 1}, {0, 1, 0xff00007f,-1, 1, 1},
	{0, 0, 0xff00007f,-1,-1,-1}, {1, 0, 0xff00007f,-1,-1, 1}, {1, 1, 0xff00007f, 1,-1, 1},
	{0, 0, 0xff00007f,-1,-1,-1}, {1, 1, 0xff00007f, 1,-1, 1}, {0, 1, 0xff00007f, 1,-1,-1},
};

/* The texture functions, the knob this scene is really about. */
static const struct { int func; const char* name; } funcs[] = {
	{ GU_TFX_ADD,      "ADD" },
	{ GU_TFX_MODULATE, "MODULATE" },
	{ GU_TFX_DECAL,    "DECAL" },
	{ GU_TFX_BLEND,    "BLEND" },
	{ GU_TFX_REPLACE,  "REPLACE" },
};

static DemoTurn turn;
static int func, tiling, nearest;

static void init(void) {}

static void reset(void)
{
	demo_turn_reset(&turn);
	func = 0;
	tiling = 1;
	nearest = 0;
}

static void draw(int frame, const DemoInput* in)
{
	demo_turn_update(&turn, in);
	if (in->pressed & PSP_CTRL_SQUARE)
		func = (func + 1) % COUNT(funcs);
	if (in->pressed & PSP_CTRL_CIRCLE)
		nearest ^= 1;
	if ((in->repeat & PSP_CTRL_RIGHT) && tiling < 8)
		tiling++;
	if ((in->repeat & PSP_CTRL_LEFT) && tiling > 1)
		tiling--;
	snprintf(demo_status, sizeof(demo_status), "%s  x%d  %s",
		funcs[func].name, tiling, nearest ? "NEAREST" : "LINEAR");

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuDepthRange(65535, 0);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	{
		ScePspFVector3 pos = { 0, 0, -3.5f * turn.zoom };
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		sceGumTranslate(&pos);
		demo_turn_apply(&turn);
		sceGumRotateXYZ(&rot);
	}

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_4444, 0, 0, 0);
	sceGuTexImage(0, 64, 64, 64, logo_start);
	sceGuTexFunc(funcs[func].func, GU_TCC_RGBA);
	sceGuTexEnvColor(0xffff00);
	sceGuTexFilter(nearest ? GU_NEAREST : GU_LINEAR, nearest ? GU_NEAREST : GU_LINEAR);
	sceGuTexScale((float)tiling, (float)tiling);

	sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 12*3, 0, vertices);
}

const Scene scene_cube = { "TEXTURED CUBE", "sceGumDrawArray, 4444 texture added to colours",
	"stick turn  ^v zoom  <> tiling  [] func  O filter", init, reset, draw };
