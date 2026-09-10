/* Depth buffer fog - after the pspsdk "zbufferfog" sample. */
#include "demo.h"
#include "geometry.h"

#define TORUS_SLICES 48
#define TORUS_ROWS 48
#define ZBUFFER_SLICE 64
#define ZFAR_LIMIT 64
#define ZNEAR_LIMIT 256
#define FOG_COLOR 0x554433

/* the depth buffer is swizzled; this window on VRAM shows it linear */
#define ZBUFFER_LINEAR(x) (0x600000 + (x))
#define VRAM_ABS(x) (0x4000000 + (x))

static TCPVertex __attribute__((aligned(16))) torus_vertices[TORUS_SLICES*TORUS_ROWS];
static unsigned short __attribute__((aligned(16))) torus_indices[TORUS_SLICES*TORUS_ROWS*6];
static unsigned int __attribute__((aligned(16))) fogPalette[256];

static void init(void)
{
	int i;
	generateTorusTCP(TORUS_SLICES, TORUS_ROWS, 1.0f, 0.5f, torus_vertices, torus_indices);
	for (i = 0; i < 256; ++i) {
		unsigned int far = (i - ZFAR_LIMIT) < 0 ? 0 : (i - ZFAR_LIMIT);
		unsigned int near = (far * 256) / (ZNEAR_LIMIT-ZFAR_LIMIT);
		unsigned int k = near > 255 ? 255 : near;
		fogPalette[i] = (k << 24) | FOG_COLOR;
	}
}

/* Blit the depth buffer over the frame as an 8-bit texture. Only the high
   byte of each 16-bit depth value is sampled, in 64-pixel strips. */
static void render_fog(void)
{
	int i, j;
	for (i = 0; i < 2; ++i) {
		sceGuTexMode(GU_PSM_T8, 0, 0, 0);
		sceGuTexImage(0, 512, 512, 1024, (void*)VRAM_ABS(ZBUFFER_LINEAR(VRAM_ZBUF + i * (256*2))));
		sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
		sceGuTexFilter(GU_NEAREST, GU_NEAREST);
		sceGuColor(0);
		for (j = 0; j < (256/ZBUFFER_SLICE); ++j) {
			TPVertex* v = (TPVertex*)sceGuGetMemory(sizeof(TPVertex)*2);
			v[0].texture.x = 1 + j*(ZBUFFER_SLICE * 2);
			v[0].texture.y = 0;
			v[0].position.x = j*ZBUFFER_SLICE + i * 256;
			v[0].position.y = 0;
			v[0].position.z = 0;
			v[1].texture.x = 1 + (j+1)*(ZBUFFER_SLICE * 2);
			v[1].texture.y = SCR_HEIGHT;
			v[1].position.x = (j+1)*ZBUFFER_SLICE + i * 256;
			v[1].position.y = SCR_HEIGHT;
			v[1].position.z = 0;
			sceGuDrawArray(GU_SPRITES, TP_VERTEX_FORMAT | GU_TRANSFORM_2D, 2, 0, v);
		}
	}
}

static void draw(int frame)
{
	sceGuClearColor(0xff000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuDepthRange(65535, 0);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 1.0f, 100.0f);
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	{
		ScePspFVector3 pos = { 0, 0, -5.0f + sinf(deg(frame)) * 2.5f };
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);
	}
	sceGumDrawArray(GU_TRIANGLES, TCP_VERTEX_FORMAT | GU_INDEX_16BIT | GU_TRANSFORM_3D, sizeof(torus_indices)/sizeof(unsigned short), torus_indices, torus_vertices);

	/* fog: the depth buffer through a palette whose alpha grows with distance */
	sceGuEnable(GU_TEXTURE_2D);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuDepthMask(GU_TRUE);
	sceGuClutMode(GU_PSM_8888, 0, 255, 0);
	sceGuClutLoad(256/8, fogPalette);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_ONE_MINUS_SRC_ALPHA, GU_SRC_ALPHA, 0, 0);
	render_fog();
	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuDepthMask(GU_FALSE);
}

const Scene scene_fog = { "DEPTH BUFFER FOG", "z-buffer read as 8-bit texture through a CLUT", init, draw };
