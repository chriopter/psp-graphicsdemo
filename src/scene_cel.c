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

static void init(void)
{
	unsigned int i, j;
	for (j = 0; j < TORUS_SLICES; ++j) {
		for (i = 0; i < TORUS_ROWS; ++i) {
			VxNP* v0 = &torus_v0[i + j*TORUS_ROWS];
			VxP*  v1 = &torus_v1[i + j*TORUS_ROWS];
			float s = i + 0.5f, t = j;
			float cs = cosf(s * (2*GU_PI)/TORUS_SLICES), ct = cosf(t * (2*GU_PI)/TORUS_ROWS);
			float ss = sinf(s * (2*GU_PI)/TORUS_SLICES), st = sinf(t * (2*GU_PI)/TORUS_ROWS);
			v0->normal[0] = cs * ct;
			v0->normal[1] = cs * st;
			v0->normal[2] = ss;
			v0->vertex[0] = (TORUS_RADIUS + TORUS_THICKNESS * cs) * ct;
			v0->vertex[1] = (TORUS_RADIUS + TORUS_THICKNESS * cs) * st;
			v0->vertex[2] = TORUS_THICKNESS * ss;
			/* the outline hull sits 0.1 along the normal */
			v1->vertex[0] = v0->vertex[0] + v0->normal[0] * 0.1f;
			v1->vertex[1] = v0->vertex[1] + v0->normal[1] * 0.1f;
			v1->vertex[2] = v0->vertex[2] + v0->normal[2] * 0.1f;
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

static void draw(int frame)
{
	ScePspFVector3 columns[2] = { { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } };

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
		ScePspFVector3 pos = { 0.0f, 0.0f, -3.0f };
		ScePspFVector3 rot = { deg(frame * 0.5f), deg(frame * 0.8f), 0.0f };
		sceGumTranslate(&pos);
		sceGumRotateXYZ(&rot);
	}

	/* Lighting stays off: the view-space normal picks a texel out of the
	   lightmap through the environment-map coordinate generator. */
	sceGuTexMode(GU_PSM_8888, 0, 0, 0);
	sceGuTexImage(0, 64, 64, 64, lightmap_start);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGB);
	sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	sceGuColor(0xff9966);
	sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE, &columns[0]);
	sceGuLight(1, GU_DIRECTIONAL, GU_DIFFUSE, &columns[1]);
	sceGuTexMapMode(GU_ENVIRONMENT_MAP, 0, 1);

	sceGuEnable(GU_TEXTURE_2D);
	sceGumDrawArray(GU_TRIANGLES, GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_INDEX_16BIT | GU_TRANSFORM_3D,
		sizeof(torus_in)/sizeof(unsigned short), torus_in, torus_v0);
	sceGuDisable(GU_TEXTURE_2D);

	/* outline: the inflated hull, back faces only, in black */
	sceGuFrontFace(GU_CCW);
	sceGuColor(0x0);
	sceGumDrawArray(GU_TRIANGLES, GU_VERTEX_32BITF | GU_INDEX_16BIT | GU_TRANSFORM_3D,
		sizeof(torus_in)/sizeof(unsigned short), torus_in, torus_v1);
	sceGuFrontFace(GU_CW);
}

const Scene scene_cel = { "CEL SHADING", "1D lightmap via envmap coords, hull outline", init, draw };
