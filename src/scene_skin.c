/* Matrix skinning - after the pspsdk "skinning" sample. */
#include "demo.h"

#define WEIGHTS_PER_VERTEX 8
#define CYLINDER_SLICES 48
#define CYLINDER_ROWS 48
#define CYLINDER_RADIUS 0.35f
#define CYLINDER_LENGTH 1.25f
#define MINF(a,b) (((a)<(b)) ? (a) : (b))

typedef struct {
	float skinWeight[WEIGHTS_PER_VERTEX];
	float u, v;
	unsigned int color;
	float nx, ny, nz;
	float x, y, z;
} Vertex;

static Vertex __attribute__((aligned(16))) cylinder_vertices[CYLINDER_SLICES*CYLINDER_ROWS];
static unsigned short __attribute__((aligned(16))) cylinder_indices[CYLINDER_SLICES*(CYLINDER_ROWS-1)*6];

static void gen_cylinder(unsigned slices, unsigned rows, float length, float radius, unsigned bones)
{
	unsigned int i, j;
	float lengthStep = length / (float)rows;
	float boneStep = ((float)bones-1)/((float)rows);
	for (j = 0; j < slices; ++j) {
		for (i = 0; i < rows; ++i) {
			Vertex* curr = &cylinder_vertices[i+j*rows];
			float t = j;
			float ct = cosf(t * (2*GU_PI)/rows), st = sinf(t * (2*GU_PI)/rows);
			int q;
			curr->nx = 0; curr->ny = ct; curr->nz = st;
			curr->x = lengthStep * (float)i;
			curr->y = radius * ct;
			curr->z = radius * st;
			curr->u = 0; curr->v = 0;
			curr->color = 0xffffff;
			for (q = 0; q < bones; q++) {
				/* cubic B-spline blending: at most four bones touch a vertex */
				float b = MINF(((float)bones-1), boneStep * (float)i);
				float t = b - (float)q;
				float t2 = t*t, t3 = t*t*t, f = 0;
				if (t >= 0.0f && t < 1.0f) f =  t3/6.0f;
				if (t >= 1.0f && t < 2.0f) f = -0.5f*t3 + 2.0f*t2 - 2.0f*t  +  2.0f/3.0f;
				if (t >= 2.0f && t < 3.0f) f =  0.5f*t3 - 4.0f*t2 + 10.0f*t - 22.0f/3.0f;
				if (t >= 3.0f && t < 4.0f) f = -t3/6.0f + 2.0f*t2 - 8.0f*t  + 32.0f/3.0f;
				curr->skinWeight[q] = f;
			}
		}
	}
	for (j = 0; j < slices; ++j) {
		for (i = 0; i < rows-1; ++i) {
			unsigned short* curr = &cylinder_indices[(i+(j*(rows-1)))*6];
			*curr++ = i + j * rows;
			*curr++ = (i+1) + j * rows;
			*curr++ = i + ((j+1)%slices) * rows;
			*curr++ = (i+1) + j * rows;
			*curr++ = (i+1) + ((j+1)%slices) * rows;
			*curr++ = i + ((j+1)%slices) * rows;
		}
	}
}

static float bend, twist;        /* what the bones are doing this frame */
static float hold_bend, hold_twist, hold;  /* the hand on the stick, and how much of it shows */
static float yaw, zoom;
static int arms, pose;

static void init(void)
{
	gen_cylinder(CYLINDER_ROWS, CYLINDER_SLICES, CYLINDER_LENGTH, CYLINDER_RADIUS, WEIGHTS_PER_VERTEX);
}

static void reset(void)
{
	bend = 1.0f;
	twist = 0.0f;
	hold_bend = hold_twist = hold = 0.0f;
	yaw = 0.0f;
	zoom = 1.0f;
	arms = 4;
	pose = 0;
}

