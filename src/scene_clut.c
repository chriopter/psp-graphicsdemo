/* Palette cycling - after the pspsdk "clut" sample. */
#include "demo.h"

typedef struct { float u, v; float x, y, z; } Vertex;

static unsigned int __attribute__((aligned(16))) clut256[256];
static unsigned char __attribute__((aligned(16))) tex256[256*256];

static void init(void)
{
	unsigned int i, j;
	for (j = 0; j < 256; ++j)
		for (i = 0; i < 256; ++i)
			tex256[i + j * 256] = j ^ i;
}

static void draw(int frame)
{
	unsigned int i;
	/* write the palette uncached, the GE reads it right after */
	unsigned int* clut = (unsigned int*)(((unsigned int)clut256) | 0x40000000);
	Vertex* v;

	for (i = 0; i < 256; ++i) {
		unsigned int j = (i + frame * 2) & 0xff;
		unsigned int r = j;
		unsigned int g = (j * 3) & 0xff;
		unsigned int b = 255 - j;
		*(clut++) = 0xff000000 | (b << 16) | (g << 8) | r;
	}

	sceGuClearColor(0xff000000);
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuDisable(GU_DEPTH_TEST);

	sceGuEnable(GU_TEXTURE_2D);
	sceGuClutMode(GU_PSM_8888, 0, 0xff, 0);
	sceGuClutLoad(256/8, clut256);
	sceGuTexMode(GU_PSM_T8, 0, 0, 0);
	sceGuTexImage(0, 256, 256, 256, tex256);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);

	v = (Vertex*)sceGuGetMemory(2 * sizeof(Vertex));
	v[0].u = 0;   v[0].v = 0;   v[0].x = 0;         v[0].y = 0;          v[0].z = 0;
	v[1].u = 256; v[1].v = 256; v[1].x = SCR_WIDTH; v[1].y = SCR_HEIGHT; v[1].z = 0;
	sceGuDrawArray(GU_SPRITES, GU_TEXTURE_32BITF | GU_VERTEX_32BITF | GU_TRANSFORM_2D, 2, 0, v);
	sceGuEnable(GU_DEPTH_TEST);
}

const Scene scene_clut = { "PALETTE CYCLING", "8-bit XOR texture, 256-entry CLUT cycled", init, draw };
