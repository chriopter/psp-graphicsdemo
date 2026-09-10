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

static void init(void) {}

static void draw(int frame)
{
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
		ScePspFVector3 pos = { 0, 0, -3.5f };
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);
	}

	sceGuEnable(GU_TEXTURE_2D);
	sceGuTexMode(GU_PSM_4444, 0, 0, 0);
	sceGuTexImage(0, 64, 64, 64, logo_start);
	sceGuTexFunc(GU_TFX_ADD, GU_TCC_RGB);
	sceGuTexEnvColor(0xffff00);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	sceGumDrawArray(GU_TRIANGLES, GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 12*3, 0, vertices);
}

const Scene scene_cube = { "TEXTURED CUBE", "sceGumDrawArray, 4444 texture added to colours", init, draw };
