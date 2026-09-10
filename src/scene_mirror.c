/* Stencil mirror - after the pspsdk "reflection" sample. */
#include "demo.h"

typedef struct { float u, v; unsigned int color; float x, y, z; } Vertex;

static Vertex __attribute__((aligned(16))) obj[36] = {
	{0, 0, 0xff6666ff,-1,-1, 1}, {1, 0, 0xff6666ff, 1,-1, 1}, {1, 1, 0xff6666ff, 1, 1, 1},
	{0, 0, 0xff6666ff,-1,-1, 1}, {1, 1, 0xff6666ff, 1, 1, 1}, {0, 1, 0xff6666ff,-1, 1, 1},
	{1, 1, 0xff66ff66, 1, 1, 1}, {0, 1, 0xff66ff66, 1,-1, 1}, {0, 0, 0xff66ff66, 1,-1,-1},
	{1, 1, 0xff66ff66, 1, 1, 1}, {0, 0, 0xff66ff66, 1,-1,-1}, {1, 0, 0xff66ff66, 1, 1,-1},
	{0, 1, 0xffff6666,-1, 1, 1}, {0, 0, 0xffff6666, 1, 1, 1}, {1, 0, 0xffff6666, 1, 1,-1},
	{0, 1, 0xffff6666,-1, 1, 1}, {1, 0, 0xffff6666, 1, 1,-1}, {1, 1, 0xffff6666,-1, 1,-1},
	{1, 1, 0xff6666ff, 1,-1,-1}, {0, 1, 0xff6666ff,-1,-1,-1}, {0, 0, 0xff6666ff,-1, 1,-1},
	{1, 1, 0xff6666ff, 1,-1,-1}, {0, 0, 0xff6666ff,-1, 1,-1}, {1, 0, 0xff6666ff, 1, 1,-1},
	{1, 0, 0xff66ff66,-1,-1,-1}, {1, 1, 0xff66ff66,-1,-1, 1}, {0, 1, 0xff66ff66,-1, 1, 1},
	{1, 0, 0xff66ff66,-1,-1,-1}, {0, 1, 0xff66ff66,-1, 1, 1}, {0, 0, 0xff66ff66,-1, 1,-1},
	{0, 1, 0xffff6666,-1,-1,-1}, {0, 0, 0xffff6666, 1,-1,-1}, {1, 0, 0xffff6666, 1,-1, 1},
	{0, 1, 0xffff6666,-1,-1,-1}, {1, 0, 0xffff6666, 1,-1, 1}, {1, 1, 0xffff6666,-1,-1, 1},
};

static Vertex __attribute__((aligned(16))) mirror[6] = {
	{0, 0, 0xaa000000, -2.0f, 0, -2.0f}, {2, 2, 0xaa000000,  2.0f, 0,  2.0f}, {2, 0, 0xaa000000,  2.0f, 0, -2.0f},
	{0, 0, 0xaa000000, -2.0f, 0, -2.0f}, {0, 2, 0xaa000000, -2.0f, 0,  2.0f}, {2, 2, 0xaa000000,  2.0f, 0,  2.0f},
};

static Vertex __attribute__((aligned(16))) border[6] = {
	{0, 0, 0xff0055aa, -2.125f, -0.01f, -2.125f}, {2, 2, 0xff0055aa,  2.125f, -0.01f,  2.125f}, {2, 0, 0xff0055aa,  2.125f, -0.01f, -2.125f},
	{0, 0, 0xff0055aa, -2.125f, -0.01f, -2.125f}, {0, 2, 0xff0055aa, -2.125f, -0.01f,  2.125f}, {2, 2, 0xff0055aa,  2.125f, -0.01f,  2.125f},
};

#define VFMT (GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D)

static void init(void) {}

static void logo_texture(int func)
{
	sceGuTexMode(GU_PSM_4444, 0, 0, 0);
	sceGuTexImage(0, 64, 64, 64, logo_start);
	sceGuTexFunc(func, func == GU_TFX_REPLACE ? GU_TCC_RGBA : GU_TCC_RGB);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
}

static void draw(int frame)
{
	float move = fabsf(sinf(deg(frame))) + 1.0f;
	float rot = deg(frame);

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClearStencil(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_STENCIL_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(60.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0, -0.5f, -5.5f };
		ScePspFVector3 r = { deg(30.0f), deg(frame * 0.2f), 0.0f };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&r);
	}
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();

	/* 1. the mirror writes a 1 into the stencil, colour and depth untouched */
	sceGumPushMatrix();
	{
		ScePspFVector3 scale = { 1, -1, 1 };
		sceGumScale(&scale);
	}
	sceGuFrontFace(GU_CCW);
	sceGuEnable(GU_STENCIL_TEST);
	sceGuDepthMask(GU_TRUE);
	sceGuStencilFunc(GU_ALWAYS, 1, 1);
	sceGuStencilOp(GU_KEEP, GU_KEEP, GU_REPLACE);
	sceGumDrawArray(GU_TRIANGLES, VFMT, 6, 0, mirror);
	sceGuDepthMask(GU_FALSE);

	/* 2. the reflection, flipped in y, only where the stencil is 1 */
	logo_texture(GU_TFX_ADD);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuFrontFace(GU_CW);
	sceGuStencilFunc(GU_EQUAL, 1, 1);
	sceGuStencilOp(GU_KEEP, GU_KEEP, GU_KEEP);
	{
		ScePspFVector3 pos = { 0, move, 0 };
		ScePspFVector3 rvec = { 0, rot * -0.83f, 0 };
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rvec);
	}
	sceGumDrawArray(GU_TRIANGLES, VFMT, 36, 0, obj);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuDisable(GU_STENCIL_TEST);
	sceGumPopMatrix();

	/* 3. the mirror surface itself, translucent, plus its border */
	sceGuEnable(GU_TEXTURE_2D);
	logo_texture(GU_TFX_REPLACE);
	sceGuFrontFace(GU_CCW);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	sceGumDrawArray(GU_TRIANGLES, VFMT, 6, 0, mirror);
	sceGuDisable(GU_BLEND);
	sceGuDisable(GU_TEXTURE_2D);
	sceGumDrawArray(GU_TRIANGLES, VFMT, 6, 0, border);

	/* 4. the object above the mirror */
	sceGuEnable(GU_TEXTURE_2D);
	logo_texture(GU_TFX_ADD);
	{
		ScePspFVector3 pos = { 0, move, 0 };
		ScePspFVector3 rvec = { 0, rot * -0.83f, 0 };
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rvec);
	}
	sceGumDrawArray(GU_TRIANGLES, VFMT, 36, 0, obj);
	sceGuDisable(GU_TEXTURE_2D);
	sceGuFrontFace(GU_CW);
}

const Scene scene_mirror = { "STENCIL MIRROR", "stencil marks the mirror, flipped cube inside", init, draw };