static void draw(int frame, const DemoInput* in)
{
	ScePspFMatrix4 bones[WEIGHTS_PER_VERTEX];
	ScePspFVector3 lightDir = { 0, 0, 1 };
	float swing;
	int q;

	/* The stick bends the chain by hand. Let go and the arms swing on by
	   themselves again, unless O was pressed to keep the pose. */
	/* The chain swings by itself; the stick takes it over and fades back out
	   of the way on release, so a scene nobody touches bends exactly as the
	   sample always did. */
	demo_zoom_update(&zoom, in);
	swing = cosf(deg(frame));
	if (in->x != 0.0f || in->y != 0.0f) {
		hold_bend = in->x * 1.3f;
		hold_twist = in->y * 0.9f;
		hold += (1.0f - hold) * 0.2f;
	} else if (!pose) {
		hold *= 0.92f;
		if (hold < 0.002f)
			hold = 0.0f;
	}
	bend = swing + (hold_bend - swing) * hold;
	twist = hold_twist * hold;
	if (in->held & PSP_CTRL_RIGHT)
		yaw += deg(2.0f);
	if (in->held & PSP_CTRL_LEFT)
		yaw -= deg(2.0f);
	if (in->pressed & PSP_CTRL_SQUARE)
		arms = (arms % 4) + 1;
	if (in->pressed & PSP_CTRL_CIRCLE)
		pose ^= 1;
	snprintf(demo_status, sizeof(demo_status), "bend %+.2f  %d arms  %s",
		bend, arms, pose ? "POSE" : "AUTO");

	sceGuClearColor(0xff554433);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuEnable(GU_DITHER);

	sceGuEnable(GU_LIGHTING);
	sceGuEnable(GU_LIGHT0);
	sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE, &lightDir);
	sceGuLightColor(0, GU_DIFFUSE, 0x00ff4040);
	sceGuLightAtt(0, 1.0f, 0.0f, 0.0f);
	sceGuAmbient(0x00202020);

	sceGumMatrixMode(GU_PROJECTION);
	sceGumLoadIdentity();
	sceGumPerspective(75.0f, 16.0f/9.0f, 0.5f, 1000.0f);
	sceGumMatrixMode(GU_VIEW);
	{
		ScePspFVector3 pos = { 0, 0, -5.0f * zoom };
		sceGumLoadIdentity();
		sceGumTranslate(&pos);
	}

	/* a chain of eight bones, each bent a little further than its parent */
	for (q = 0; q < WEIGHTS_PER_VERTEX; ++q) {
		ScePspFVector3 rot = { 0, twist, bend };
		gumLoadIdentity(&bones[q]);
		gumRotateXYZ(&bones[q], &rot);
		if (q > 0) {
			ScePspFVector3 pos = { CYLINDER_LENGTH, 0, 0 };
			gumTranslate(&bones[q], &pos);
			gumMultMatrix(&bones[q], &bones[q-1], &bones[q]);
		}
		sceGuBoneMatrix(q, &bones[q]);
		sceGuMorphWeight(q, 1.0f);
	}

	sceGumMatrixMode(GU_MODEL);
	{
		ScePspFVector3 rot = { GU_PI/7.0f, GU_PI/9.0f + yaw, 0 };
		sceGumLoadIdentity();
		sceGumRotateXYZ(&rot);
	}
	for (q = 0; q < arms; ++q) {
		ScePspFVector3 rot = { 0, 0, GU_PI/2.0f };
		sceGumRotateXYZ(&rot);
		sceGumDrawArray(GU_TRIANGLES,
			GU_WEIGHTS(WEIGHTS_PER_VERTEX) | GU_NORMAL_32BITF | GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_WEIGHT_32BITF | GU_INDEX_16BIT | GU_TRANSFORM_3D,
			sizeof(cylinder_indices)/sizeof(unsigned short), cylinder_indices, cylinder_vertices);
	}
}

const Scene scene_skin = { "MATRIX SKINNING", "eight bone matrices, cubic weights per vertex",
	"stick bend  ^v zoom  <> turn  [] arms  O keep pose", init, reset, draw };
