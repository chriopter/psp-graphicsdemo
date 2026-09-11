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

/* how much of the mirror surface is left in front of the reflection */
static const unsigned char alphas[] = { 255, 170, 85, 0 };

static DemoTurn turn;
static float lift;
static int alpha, stencil;

static void init(void) {}

static void reset(void)
{
	demo_turn_reset(&turn);
	lift = 0.0f;
	alpha = 0;
	stencil = 1;
}

static void logo_texture(int func)
{
	sceGuTexMode(GU_PSM_4444, 0, 0, 0);
	sceGuTexImage(0, 64, 64, 64, logo_start);
	sceGuTexFunc(func, func == GU_TFX_ADD ? GU_TCC_RGB : GU_TCC_RGBA);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
}

static void draw(int frame, const DemoInput* in)
{
	float move, rot = deg(frame);
	Vertex* surface;
	int i;

	/* the stick walks the camera around the mirror */
	demo_turn_update(&turn, in);
	turn.pitch = clampf(turn.pitch, deg(-25.0f), deg(55.0f));
	if (in->held & PSP_CTRL_RIGHT)
		lift = clampf(lift + 0.03f, 0.0f, 2.5f);
	if (in->held & PSP_CTRL_LEFT)
		lift = clampf(lift - 0.03f, 0.0f, 2.5f);
	if (in->pressed & PSP_CTRL_SQUARE)
		alpha = (alpha + 1) % COUNT(alphas);
	if (in->pressed & PSP_CTRL_CIRCLE)
		stencil ^= 1;
	snprintf(demo_status, sizeof(demo_status), "mirror %d%%  %s",
		alphas[alpha] * 100 / 255, stencil ? "STENCIL" : "NO STENCIL");
	move = fabsf(sinf(deg(frame))) + 1.0f + lift;

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClearStencil(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_STENCIL_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(60.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0, -0.5f, -5.5f * turn.zoom };
		ScePspFVector3 r = { deg(30.0f) + turn.pitch, deg(frame * 0.2f) + turn.yaw, 0.0f };
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

	/* 2. the reflection, flipped in y, only where the stencil is 1.
	      With the test off it spills across the whole floor. */
	logo_texture(GU_TFX_ADD);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuFrontFace(GU_CW);
	if (!stencil)
		sceGuDisable(GU_STENCIL_TEST);
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
	logo_texture(GU_TFX_MODULATE);
	sceGuFrontFace(GU_CCW);
	sceGuEnable(GU_BLEND);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
	surface = sceGuGetMemory(sizeof(mirror));
	memcpy(surface, mirror, sizeof(mirror));
	for (i = 0; i < 6; ++i)
		surface[i].color = ((unsigned int)alphas[alpha] << 24) | 0xffffff;
	sceGumDrawArray(GU_TRIANGLES, VFMT, 6, 0, surface);
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

const Scene scene_mirror = { "STENCIL MIRROR", "stencil marks the mirror, flipped cube inside",
	"stick orbit  ^v zoom  <> lift  [] mirror  O stencil", init, reset, draw };
