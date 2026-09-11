/* Cel shading - after the pspsdk "celshading" sample. */
#include "demo.h"

#define TORUS_SLICES 48
#define TORUS_ROWS 48
#define TORUS_RADIUS 1.0f
#define TORUS_THICKNESS 0.5f

typedef struct { float normal[3]; float vertex[3]; } VxNP;
typedef struct { float vertex[3]; } VxP;

static VxNP __attribute__((aligned(16))) torus_v0[TORUS_SLICES * TORUS_ROWS];
static VxP  __attribute__((aligned(16))) torus_v1[TORUS_SLICES * TORUS_ROWS];
static unsigned short __attribute__((aligned(16))) torus_in[TORUS_SLICES * TORUS_ROWS * 6];

/* how far the outline hull is pushed out along the normals */
static const float outlines[] = { 0.1f, 0.2f, 0.04f, 0.0f };
static const unsigned int tints[] = { 0xff9966, 0x66ccff, 0x77ee77, 0xcc88ff, 0xffffff };

static DemoTurn turn;
static float light;
static int outline, tint;

/* The hull is geometry, so a new width means rebuilding it and pushing it out of the cache. */
static void build_hull(float width)
{
	unsigned int i;
	for (i = 0; i < TORUS_SLICES * TORUS_ROWS; ++i) {
		torus_v1[i].vertex[0] = torus_v0[i].vertex[0] + torus_v0[i].normal[0] * width;
		torus_v1[i].vertex[1] = torus_v0[i].vertex[1] + torus_v0[i].normal[1] * width;
		torus_v1[i].vertex[2] = torus_v0[i].vertex[2] + torus_v0[i].normal[2] * width;
	}
	sceKernelDcacheWritebackRange(torus_v1, sizeof(torus_v1));
}

static void reset(void)
{
	demo_turn_reset(&turn);
	light = 0.0f;
	outline = 0;
	tint = 0;
	build_hull(outlines[0]);
}

static void init(void)
{
	unsigned int i, j;
	for (j = 0; j < TORUS_SLICES; ++j) {
		for (i = 0; i < TORUS_ROWS; ++i) {
			VxNP* v0 = &torus_v0[i + j*TORUS_ROWS];
			float s = i + 0.5f, t = j;
			float cs = cosf(s * (2*GU_PI)/TORUS_SLICES), ct = cosf(t * (2*GU_PI)/TORUS_ROWS);
			float ss = sinf(s * (2*GU_PI)/TORUS_SLICES), st = sinf(t * (2*GU_PI)/TORUS_ROWS);
			v0->normal[0] = cs * ct;
			v0->normal[1] = cs * st;
			v0->normal[2] = ss;
			v0->vertex[0] = (TORUS_RADIUS + TORUS_THICKNESS * cs) * ct;
			v0->vertex[1] = (TORUS_RADIUS + TORUS_THICKNESS * cs) * st;
			v0->vertex[2] = TORUS_THICKNESS * ss;
			/* the outline hull rides along the normals; reset() builds it */
		}
	}
	for (j = 0; j < TORUS_SLICES; ++j) {
		for (i = 0; i < TORUS_ROWS; ++i) {
			unsigned short* in = &torus_in[(i+(j*TORUS_ROWS))*6];
			unsigned int i1 = (i+1)%TORUS_ROWS, j1 = (j+1)%TORUS_SLICES;
			*in++ = i + j * TORUS_ROWS;
			*in++ = i1 + j * TORUS_ROWS;
			*in++ = i + j1 * TORUS_ROWS;
			*in++ = i1 + j * TORUS_ROWS;
			*in++ = i1 + j1 * TORUS_ROWS;
			*in++ = i + j1 * TORUS_ROWS;
		}
	}
}

static void draw(int frame, const DemoInput* in)
{
	ScePspFVector3 columns[2];
	float cs, sn;

	demo_turn_update(&turn, in);
	if (in->held & PSP_CTRL_RIGHT)
		light += deg(2.0f);
	if (in->held & PSP_CTRL_LEFT)
		light -= deg(2.0f);
	if (in->pressed & PSP_CTRL_SQUARE)
		tint = (tint + 1) % COUNT(tints);
	if (in->pressed & PSP_CTRL_CIRCLE) {
		outline = (outline + 1) % COUNT(outlines);
		build_hull(outlines[outline]);
	}
	snprintf(demo_status, sizeof(demo_status), "outline %.2f  colour %d",
		outlines[outline], tint + 1);

	/* turning these two columns rolls the lightmap around the view axis:
	   the band of light moves as if the lamp were carried around the torus */
	cs = cosf(light);
	sn = sinf(light);
	columns[0].x = cs;  columns[0].y = sn; columns[0].z = 0.0f;
	columns[1].x = -sn; columns[1].y = cs; columns[1].z = 0.0f;

	sceGuClearColor(0xffffffff);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 1.0f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	sceGumLoadIdentity();
	sceGumMatrixMode(GU_MODEL);
	sceGumLoadIdentity();
	{
		ScePspFVector3 pos = { 0.0f, 0.0f, -3.0f * turn.zoom };
		ScePspFVector3 rot = { deg(frame * 0.5f), deg(frame * 0.8f), 0.0f };
		sceGumTranslate(&pos);
		demo_turn_apply(&turn);
		sceGumRotateXYZ(&rot);
	}

	/* Lighting stays off: the view-space normal picks a texel out of the
	   lightmap through the environment-map coordinate generator. */
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, 64, 64, 64, lightmap_start);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuColor(tints[tint]);
	sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE, &columns[0]);
	sceGuLight(1, GU_DIRECTIONAL, GU_DIFFUSE, &columns[1]);
	sceGuTexMapMode(GU_ENVIRONMENT_MAP, 0, 1);

	sceGuEnable(GU_TEXTURE_2D);
	sceGumDrawArray(GU_TRIANGLES, GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_INDEX_16BIT | GU_TRANSFORM_3D,
		sizeof(torus_in)/sizeof(unsigned short), torus_in, torus_v0);
	sceGuDisable(GU_TEXTURE_2D);

	/* outline: the inflated hull, back faces only, in black */
	if (outlines[outline] > 0.0f) {
		sceGuFrontFace(GU_CCW);
		sceGuColor(0x0);
		sceGumDrawArray(GU_TRIANGLES, GU_VERTEX_32BITF | GU_INDEX_16BIT | GU_TRANSFORM_3D,
			sizeof(torus_in)/sizeof(unsigned short), torus_in, torus_v1);
		sceGuFrontFace(GU_CW);
	}
}

const Scene scene_cel = { "CEL SHADING", "1D lightmap via envmap coords, hull outline",
	"stick turn  ^v zoom  <> light  [] colour  O outline", init, reset, draw };
