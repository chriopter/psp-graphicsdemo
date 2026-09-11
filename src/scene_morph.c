/* Morph targets - after the pspsdk "morph" sample. */
#include "demo.h"

#define ROWS 64
#define COLS 64
#define MAXF(a,b) (((a)<(b)) ? (b) : (a))
#define MINF(a,b) (((a)<(b)) ? (a) : (b))

typedef struct { unsigned int color; ScePspFVector3 normal; ScePspFVector3 pos; } Vertex;
typedef struct { Vertex v0; Vertex v1; } MorphVertex;

static unsigned short __attribute__((aligned(16))) indices[ROWS*COLS*6];
static MorphVertex __attribute__((aligned(16))) vertices[ROWS*COLS];

static void init(void)
{
	unsigned int i, j;
	for (i = 0; i < ROWS; ++i) {
		float s = (((float)i)/ROWS) * GU_PI * 2;
		ScePspFVector3 v = { cosf(s), cosf(s), sinf(s) };
		for (j = 0; j < COLS; ++j) {
			unsigned short* curr = &indices[(j+(i*COLS))*6];
			unsigned int i1 = (i+1)%ROWS, j1 = (j+1)%COLS;
			float t = (((float)j)/COLS) * GU_PI * 2;
			ScePspFVector3 v2 = { v.x * cosf(t), v.y * sinf(t), v.z };
			ScePspFVector3 v3;
			/* target 1: the sphere pushed out into a cube */
			v3.x = v2.x > 0 ? MINF(v2.x * 10.0f, 1.0f) : MAXF(v2.x * 10.0f, -1.0f);
			v3.y = v2.y > 0 ? MINF(v2.y * 10.0f, 1.0f) : MAXF(v2.y * 10.0f, -1.0f);
			v3.z = v2.z > 0 ? MINF(v2.z * 10.0f, 1.0f) : MAXF(v2.z * 10.0f, -1.0f);
			vertices[j+i*COLS].v0.color = (0xff<<24)|((int)(fabsf(v2.x) * 255.0f) << 16)|((int)(fabsf(v2.y) * 255.0f) << 8)|((int)(fabsf(v2.z) * 255.0f));
			vertices[j+i*COLS].v0.normal = v2;
			vertices[j+i*COLS].v0.pos = v2;
			vertices[j+i*COLS].v1.color = vertices[j+i*COLS].v0.color;
			vertices[j+i*COLS].v1.normal = v3;
			gumNormalize(&vertices[j+i*COLS].v1.normal);
			vertices[j+i*COLS].v1.pos = v3;
			*curr++ = j + i * COLS;
			*curr++ = j1 + i * COLS;
			*curr++ = j + i1 * COLS;
			*curr++ = j1 + i * COLS;
			*curr++ = j1 + i1 * COLS;
			*curr++ = j + i1 * COLS;
		}
	}
}

static DemoTurn turn;
static float weight;   /* how much of the sphere is left in the blend */
static int manual, flat;

static void reset(void)
{
	demo_turn_reset(&turn);
	weight = 0.5f;
	manual = 0;
	flat = 0;
}

static void draw(int frame, const DemoInput* in)
{
	ScePspFVector3 lpos = { 1, 0, 1 };
	float w;

	demo_turn_update(&turn, in);
	if (in->held & PSP_CTRL_RIGHT) {
		weight = clampf(weight - 0.02f, 0.0f, 1.0f);
		manual = 1;
	}
	if (in->held & PSP_CTRL_LEFT) {
		weight = clampf(weight + 0.02f, 0.0f, 1.0f);
		manual = 1;
	}
	if (in->pressed & PSP_CTRL_SQUARE)
		manual ^= 1;
	if (in->pressed & PSP_CTRL_CIRCLE)
		flat ^= 1;
	if (!manual)
		weight = 0.5f * sinf(deg(frame * 1.5f)) + 0.5f;
	w = weight;
	snprintf(demo_status, sizeof(demo_status), "cube %d%%  %s  %s",
		(int)((1.0f - w) * 100.0f + 0.5f), manual ? "HAND" : "AUTO", flat ? "FLAT" : "SMOOTH");

	sceGuShadeModel(flat ? GU_FLAT : GU_SMOOTH);
	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	sceGuEnable(GU_LIGHTING);
	sceGuEnable(GU_LIGHT0);
	sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE_AND_SPECULAR, &lpos);
	sceGuLightColor(0, GU_DIFFUSE_AND_SPECULAR, 0xffffffff);
	sceGuSpecular(12.0f);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0.0f, 0.0f, -2.5f * turn.zoom };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}
	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 rot = { deg(frame * 0.79f), deg(frame * 0.98f), deg(frame * 1.32f) };
		sceGumLoadIdentity();
		demo_turn_apply(&turn);
		sceGumRotateXYZ(&rot);
	}

	sceGuMorphWeight(0, w);
	sceGuMorphWeight(1, 1.0f - w);
	sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_NORMAL_32BITF | GU_VERTEX_32BITF | GU_VERTICES(2) | GU_INDEX_16BIT | GU_TRANSFORM_3D,
		sizeof(indices)/sizeof(unsigned short), indices, vertices);
}

const Scene scene_morph = { "MORPH TARGETS", "GU_VERTICES(2), blended by sceGuMorphWeight",
	"stick turn  ^v zoom  <> blend  [] auto  O flat", init, reset, draw };
